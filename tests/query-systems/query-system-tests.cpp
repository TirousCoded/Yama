

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
// query system which the behaviour of the Yama API will then be tested in 
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
    using qtype_enum = test_qtype;
};

static_assert(yama::qs::system_traits_conforms<test_qtype>);

template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::a> final {
    using qtype_enum = test_qtype;
    static constexpr auto qtype = test_qtype::a;
    using key = key_a;
    using result = result_a;
};
template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::b> final {
    using qtype_enum = test_qtype;
    static constexpr test_qtype qtype = test_qtype::b;
    using key = key_b;
    using result = result_b;
};
template<>
struct yama::qs::provider_traits<test_qtype, test_qtype::c> final {
    using qtype_enum = test_qtype;
    static constexpr test_qtype qtype = test_qtype::c;
    using key = key_c;
    using result = result_c;
};

static_assert(yama::qs::provider_traits_conforms<test_qtype, test_qtype::a>);
static_assert(yama::qs::provider_traits_conforms<test_qtype, test_qtype::b>);
static_assert(yama::qs::provider_traits_conforms<test_qtype, test_qtype::c>);

template<>
struct yama::qs::key_traits<test_qtype, key_a> final {
    using qtype_enum = test_qtype;
    using key = key_a;
    static constexpr auto qtype = test_qtype::a;
};
template<>
struct yama::qs::key_traits<test_qtype, key_b> final {
    using qtype_enum = test_qtype;
    using key = key_b;
    static constexpr auto qtype = test_qtype::b;
};
template<>
struct yama::qs::key_traits<test_qtype, key_c> final {
    using qtype_enum = test_qtype;
    using key = key_c;
    static constexpr auto qtype = test_qtype::c;
};

static_assert(yama::qs::key_traits_conforms<test_qtype, key_a>);
static_assert(yama::qs::key_traits_conforms<test_qtype, key_b>);
static_assert(yama::qs::key_traits_conforms<test_qtype, key_c>);


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

    std::unordered_map<std::string, result_t> cache;


    provider_a_policy(primary_source& primary_src, system_t& secondary_src) 
        : primary(&primary_src), 
        secondary(&secondary_src) {}


    size_t number() const noexcept {
        return cache.size();
    }

    bool exists(const key_t& k) const noexcept {
        return cache.contains(k.s);
    }
    
    std::optional<result_t> query(const key_t& k) {
        std::optional<result_t> result = std::nullopt;
        YAMA_DEREF_SAFE(primary && secondary) {
            if (k.s != "illegal" && k.s != "illegal-for-a") {
                result = std::make_optional<result_t>(std::format("a({}, {})", k.s, primary->name));
                cache[k.s] = *result; // dirty copy
            }
        }
        return result;
    }
    
    std::optional<result_t> fetch(const key_t& k) {
        return
            exists(k)
            ? std::make_optional(cache.at(k.s))
            : std::nullopt;
    }
    
    void discard(const key_t& k) {
        cache.erase(k.s);
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

    std::unordered_map<std::string, result_t> cache;


    provider_b_policy(primary_source& primary_src, system_t& secondary_src) 
        : primary(&primary_src), 
        secondary(&secondary_src) {}


    size_t number() const noexcept {
        return cache.size();
    }

    bool exists(const key_t& k) const noexcept {
        return cache.contains(k.s);
    }

    std::optional<result_t> query(const key_t& k) {
        std::optional<result_t> result = std::nullopt;
        YAMA_DEREF_SAFE(primary && secondary) {
            if (k.s != "illegal") {
                auto res_a = secondary->query(key_a{ k.s });
                if (res_a) {
                    result = std::make_optional<result_t>(std::format("{} b({}, {})", res_a->s, k.s, primary->name));
                    cache[k.s] = *result; // dirty copy
                }
            }
        }
        return result;
    }
    
    std::optional<result_t> fetch(const key_t& k) {
        return
            exists(k)
            ? std::make_optional(cache.at(k.s))
            : std::nullopt;
    }
    
    void discard(const key_t& k) {
        cache.erase(k.s);
    }
    
    void discard_all() {
        cache.clear();
    }
};

