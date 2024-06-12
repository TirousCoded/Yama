

#include <gtest/gtest.h>

#include <memory>
#include <format>
#include <unordered_map>

#include <yama/core/asserts.h>
#include <yama/query-systems/all.h>
#include <yama/debug-layers/stderr_debug.h>


// due to how Yama's query system is an interconnected system of different
// templated components, w/ a bunch of trait types, I think the best way to
// go about testing it is all as one contiguous system, rather than as 
// individual components tested in isolation

// to this end, these tests will hereafter define a small self-contained
// query system which the behaviour of the impl will then be tested in 
// relation to

// I like this as it lets us comprehensively test things, while still
// decoupling the query system itself from Yama's specific semantics, which
// if we attempted to test as part of this would involve us having to change
// these tests all the time as we add/remove/modify things


enum class test_qtype {
    a,
    b,
    c,
};

// for this mock query system impl we're gonna have key_# map to result_#,
// where each key_# and result_# encapsulate a simple string

// likewise, each primary source will have a string 'name'

// for resource 'a', querying w/ some key will produce a result who's string
// will include the key string, and the primary source name

// for resource 'b', querying w/ some key will produce the same as w/ 'a',
// but w/ resource 'b' queries also querying a 'a' from the provider's
// secondary source, w/ the result's string being incorporated into the
// resource 'b' result's string

// for resource 'b', querying will fail if the nested 'a' query fails

// for resource 'c', we'll have no providers at all, so that we can test
// for in scenarios where no provider can be found

//      (resource 'a')  "abc" -> "a(abc, src_a1)"
//      (resource 'b')  "abc" -> "a(abc, src_a1) b(abc, src_b1)"
//      (resource 'c')  "abc" -> *fail due to no provider found*

// for both resources 'a' and 'b', the key "illegal" will be illegal to query,
// resulting in a failed computation/caching of new result data

// for resource 'a', the key "illegal-for-a" will be directly illegal to query,
// to test scenarios where 'b' queries fail due to failed 'a' queries therein

struct key_a final {
    std::string s;
};
struct key_b final {
    std::string s;
};
struct key_c final {
    std::string s;
};

struct result_a final {
    std::string s;
};
struct result_b final {
    std::string s;
};
struct result_c final {
    std::string s;
};

template<>
struct yama::qs::system_traits<test_qtype> final {
    using qtypes = test_qtype;
};

template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::a> final {
    using qtypes = test_qtype;
    static constexpr auto qtype = test_qtype::a;
    using key = key_a;
    using result = result_a;
};
template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::b> final {
    using qtypes = test_qtype;
    static constexpr auto qtype = test_qtype::b;
    using key = key_b;
    using result = result_b;
};
template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::c> final {
    using qtypes = test_qtype;
    static constexpr auto qtype = test_qtype::c;
    using key = key_c;
    using result = result_c;
};

template<>
struct yama::qs::key_traits<test_qtype, key_a> final {
    using qtypes = test_qtype;
    using key = key_a;
    static constexpr auto qtype = test_qtype::a;
};
template<>
struct yama::qs::key_traits<test_qtype, key_b> final {
    using qtypes = test_qtype;
    using key = key_b;
    static constexpr auto qtype = test_qtype::b;
};
template<>
struct yama::qs::key_traits<test_qtype, key_c> final {
    using qtypes = test_qtype;
    using key = key_c;
    static constexpr auto qtype = test_qtype::c;
};


struct primary_source_a final {
    std::string name;
};

struct primary_source_b final {
    std::string name;
};


struct provider_a_policy final {
    using key_t = key_a;
    using result_t = result_a;
    using system_t = yama::qs::system<test_qtype>;

    using primary_source = primary_source_a;


    primary_source* primary;
    system_t* secondary;

    std::unordered_map<std::string, std::shared_ptr<result_t>> cache;


    provider_a_policy(primary_source& primary_src, system_t& secondary_src) 
        : primary(&primary_src), 
        secondary(&secondary_src) {}


    size_t number() const noexcept {
        return cache.size();
    }
    
    std::shared_ptr<result_t> query(const key_t& key) {
        std::shared_ptr<result_t> result = nullptr;
        YAMA_DEREF_SAFE(primary && secondary) {
            if (key.s != "illegal" && key.s != "illegal-for-a") {
                result = std::make_shared<result_t>(std::format("a({}, {})", key.s, primary->name));
                cache[key.s] = result;
            }
        }
        return result;
    }
    
    std::shared_ptr<result_t> fetch(const key_t& key) {
        if (cache.contains(key.s)) {
            return cache.at(key.s);
        }
        return nullptr;
    }
    
    void discard(const key_t& key) {
        cache.erase(key.s);
    }
    
    void discard_all() {
        cache.clear();
    }
};

struct provider_b_policy final {
    using key_t = key_b;
    using result_t = result_b;
    using system_t = yama::qs::system<test_qtype>;

    using primary_source = primary_source_b;


    primary_source* primary;
    system_t* secondary;

    std::unordered_map<std::string, std::shared_ptr<result_t>> cache;


    provider_b_policy(primary_source& primary_src, system_t& secondary_src) 
        : primary(&primary_src), 
        secondary(&secondary_src) {}


    size_t number() const noexcept {
        return cache.size();
    }
    
    std::shared_ptr<result_t> query(const key_t& key) {
        std::shared_ptr<result_t> result = nullptr;
        YAMA_DEREF_SAFE(primary && secondary) {
            if (key.s != "illegal") {
                auto res_a = secondary->query(key_a{ key.s });
                if (res_a) {
                    result = std::make_shared<result_t>(std::format("{} b({}, {})", res_a->s, key.s, primary->name));
                    cache[key.s] = result;
                }
            }
        }
        return result;
    }
    
    std::shared_ptr<result_t> fetch(const key_t& key) {
        if (cache.contains(key.s)) {
            return cache.at(key.s);
        }
        return nullptr;
    }
    
    void discard(const key_t& key) {
        cache.erase(key.s);
    }
    
    void discard_all() {
        cache.clear();
    }
};

using provider_a = yama::qs::provider_impl<test_qtype, test_qtype::a, provider_a_policy>;
using provider_b = yama::qs::provider_impl<test_qtype, test_qtype::b, provider_b_policy>;


// below tests the two frontend portions of the query system: providers and systems

// these tests will involve two query system impls: one 'regular' and one 'irregular'

// the regular impl encapsulates two providers for resources 'a' and 'b', and these
// providers are passed the query system itself as their secondary information source

// the irregular impl is almost identical to the regular one, except that its provider
// for resource 'b' will be passed a totally different query system as its secondary
// information source, in order to test more nuanced scenarios where this may be a factor

class regular_system_impl final : public yama::qs::system<test_qtype> {
public:

    regular_system_impl(std::shared_ptr<yama::debug> dbg = nullptr) 
        : system(dbg), 
        src_a{ "src1" }, 
        src_b{ "src2" }, 
        prov_a(src_a, *this), 
        prov_b(src_b, *this) {}


protected:

    yama::qs::untyped_provider<test_qtype>* get_provider(test_qtype qtype) const noexcept override final {
        using return_t = yama::qs::untyped_provider<test_qtype>*;
        if (qtype == test_qtype::a) return (return_t)&prov_a;
        else if (qtype == test_qtype::b) return (return_t)&prov_b;
        else return nullptr;
    }

    void do_discard_all() override final {
        prov_a.discard_all();
        prov_b.discard_all();
    }


private:

    primary_source_a src_a;
    primary_source_b src_b;
    provider_a prov_a;
    provider_b prov_b;
};

class irregular_system_impl final : public yama::qs::system<test_qtype> {
public:

    regular_system_impl prov_b_upstream;


    irregular_system_impl(std::shared_ptr<yama::debug> dbg = nullptr) 
        : system(dbg), 
        prov_b_upstream(dbg),
        src_a{ "src1-original" }, 
        src_b{ "src2" }, 
        prov_a(src_a, *this), 
        prov_b(src_b, prov_b_upstream) {}


protected:

    yama::qs::untyped_provider<test_qtype>* get_provider(test_qtype qtype) const noexcept override final {
        using return_t = yama::qs::untyped_provider<test_qtype>*;
        if (qtype == test_qtype::a) return (return_t)&prov_a;
        else if (qtype == test_qtype::b) return (return_t)&prov_b;
        else return nullptr;
    }

    void do_discard_all() override final {
        prov_a.discard_all();
        prov_b.discard_all();
    }


private:

    primary_source_a src_a;
    primary_source_b src_b;
    provider_a prov_a;
    provider_b prov_b;
};