using provider_a = yama::qs::provider_impl<test_qtype, test_qtype::a, provider_a_policy>;
using provider_b = yama::qs::provider_impl<test_qtype, test_qtype::b, provider_b_policy>;


class system_impl final : public yama::qs::system<test_qtype> {
public:

    // making these public so we can use them in our provider tests to
    // tests the internal behaviour of a basic system impl

    primary_source_a src_a;
    primary_source_b src_b;
    provider_a prov_a;
    provider_b prov_b;

    // these help for detecting get_upstream, do_discard_all and do_reset calls

    mutable bool called_get_upstream = false;
    bool called_do_discard_all = false;
    bool called_do_reset = false;


    system_impl(std::shared_ptr<yama::debug> dbg = nullptr) 
        : system(dbg), 
        src_a{ "src1" }, 
        src_b{ "src2" }, 
        prov_a(src_a, *this), 
        prov_b(src_b, *this) {}


protected:

    std::shared_ptr<system<test_qtype>> get_upstream() const noexcept override final {
        called_get_upstream = true;
        return nullptr;
    }

    yama::qs::untyped_provider<test_qtype>* get_provider(test_qtype qtype) const noexcept override final {
        using return_t = yama::qs::untyped_provider<test_qtype>*;
        if (qtype == test_qtype::a) return (return_t)&prov_a;
        else if (qtype == test_qtype::b) return (return_t)&prov_b;
        else return nullptr;
    }

    void do_discard_all() override final {
        prov_a.discard_all();
        prov_b.discard_all();
        called_do_discard_all = true;
    }

    void do_reset() override final {
        prov_a.discard_all();
        prov_b.discard_all();
        called_do_reset = true;
    }
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
    system_impl sys(dbg);

    EXPECT_EQ(sys.prov_a.number(), 0);

    sys.prov_a.query(key_a{ "a" });

    EXPECT_EQ(sys.prov_a.number(), 1);

    sys.prov_a.query(key_a{ "b" });

    EXPECT_EQ(sys.prov_a.number(), 2);

    sys.prov_a.query(key_a{ "b" });

    EXPECT_EQ(sys.prov_a.number(), 2);
}

TEST_F(QuerySystemTests, Provider_Exists) {
    system_impl sys(dbg);

    EXPECT_FALSE(sys.prov_a.exists(key_a{ "abc" }));
    EXPECT_FALSE(sys.prov_a.exists(key_a{ "def" }));
    EXPECT_FALSE(sys.prov_a.exists(key_a{ "ghi" }));

    ASSERT_TRUE(sys.prov_a.query(key_a{ "abc" }));
    ASSERT_TRUE(sys.prov_a.query(key_a{ "ghi" }));

    EXPECT_TRUE(sys.prov_a.exists(key_a{ "abc" }));
    EXPECT_FALSE(sys.prov_a.exists(key_a{ "def" }));
    EXPECT_TRUE(sys.prov_a.exists(key_a{ "ghi" }));
}

TEST_F(QuerySystemTests, Provider_Query) {
    system_impl sys(dbg);

    auto result0 = sys.prov_a.query(key_a{ "abc" });

    EXPECT_EQ(sys.prov_a.number(), 1);

    auto result1 = sys.prov_a.query(key_a{ "def" });

    EXPECT_EQ(sys.prov_a.number(), 2);

    auto result2 = sys.prov_a.query(key_a{ "def" });

    EXPECT_EQ(sys.prov_a.number(), 2);

    EXPECT_TRUE(result0);
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);

    if (result0) EXPECT_EQ(result0->s, "a(abc, src1)");
    if (result1) EXPECT_EQ(result1->s, "a(def, src1)");
    if (result2) EXPECT_EQ(result2->s, "a(def, src1)");
}