class QuerySystemTests : public testing::Test {
public:

    std::shared_ptr<yama::debug> dbg = nullptr;


    QuerySystemTests() 
        : Test(), 
        dbg(std::make_shared<yama::stderr_debug>(yama::all_c)) {}


protected:

    void SetUp() override final {
        //
    }

    void TearDown() override final {
        //
    }
};


// provider tests

TEST_F(QuerySystemTests, Provider_Number) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    EXPECT_EQ(prov.number(), 0);

    prov.query(key_a{ "a" });

    EXPECT_EQ(prov.number(), 1);

    prov.query(key_a{ "b" });

    EXPECT_EQ(prov.number(), 2);

    prov.query(key_a{ "b" });

    EXPECT_EQ(prov.number(), 2);
}

TEST_F(QuerySystemTests, Provider_Query) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    auto result0 = prov.query(key_a{ "abc" });

    EXPECT_EQ(prov.number(), 1);

    auto result1 = prov.query(key_a{ "def" });

    EXPECT_EQ(prov.number(), 2);

    auto result2 = prov.query(key_a{ "def" });

    EXPECT_EQ(prov.number(), 2);

    EXPECT_TRUE(result0);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);

    if (result0) EXPECT_EQ(result0->s, "a(abc, src)");
    if (result1) EXPECT_EQ(result1->s, "a(def, src)");
    if (result2) EXPECT_EQ(result2->s, "a(def, src)");
}

TEST_F(QuerySystemTests, Provider_Query_FailDueToQueryFailure) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    auto result = prov.query(key_a{ "illegal" });

    EXPECT_EQ(prov.number(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, Provider_Fetch) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    prov.query(key_a{ "def" }); // <- add new info

    ASSERT_EQ(prov.number(), 1);

    auto result = prov.fetch(key_a{ "def" });

    EXPECT_EQ(prov.number(), 1);

    EXPECT_TRUE(result);

    if (result) EXPECT_EQ(result->s, "a(def, src)");
}

TEST_F(QuerySystemTests, Provider_Fetch_FailDueToNoCachedResultData) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    auto result = prov.fetch(key_a{ "abc" });

    EXPECT_EQ(prov.number(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, Provider_Discard) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    prov.query(key_a{ "abc" });
    prov.query(key_a{ "def" });
    prov.query(key_a{ "ghi" });

    ASSERT_EQ(prov.number(), 3);

    prov.discard(key_a{ "def" });

    EXPECT_EQ(prov.number(), 2);

    EXPECT_TRUE(prov.fetch(key_a{ "abc" }));
    EXPECT_TRUE(prov.fetch(key_a{ "ghi" }));
}

TEST_F(QuerySystemTests, Provider_Discard_FailQuietlyDueToNoCachedResultDataFound) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    prov.query(key_a{ "abc" });
    prov.query(key_a{ "def" });
    prov.query(key_a{ "ghi" });

    ASSERT_EQ(prov.number(), 3);

    prov.discard(key_a{ "jkl" });

    EXPECT_EQ(prov.number(), 3);

    EXPECT_TRUE(prov.fetch(key_a{ "abc" }));
    EXPECT_TRUE(prov.fetch(key_a{ "def" }));
    EXPECT_TRUE(prov.fetch(key_a{ "ghi" }));
}

TEST_F(QuerySystemTests, Provider_DiscardAll) {
    primary_source_a primary{ "src" };
    regular_system_impl secondary{};
    provider_a prov(primary, secondary);

    prov.query(key_a{ "abc" });
    prov.query(key_a{ "def" });
    prov.query(key_a{ "ghi" });

    ASSERT_EQ(prov.number(), 3);

    prov.discard_all();

    EXPECT_EQ(prov.number(), 0);
}


// regular system tests

TEST_F(QuerySystemTests, System_Regular_Number_ByQType) {
    regular_system_impl sys(dbg);

    EXPECT_EQ(sys.number<test_qtype::a>(), 0);
    EXPECT_EQ(sys.number<test_qtype::b>(), 0);

    sys.query(key_a{ "a" });

    EXPECT_EQ(sys.number<test_qtype::a>(), 1);
    EXPECT_EQ(sys.number<test_qtype::b>(), 0);

    sys.query(key_a{ "b" });

    EXPECT_EQ(sys.number<test_qtype::a>(), 2);
    EXPECT_EQ(sys.number<test_qtype::b>(), 0);

    sys.query(key_a{ "b" });

    EXPECT_EQ(sys.number<test_qtype::a>(), 2);
    EXPECT_EQ(sys.number<test_qtype::b>(), 0);
}

TEST_F(QuerySystemTests, System_Regular_Number_ByQType_ReturnZeroIfNoProvider) {
    regular_system_impl sys(dbg);

    EXPECT_EQ(sys.number<test_qtype::c>(), 0);
}

TEST_F(QuerySystemTests, System_Regular_Number_ByKey) {
    regular_system_impl sys(dbg);

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    sys.query(key_a{ "a" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 0);

    sys.query(key_a{ "b" });

    EXPECT_EQ(sys.number<key_a>(), 2);
    EXPECT_EQ(sys.number<key_b>(), 0);

    sys.query(key_a{ "b" });

    EXPECT_EQ(sys.number<key_a>(), 2);
    EXPECT_EQ(sys.number<key_b>(), 0);
}

TEST_F(QuerySystemTests, System_Regular_Number_ByKey_ReturnZeroIfNoProvider) {
    regular_system_impl sys(dbg);

    EXPECT_EQ(sys.number<key_c>(), 0);
}

TEST_F(QuerySystemTests, System_Regular_Query) {
    regular_system_impl sys(dbg);

    auto result0 = sys.query(key_a{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 0);

    auto result1 = sys.query(key_a{ "def" });

    EXPECT_EQ(sys.number<key_a>(), 2);
    EXPECT_EQ(sys.number<key_b>(), 0);

    auto result2 = sys.query(key_a{ "def" });

    EXPECT_EQ(sys.number<key_a>(), 2);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_TRUE(result0);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);

    if (result0) EXPECT_EQ(result0->s, "a(abc, src1)");
    if (result1) EXPECT_EQ(result1->s, "a(def, src1)");
    if (result2) EXPECT_EQ(result2->s, "a(def, src1)");
}

TEST_F(QuerySystemTests, System_Regular_Query_IndirectQueryByAnotherQuery) {
    regular_system_impl sys(dbg);

    auto result0 = sys.query(key_b{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 1);

    auto result1 = sys.fetch(key_a{ "abc" }); // <- above query added this data

    EXPECT_TRUE(result0);
    EXPECT_TRUE(result1);

    if (result0) EXPECT_EQ(result0->s, "a(abc, src1) b(abc, src2)");
    if (result1) EXPECT_EQ(result1->s, "a(abc, src1)");
}

TEST_F(QuerySystemTests, System_Regular_Query_FailDueToQueryFailure) {
    regular_system_impl sys(dbg);

    auto result = sys.query(key_a{ "illegal" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Regular_Query_FailDueToIndirectQueryFailure) {
    regular_system_impl sys(dbg);

    // this test tests that querying resource b will fail if the upstream
    // query to resource a fails

    // remember that 'illegal-for-a' in this system only fails for resource a,
    // not *directly* for resource b

    auto result = sys.query(key_b{ "illegal-for-a" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Regular_Query_FailDueToNoProvider) {
    regular_system_impl sys(dbg);

    // there is no provider for resource c

    auto result = sys.query(key_c{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Regular_Fetch) {
    regular_system_impl sys(dbg);

    sys.query(key_a{ "def" }); // <- add new info

    ASSERT_EQ(sys.number<key_a>(), 1);
    ASSERT_EQ(sys.number<key_b>(), 0);

    auto result = sys.fetch(key_a{ "def" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_TRUE(result);

    if (result) EXPECT_EQ(result->s, "a(def, src1)");
}

TEST_F(QuerySystemTests, System_Regular_Fetch_FailDueToNoCachedResultData) {
    regular_system_impl sys(dbg);

    auto result = sys.fetch(key_a{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Regular_Discard) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.fetch(key_a{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_a{ "def" }));
    ASSERT_TRUE(sys.fetch(key_a{ "ghi" }));
    ASSERT_TRUE(sys.fetch(key_b{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_b{ "def" }));
    ASSERT_TRUE(sys.fetch(key_b{ "ghi" }));

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);

    //EXPECT_TRUE(sys.fetch(key_a{ "abc" }));
    //EXPECT_TRUE(sys.fetch(key_a{ "def" }));
    EXPECT_TRUE(sys.fetch(key_a{ "ghi" }));
    //EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Regular_Discard_FailQuietlyDueToNoCachedResultDataFound_AndWhileOtherArgsSucceed) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.fetch(key_a{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_a{ "def" }));
    ASSERT_TRUE(sys.fetch(key_a{ "ghi" }));
    ASSERT_TRUE(sys.fetch(key_b{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_b{ "def" }));
    ASSERT_TRUE(sys.fetch(key_b{ "ghi" }));

    // there is no resource a under 'jkl'

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" }, key_a{ "jkl" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);

    //EXPECT_TRUE(sys.fetch(key_a{ "abc" }));
    //EXPECT_TRUE(sys.fetch(key_a{ "def" }));
    EXPECT_TRUE(sys.fetch(key_a{ "ghi" }));
    //EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Regular_Discard_FailQuietlyDueToNoProvider_AndWhileOtherArgsSucceed) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_TRUE(sys.fetch(key_a{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_a{ "def" }));
    ASSERT_TRUE(sys.fetch(key_a{ "ghi" }));
    ASSERT_TRUE(sys.fetch(key_b{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_b{ "def" }));
    ASSERT_TRUE(sys.fetch(key_b{ "ghi" }));

    // there is no provider for resource c

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" }, key_c{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);
    ASSERT_EQ(sys.number<key_c>(), 0);

    //EXPECT_TRUE(sys.fetch(key_a{ "abc" }));
    //EXPECT_TRUE(sys.fetch(key_a{ "def" }));
    EXPECT_TRUE(sys.fetch(key_a{ "ghi" }));
    //EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Regular_Discard_FailQuietlyDueToZeroArgs) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.fetch(key_a{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_a{ "def" }));
    ASSERT_TRUE(sys.fetch(key_a{ "ghi" }));
    ASSERT_TRUE(sys.fetch(key_b{ "abc" }));
    ASSERT_TRUE(sys.fetch(key_b{ "def" }));
    ASSERT_TRUE(sys.fetch(key_b{ "ghi" }));

    sys.discard();

    EXPECT_EQ(sys.number<key_a>(), 3);
    EXPECT_EQ(sys.number<key_b>(), 3);

    EXPECT_TRUE(sys.fetch(key_a{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_a{ "def" }));
    EXPECT_TRUE(sys.fetch(key_a{ "ghi" }));
    EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Regular_DiscardAll_ForAllProviders) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    sys.discard_all();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);
    EXPECT_EQ(sys.number<key_c>(), 0);
}

TEST_F(QuerySystemTests, System_Regular_DiscardAll_ForSpecificProvider_ByQType) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    sys.discard_all<test_qtype::a>();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 3);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(sys.fetch(key_a{ "abc" }));
    EXPECT_FALSE(sys.fetch(key_a{ "def" }));
    EXPECT_FALSE(sys.fetch(key_a{ "ghi" }));
    EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Regular_DiscardAll_ForSpecificProvider_ByKey) {
    regular_system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    sys.discard_all<key_a>();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 3);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(sys.fetch(key_a{ "abc" }));
    EXPECT_FALSE(sys.fetch(key_a{ "def" }));
    EXPECT_FALSE(sys.fetch(key_a{ "ghi" }));
    EXPECT_TRUE(sys.fetch(key_b{ "abc" }));
    EXPECT_TRUE(sys.fetch(key_b{ "def" }));
    EXPECT_TRUE(sys.fetch(key_b{ "ghi" }));
}


// irregular system tests

// this is just gonna test what makes the irregular_system_impl 'irregular', as the
// rest is already covered by the above

TEST_F(QuerySystemTests, System_Irregular_Query_QueryProviderHasSecondarySourceWhichIsNotTheSystemWhichOwnsTheProvider) {
    irregular_system_impl sys(dbg);

    auto result = sys.query(key_b{ "abc" });

    // provider for resource b is not pulling upstream from the provider for
    // resource a from the system which owns it, instead pulling upstream
    // from a totally different system, and thus provider

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 1);
    EXPECT_EQ(sys.prov_b_upstream.number<key_a>(), 1);
    EXPECT_EQ(sys.prov_b_upstream.number<key_b>(), 0);

    EXPECT_TRUE(result);

    if (result) EXPECT_EQ(result->s, "a(abc, src1) b(abc, src2)");
}