TEST_F(QuerySystemTests, Provider_Query_FailDueToQueryFailure) {
    system_impl sys(dbg);

    auto result = sys.prov_a.query(key_a{ "illegal" });

    EXPECT_EQ(sys.prov_a.number(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, Provider_Fetch) {
    system_impl sys(dbg);

    sys.prov_a.query(key_a{ "def" }); // <- add new info

    ASSERT_EQ(sys.prov_a.number(), 1);

    auto result = sys.prov_a.fetch(key_a{ "def" });

    EXPECT_EQ(sys.prov_a.number(), 1);

    EXPECT_TRUE(result);

    if (result) EXPECT_EQ(result->s, "a(def, src1)");
}

TEST_F(QuerySystemTests, Provider_Fetch_FailDueToNoCachedResultData) {
    system_impl sys(dbg);

    auto result = sys.prov_a.fetch(key_a{ "abc" });

    EXPECT_EQ(sys.prov_a.number(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, Provider_Discard) {
    system_impl sys(dbg);

    sys.prov_a.query(key_a{ "abc" });
    sys.prov_a.query(key_a{ "def" });
    sys.prov_a.query(key_a{ "ghi" });

    ASSERT_EQ(sys.prov_a.number(), 3);

    sys.prov_a.discard(key_a{ "def" });

    EXPECT_EQ(sys.prov_a.number(), 2);

    EXPECT_TRUE(sys.prov_a.fetch(key_a{ "abc" }));
    EXPECT_TRUE(sys.prov_a.fetch(key_a{ "ghi" }));
}

TEST_F(QuerySystemTests, Provider_Discard_FailQuietlyDueToNoCachedResultDataFound) {
    system_impl sys(dbg);

    sys.prov_a.query(key_a{ "abc" });
    sys.prov_a.query(key_a{ "def" });
    sys.prov_a.query(key_a{ "ghi" });

    ASSERT_EQ(sys.prov_a.number(), 3);

    sys.prov_a.discard(key_a{ "jkl" });

    EXPECT_EQ(sys.prov_a.number(), 3);

    EXPECT_TRUE(sys.prov_a.fetch(key_a{ "abc" }));
    EXPECT_TRUE(sys.prov_a.fetch(key_a{ "def" }));
    EXPECT_TRUE(sys.prov_a.fetch(key_a{ "ghi" }));
}

TEST_F(QuerySystemTests, Provider_DiscardAll) {
    system_impl sys(dbg);

    sys.prov_a.query(key_a{ "abc" });
    sys.prov_a.query(key_a{ "def" });
    sys.prov_a.query(key_a{ "ghi" });

    ASSERT_EQ(sys.prov_a.number(), 3);

    sys.prov_a.discard_all();

    EXPECT_EQ(sys.prov_a.number(), 0);
}


// system tests

TEST_F(QuerySystemTests, System_Upstream) {
    system_impl sys(dbg);

    ASSERT_FALSE(sys.called_get_upstream);

    EXPECT_EQ(sys.upstream(), nullptr);

    EXPECT_TRUE(sys.called_get_upstream);
}

TEST_F(QuerySystemTests, System_Number_ByQType) {
    system_impl sys(dbg);

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

TEST_F(QuerySystemTests, System_Number_ByQType_ReturnZeroIfNoProvider) {
    system_impl sys(dbg);

    EXPECT_EQ(sys.number<test_qtype::c>(), 0);
}

TEST_F(QuerySystemTests, System_Number_ByKey) {
    system_impl sys(dbg);

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

TEST_F(QuerySystemTests, System_Number_ByKey_ReturnZeroIfNoProvider) {
    system_impl sys(dbg);

    EXPECT_EQ(sys.number<key_c>(), 0);
}

TEST_F(QuerySystemTests, System_Exists) {
    system_impl sys(dbg);

    EXPECT_FALSE(sys.exists(key_a{ "abc" }));
    EXPECT_FALSE(sys.exists(key_a{ "def" }));
    EXPECT_FALSE(sys.exists(key_a{ "ghi" }));
    EXPECT_FALSE(sys.exists(key_b{ "abc" }));
    EXPECT_FALSE(sys.exists(key_b{ "def" }));
    EXPECT_FALSE(sys.exists(key_b{ "ghi" }));
    
    // heterogeneous arg types

    EXPECT_FALSE(sys.exists(key_a{ "abc" }, key_b{ "ghi" }, key_a{ "def" }));

    ASSERT_TRUE(sys.query(key_b{ "abc" }));
    ASSERT_TRUE(sys.query(key_b{ "def" }));
    ASSERT_TRUE(sys.query(key_b{ "ghi" }));

    EXPECT_TRUE(sys.exists(key_a{ "abc" }));
    EXPECT_TRUE(sys.exists(key_a{ "def" }));
    EXPECT_TRUE(sys.exists(key_a{ "ghi" }));
    EXPECT_TRUE(sys.exists(key_b{ "abc" }));
    EXPECT_TRUE(sys.exists(key_b{ "def" }));
    EXPECT_TRUE(sys.exists(key_b{ "ghi" }));

    // heterogeneous arg types
    
    EXPECT_TRUE(sys.exists(key_a{ "abc" }, key_b{ "ghi" }, key_a{ "def" }));
}

TEST_F(QuerySystemTests, System_Exists_FailDueToNoCachedResultDataFound_AndWhileOtherArgsSucceed) {
    system_impl sys(dbg);

    ASSERT_TRUE(sys.query(key_b{ "abc" }));
    ASSERT_TRUE(sys.query(key_b{ "def" }));
    ASSERT_TRUE(sys.query(key_b{ "ghi" }));

    // there is no resource a under 'jkl'

    EXPECT_FALSE(sys.exists(key_a{ "abc" }, key_b{ "ghi" }, key_a{ "def" }, key_a{ "jkl" }));
}

TEST_F(QuerySystemTests, System_Exists_FailDueToNoProvider_AndWhileOtherArgsSucceed) {
    system_impl sys(dbg);

    ASSERT_TRUE(sys.query(key_b{ "abc" }));
    ASSERT_TRUE(sys.query(key_b{ "def" }));
    ASSERT_TRUE(sys.query(key_b{ "ghi" }));

    // there is no provider for resource c

    EXPECT_FALSE(sys.exists(key_a{ "abc" }, key_b{ "ghi" }, key_a{ "def" }, key_c{ "abc" }));
}

TEST_F(QuerySystemTests, System_Exists_ZeroArgsEdgeCase) {
    system_impl sys(dbg);

    // if we don't have any keys, then exists should always succeed, as there's
    // no scenario where the keys in question will not be found

    EXPECT_TRUE(sys.exists());
}

TEST_F(QuerySystemTests, System_Query) {
    system_impl sys(dbg);

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

TEST_F(QuerySystemTests, System_Query_IndirectQueryByAnotherQuery) {
    system_impl sys(dbg);

    auto result0 = sys.query(key_b{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 1);

    auto result1 = sys.fetch(key_a{ "abc" }); // <- above query added this data

    EXPECT_TRUE(result0);
    EXPECT_TRUE(result1);

    if (result0) EXPECT_EQ(result0->s, "a(abc, src1) b(abc, src2)");
    if (result1) EXPECT_EQ(result1->s, "a(abc, src1)");
}

TEST_F(QuerySystemTests, System_Query_FailDueToQueryFailure) {
    system_impl sys(dbg);

    auto result = sys.query(key_a{ "illegal" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Query_FailDueToIndirectQueryFailure) {
    system_impl sys(dbg);

    // this test tests that querying resource b will fail if the upstream
    // query to resource a fails

    // remember that 'illegal-for-a' in this system only fails for resource a,
    // not *directly* for resource b

    auto result = sys.query(key_b{ "illegal-for-a" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Query_FailDueToNoProvider) {
    system_impl sys(dbg);

    // there is no provider for resource c

    auto result = sys.query(key_c{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Fetch) {
    system_impl sys(dbg);

    sys.query(key_a{ "def" }); // <- add new info

    ASSERT_EQ(sys.number<key_a>(), 1);
    ASSERT_EQ(sys.number<key_b>(), 0);

    auto result = sys.fetch(key_a{ "def" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_TRUE(result);

    if (result) EXPECT_EQ(result->s, "a(def, src1)");
}

TEST_F(QuerySystemTests, System_Fetch_FailDueToNoCachedResultData) {
    system_impl sys(dbg);

    auto result = sys.fetch(key_a{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);

    EXPECT_FALSE(result);
}

TEST_F(QuerySystemTests, System_Discard) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.exists(
        key_a{ "abc" }, key_a{ "def" }, key_a{ "ghi" },
        key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);

    EXPECT_TRUE(sys.exists(
        /*key_a{"abc"}, key_a{"def"},*/ key_a{"ghi"},
        /*key_b{"abc"},*/ key_b{"def"}, key_b{"ghi"}));
}

TEST_F(QuerySystemTests, System_Discard_FailQuietlyDueToNoCachedResultDataFound_AndWhileOtherArgsSucceed) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.exists(
        key_a{ "abc" }, key_a{ "def" }, key_a{ "ghi" },
        key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    // there is no resource a under 'jkl'

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" }, key_a{ "jkl" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);

    EXPECT_TRUE(sys.exists(
        /*key_a{"abc"}, key_a{"def"},*/ key_a{ "ghi" },
        /*key_b{"abc"},*/ key_b{ "def" }, key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Discard_FailQuietlyDueToNoProvider_AndWhileOtherArgsSucceed) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_TRUE(sys.exists(
        key_a{ "abc" }, key_a{ "def" }, key_a{ "ghi" },
        key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    // there is no provider for resource c

    sys.discard(key_a{ "def" }, key_b{ "abc" }, key_a{ "abc" }, key_c{ "abc" });

    EXPECT_EQ(sys.number<key_a>(), 1);
    EXPECT_EQ(sys.number<key_b>(), 2);
    ASSERT_EQ(sys.number<key_c>(), 0);

    EXPECT_TRUE(sys.exists(
        /*key_a{"abc"}, key_a{"def"},*/ key_a{ "ghi" },
        /*key_b{"abc"},*/ key_b{ "def" }, key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_Discard_ZeroArgsEdgeCase) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);

    ASSERT_TRUE(sys.exists(
        key_a{ "abc" }, key_a{ "def" }, key_a{ "ghi" },
        key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    sys.discard();

    EXPECT_EQ(sys.number<key_a>(), 3);
    EXPECT_EQ(sys.number<key_b>(), 3);

    EXPECT_TRUE(sys.exists(
        key_a{ "abc" }, key_a{ "def" }, key_a{ "ghi" },
        key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));
}

TEST_F(QuerySystemTests, System_DiscardAll_ForAllProviders) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_FALSE(sys.called_do_discard_all);

    sys.discard_all();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_TRUE(sys.called_do_discard_all);
}

TEST_F(QuerySystemTests, System_DiscardAll_ForSpecificProvider_ByQType) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_FALSE(sys.called_do_discard_all);

    sys.discard_all<test_qtype::a>();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 3);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(sys.exists(key_a{ "abc" }));
    EXPECT_FALSE(sys.exists(key_a{ "def" }));
    EXPECT_FALSE(sys.exists(key_a{ "ghi" }));
    EXPECT_TRUE(sys.exists(key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    EXPECT_FALSE(sys.called_do_discard_all);
}

TEST_F(QuerySystemTests, System_DiscardAll_ForSpecificProvider_ByKey) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_FALSE(sys.called_do_discard_all);

    sys.discard_all<key_a>();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 3);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_FALSE(sys.exists(key_a{ "abc" }));
    EXPECT_FALSE(sys.exists(key_a{ "def" }));
    EXPECT_FALSE(sys.exists(key_a{ "ghi" }));
    EXPECT_TRUE(sys.exists(key_b{ "abc" }, key_b{ "def" }, key_b{ "ghi" }));

    EXPECT_FALSE(sys.called_do_discard_all);
}

TEST_F(QuerySystemTests, System_Reset) {
    system_impl sys(dbg);

    sys.query(key_b{ "abc" });
    sys.query(key_b{ "def" });
    sys.query(key_b{ "ghi" });

    ASSERT_EQ(sys.number<key_a>(), 3);
    ASSERT_EQ(sys.number<key_b>(), 3);
    ASSERT_EQ(sys.number<key_c>(), 0);

    ASSERT_FALSE(sys.called_do_reset);

    sys.reset();

    EXPECT_EQ(sys.number<key_a>(), 0);
    EXPECT_EQ(sys.number<key_b>(), 0);
    EXPECT_EQ(sys.number<key_c>(), 0);

    EXPECT_TRUE(sys.called_do_reset);
}

