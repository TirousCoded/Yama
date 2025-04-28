

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/parcel.h>


using namespace yama::string_literals;


class DomainTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    std::shared_ptr<yama::domain> dm;


protected:
    void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
        dm = std::make_shared<yama::domain>(dbg);
    }

    void TearDown() override final {
        //
    }
};


TEST_F(DomainTests, YamaParcelIsPreinstalled) {
    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_F(DomainTests, Builtins) {
    const auto _None = dm->load("yama:None"_str);
    const auto _Int = dm->load("yama:Int"_str);
    const auto _UInt = dm->load("yama:UInt"_str);
    const auto _Float = dm->load("yama:Float"_str);
    const auto _Bool = dm->load("yama:Bool"_str);
    const auto _Char = dm->load("yama:Char"_str);
    const auto _Type = dm->load("yama:Type"_str);

    ASSERT_TRUE(_None);
    ASSERT_TRUE(_Int);
    ASSERT_TRUE(_UInt);
    ASSERT_TRUE(_Float);
    ASSERT_TRUE(_Bool);
    ASSERT_TRUE(_Char);
    ASSERT_TRUE(_Type);

    EXPECT_EQ(_None, dm->none_type());
    EXPECT_EQ(_Int, dm->int_type());
    EXPECT_EQ(_UInt, dm->uint_type());
    EXPECT_EQ(_Float, dm->float_type());
    EXPECT_EQ(_Bool, dm->bool_type());
    EXPECT_EQ(_Char, dm->char_type());
    EXPECT_EQ(_Type, dm->type_type());

    EXPECT_EQ(_None->fullname(), "yama:None"_str);
    EXPECT_EQ(_Int->fullname(), "yama:Int"_str);
    EXPECT_EQ(_UInt->fullname(), "yama:UInt"_str);
    EXPECT_EQ(_Float->fullname(), "yama:Float"_str);
    EXPECT_EQ(_Bool->fullname(), "yama:Bool"_str);
    EXPECT_EQ(_Char->fullname(), "yama:Char"_str);
    EXPECT_EQ(_Type->fullname(), "yama:Type"_str);

    EXPECT_EQ(_None->kind(), yama::kind::primitive);
    EXPECT_EQ(_Int->kind(), yama::kind::primitive);
    EXPECT_EQ(_UInt->kind(), yama::kind::primitive);
    EXPECT_EQ(_Float->kind(), yama::kind::primitive);
    EXPECT_EQ(_Bool->kind(), yama::kind::primitive);
    EXPECT_EQ(_Char->kind(), yama::kind::primitive);
    EXPECT_EQ(_Type->kind(), yama::kind::primitive);

    EXPECT_EQ(_None->ptype(), std::make_optional(yama::ptype::none));
    EXPECT_EQ(_Int->ptype(), std::make_optional(yama::ptype::int0));
    EXPECT_EQ(_UInt->ptype(), std::make_optional(yama::ptype::uint));
    EXPECT_EQ(_Float->ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(_Bool->ptype(), std::make_optional(yama::ptype::bool0));
    EXPECT_EQ(_Char->ptype(), std::make_optional(yama::ptype::char0));
    EXPECT_EQ(_Type->ptype(), std::make_optional(yama::ptype::type));
}


namespace {
    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;

        yama::parcel_metadata& get_md() {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }


        test_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str&) override final { return std::nullopt; }
    };
}

TEST_F(DomainTests, Install_PriorToFirstInstall) {
    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_F(DomainTests, Install_EmptyBatch) {
    ASSERT_TRUE(dm->install(yama::install_batch{}));

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));

    ASSERT_TRUE(dm->install(yama::install_batch{})); // works w/ multiple installs

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_F(DomainTests, Install_NoDeps) {
    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();
    auto cc = yama::make_res<test_parcel>();
    auto dd = yama::make_res<test_parcel>();

    {
        yama::install_batch b{};
        b
            .install("a"_str, aa)
            .install("b"_str, bb)
            .map_dep("a"_str, "yama"_str, "yama"_str)
            .map_dep("b"_str, "yama"_str, "yama"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/out existing installs

        ASSERT_EQ(dm->install_count(), 3);
        ASSERT_TRUE(dm->is_installed("yama"_str));
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_FALSE(dm->is_installed("c"_str));
        ASSERT_FALSE(dm->is_installed("d"_str));
    }
    {
        yama::install_batch b{};
        b
            .install("c"_str, cc)
            .install("d"_str, dd)
            .map_dep("c"_str, "yama"_str, "yama"_str)
            .map_dep("d"_str, "yama"_str, "yama"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/ existing installs

        ASSERT_EQ(dm->install_count(), 5);
        ASSERT_TRUE(dm->is_installed("yama"_str));
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_TRUE(dm->is_installed("c"_str));
        ASSERT_TRUE(dm->is_installed("d"_str));
    }
}

TEST_F(DomainTests, Install_Deps) {
    // this unit test covers both deps between parcels in same install batch, and
    // deps between parcels where one is in batch, and the other is fully installed

    test_parcel bbb{};
    bbb.get_md().add_dep_name("A"_str);
    
    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();
    bb->get_md().add_dep_name("A"_str);
    auto cc = yama::make_res<test_parcel>();
    cc->get_md().add_dep_name("A"_str);
    cc->get_md().add_dep_name("B"_str);
    auto dd = yama::make_res<test_parcel>();
    dd->get_md().add_dep_name("A"_str);
    dd->get_md().add_dep_name("B"_str);
    dd->get_md().add_dep_name("C"_str);

    {
        yama::install_batch b{};
        b
            .install("a"_str, aa)
            .install("b"_str, bb)
            .map_dep("a"_str, "yama"_str, "yama"_str)
            .map_dep("b"_str, "yama"_str, "yama"_str)
            .map_dep("b"_str, "A"_str, "a"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/out existing installs

        ASSERT_EQ(dm->install_count(), 3);
        ASSERT_TRUE(dm->is_installed("yama"_str));
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_FALSE(dm->is_installed("c"_str));
        ASSERT_FALSE(dm->is_installed("d"_str));
    }
    {
        yama::install_batch b{};
        b
            .install("c"_str, cc)
            .install("d"_str, dd)
            .map_dep("c"_str, "yama"_str, "yama"_str)
            .map_dep("c"_str, "A"_str, "a"_str)
            .map_dep("c"_str, "B"_str, "b"_str)
            .map_dep("d"_str, "yama"_str, "yama"_str)
            .map_dep("d"_str, "A"_str, "a"_str)
            .map_dep("d"_str, "B"_str, "b"_str)
            .map_dep("d"_str, "C"_str, "c"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/ existing installs

        ASSERT_EQ(dm->install_count(), 5);
        ASSERT_TRUE(dm->is_installed("yama"_str));
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_TRUE(dm->is_installed("c"_str));
        ASSERT_TRUE(dm->is_installed("d"_str));
    }
}

class test_invalid_parcel final : public yama::parcel {
public:
    std::optional<yama::parcel_metadata> md;


    test_invalid_parcel() = default;


    const yama::parcel_metadata& metadata() override final {
        if (!md) md = yama::parcel_metadata{ "self"_str, { "self"_str } }; // <- invalid! dep-name is self-name
        return *md;
    }
    std::optional<yama::import_result> import(const yama::str&) override final { return std::nullopt; }
};

TEST_F(DomainTests, Install_Fail_InvalidParcel_SelfNameDepNameConflict) {
    auto aa = yama::make_res<test_invalid_parcel>();

    yama::install_batch ib{};
    ib
        .install("a"_str, aa);

    ASSERT_FALSE(dm->install(std::move(ib))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_parcel), 1);
}

TEST_F(DomainTests, Install_Fail_InstallNameConflict) {
    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .map_dep("a"_str, "yama"_str, "yama"_str);

    ASSERT_TRUE(dm->install(std::move(b0)));

    ASSERT_EQ(dm->install_count(), 2);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_TRUE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    yama::install_batch b1{};
    b1
        .install("a"_str, bb)
        .map_dep("a"_str, "yama"_str, "yama"_str);

    ASSERT_FALSE(dm->install(std::move(b1))); // error

    ASSERT_EQ(dm->install_count(), 2);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_TRUE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_install_name_conflict), 1);
}

TEST_F(DomainTests, Install_Fail_MissingDepMapping) {
    auto aa = yama::make_res<test_parcel>();
    aa->get_md().add_dep_name("other"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa) // no dep mapping for other
        .map_dep("a"_str, "yama"_str, "yama"_str);

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_missing_dep_mapping), 1);
}

TEST_F(DomainTests, Install_Fail_InvalidDepMapping_InstallName) {
    auto aa = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("b"_str, "A"_str, "a"_str); // b is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_F(DomainTests, Install_Fail_InvalidDepMapping_InstallName_InstallNameRefersToAlreadyInstalledParcel) {
    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("b"_str, bb)
        .map_dep("b"_str, "yama"_str, "yama"_str);

    ASSERT_TRUE(dm->install(std::move(b0)));

    yama::install_batch b1{};
    b1
        .install("a"_str, aa)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("b"_str, "A"_str, "a"_str); // b is valid install name, but NOT to anything in the batch

    ASSERT_FALSE(dm->install(std::move(b1))); // error

    ASSERT_EQ(dm->install_count(), 2);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));
    ASSERT_TRUE(dm->is_installed("b"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_F(DomainTests, Install_Fail_InvalidDepMapping_DepName) {
    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .install("b"_str, bb)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("b"_str, "yama"_str, "yama"_str)
        .map_dep("b"_str, "A"_str, "a"_str); // A is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_F(DomainTests, Install_Fail_InvalidDepMapping_MappedTo) {
    auto aa = yama::make_res<test_parcel>();
    aa->get_md().add_dep_name("B"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("a"_str, "B"_str, "b"_str); // b is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_F(DomainTests, Install_Fail_DepGraphCycle) {
    auto aa = yama::make_res<test_parcel>();
    aa->get_md().add_dep_name("other"_str);
    auto bb = yama::make_res<test_parcel>();
    bb->get_md().add_dep_name("other"_str);
    auto cc = yama::make_res<test_parcel>();
    cc->get_md().add_dep_name("other"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .install("b"_str, bb)
        .install("c"_str, cc)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("a"_str, "other"_str, "b"_str)
        .map_dep("b"_str, "yama"_str, "yama"_str)
        .map_dep("b"_str, "other"_str, "c"_str)
        .map_dep("c"_str, "yama"_str, "yama"_str)
        .map_dep("c"_str, "other"_str, "a"_str);

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
    ASSERT_FALSE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));
    ASSERT_FALSE(dm->is_installed("c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_dep_graph_cycle), 1);
}


namespace {
    // the type 'bad' will fail static verification during import
    
    static const auto bad_consts =
    yama::const_table_info()
    .add_primitive_type("self:a"_str)
    .add_primitive_type("self:b"_str)
    .add_primitive_type("self:c"_str);
    static const auto bad_callsig = yama::make_callsig_info({ 0, 7, 2 }, 1); // <- link index 7 is out-of-bounds!
    yama::type_info bad_info{
        .unqualified_name = "bad"_str,
        .consts = bad_consts,
        .info = yama::function_info{
            .callsig = bad_callsig,
            .call_fn = yama::noop_call_fn,
            .max_locals = 4,
        },
    };


    // give each of these module_info different internal structure so we can value-compare
    // to check that importing worked

    static const yama::module_info modinf_abc =
        yama::module_factory()
        .add_primitive_type("modinf_abc_1"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_abc_2"_str, yama::const_table_info{}, yama::ptype::int0)
        .done();

    static const yama::module_info modinf_def =
        yama::module_factory()
        .add_primitive_type("modinf_def_1"_str, yama::const_table_info{}, yama::ptype::int0)
        .done();

    static const yama::module_info modinf_root =
        yama::module_factory()
        .add_primitive_type("modinf_root_1"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_root_2"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_root_3"_str, yama::const_table_info{}, yama::ptype::int0)
        .done();
    
    static const yama::module_info modinf_selfstuff =
        yama::module_factory()
        .add_primitive_type("modinf_selfstuff_1"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_selfstuff_2"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_selfstuff_3"_str, yama::const_table_info{}, yama::ptype::int0)
        .add_primitive_type("modinf_selfstuff_4"_str, yama::const_table_info{}, yama::ptype::int0)
        .done();


    class test_parcel1 final : public yama::parcel {
    public:
        std::optional<yama::str> last_relative_path = {};

        std::optional<yama::parcel_metadata> md;


        test_parcel1() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, {} };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            last_relative_path = relative_path;
            if (relative_path == ".a.b.c"_str) {
                return yama::make_res<yama::module_info>(modinf_abc);
            }
            else if (relative_path == ".d.e.f"_str) {
                return yama::make_res<yama::module_info>(modinf_def);
            }
            else if (relative_path == ""_str) {
                return yama::make_res<yama::module_info>(modinf_root);
            }
            else if (relative_path == ".bad"_str) {
                // IMPORTANT: not to be confused w/ '.b.a.d' which we're using to test fail due to
                //            attempting to import a *nonexistent* module
                // this returns an invalid module to test import failure due to static verif fail
                yama::module_factory mf{};
                mf.add_type(yama::type_info(bad_info)); // <- will cause static verif fail
                return yama::make_res<yama::module_info>(mf.done());
            }
            else return std::nullopt;
        }
    };


    yama::type_info a_info{
        .unqualified_name = "a"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info b_info{
        .unqualified_name = "b"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info c_info{
        .unqualified_name = "c"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info d_info{
        .unqualified_name = "d"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    static const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("self.abc:a"_str)
        .add_primitive_type("self.abc:b"_str)
        .add_primitive_type("self.abc:c"_str);
    static const auto f_callsig = yama::make_callsig_info({ 0, 1, 2 }, 1);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = f_callsig,
            .call_fn = yama::noop_call_fn,
            .max_locals = 4,
        },
    };


    class test_parcel2 final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;

        yama::module_factory mf; // expose this so test code can setup how it wants
        std::shared_ptr<yama::module_info> m;


        test_parcel2() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, {} };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (relative_path != ".abc"_str) return std::nullopt;
            if (!m) m = std::make_shared<yama::module_info>(mf.done());
            return yama::res(m);
        }
    };


    class test_parcel3 final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;


        test_parcel3() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str, "other"_str } }; // <- other expected to be parcel1
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            // root module exists to trigger our compiler, define self.selfstuff (for testing self-names), and carry
            // an 'other' dep mapping
            if (relative_path == ""_str) {
                return taul::source_code{};
            }
            else if (relative_path == ".selfstuff"_str) {
                return yama::make_res<yama::module_info>(modinf_selfstuff);
            }
            else return std::nullopt;
        }
    };
}

TEST_F(DomainTests, Import) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    const auto abc_1 = dm->import("parcel1.a.b.c"_str);
    
    ASSERT_TRUE(parcel1->last_relative_path);
    ASSERT_EQ(*parcel1->last_relative_path, ".a.b.c"_str);

    const auto def_1 = dm->import("parcel1.d.e.f"_str);
    
    ASSERT_TRUE(parcel1->last_relative_path);
    ASSERT_EQ(*parcel1->last_relative_path, ".d.e.f"_str);
    
    const auto root_1 = dm->import("parcel1"_str); // root
    
    ASSERT_TRUE(parcel1->last_relative_path);
    ASSERT_EQ(*parcel1->last_relative_path, ""_str);

    // lookup of memoized will NOT result in parcel import method call
    parcel1->last_relative_path.reset();

    const auto abc_2 = dm->import("parcel1.a.b.c"_str); // test imports memoize
    
    ASSERT_FALSE(parcel1->last_relative_path);

    const auto def_2 = dm->import("parcel1.d.e.f"_str); // test imports memoize
    
    ASSERT_FALSE(parcel1->last_relative_path);
    
    const auto root_2 = dm->import("parcel1"_str); // test imports memoize
    
    ASSERT_FALSE(parcel1->last_relative_path);


    ASSERT_TRUE(abc_1);
    ASSERT_TRUE(def_1);
    ASSERT_TRUE(root_1);
    ASSERT_TRUE(abc_2);
    ASSERT_TRUE(def_2);
    ASSERT_TRUE(root_2);

    ASSERT_EQ(abc_1, abc_2);
    ASSERT_EQ(def_1, def_2);
    ASSERT_EQ(root_1, root_2);

    ASSERT_NE(abc_1, def_1);
    ASSERT_NE(abc_1, root_1);
    ASSERT_NE(def_1, root_1);
}

TEST_F(DomainTests, Import_Fail_ModuleNotFound_NoHead_WithNoRelativePath) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import(""_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_F(DomainTests, Import_Fail_ModuleNotFound_NoHead_WithRelativePath) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import(".a.b.c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_F(DomainTests, Import_Fail_ModuleNotFound_BadHead) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import("bad.a.b.c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_F(DomainTests, Import_Fail_ModuleNotFound_BadRelativePath) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import("parcel1.b.a.d"_str));

    ASSERT_TRUE(parcel1->last_relative_path);
    ASSERT_EQ(*parcel1->last_relative_path, ".b.a.d"_str);

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_F(DomainTests, Import_Fail_InvalidModule_StaticVerificationFailure) {
    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import("parcel1.bad"_str));

    ASSERT_TRUE(parcel1->last_relative_path);
    ASSERT_EQ(*parcel1->last_relative_path, ".bad"_str);

    ASSERT_GE(dbg->count(yama::dsignal::import_invalid_module), 1);
}


TEST_F(DomainTests, Load) {
    auto our_parcel = yama::make_res<test_parcel2>();
    our_parcel->mf
        .add_type(yama::type_info(f_info))
        .add_type(yama::type_info(a_info))
        .add_type(yama::type_info(b_info))
        .add_type(yama::type_info(c_info));

    yama::install_batch ib{};
    ib.install("p"_str, our_parcel);
    ASSERT_TRUE(dm->install(std::move(ib)));


    // load w/ 'p.abc:' qualifier
    const auto result_f = dm->load("p.abc:f"_str);
    const auto result_a = dm->load("p.abc:a"_str);
    const auto result_b = dm->load("p.abc:b"_str);
    const auto result_c = dm->load("p.abc:c"_str);

    EXPECT_TRUE(result_f);
    EXPECT_TRUE(result_a);
    EXPECT_TRUE(result_b);
    EXPECT_TRUE(result_c);

    if (result_f) {
        EXPECT_EQ(result_f->fullname(), "p.abc:f"_str);
        EXPECT_EQ(result_f->kind(), yama::kind::function);

        EXPECT_TRUE(result_f->callsig());
        if (result_f->callsig()) EXPECT_EQ(result_f->callsig().value(), yama::callsig(f_callsig, result_f->consts()));

        EXPECT_EQ(result_f->consts().size(), 3);
        if (result_f->consts().size() == 3) {
            EXPECT_EQ(result_f->consts().type(0), result_a);
            EXPECT_EQ(result_f->consts().type(1), result_b);
            EXPECT_EQ(result_f->consts().type(2), result_c);
        }
    }

    if (result_a) {
        EXPECT_EQ(result_a->fullname(), "p.abc:a"_str);
        EXPECT_EQ(result_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_a->callsig());
        EXPECT_EQ(result_a->consts().size(), 0);
    }

    if (result_b) {
        EXPECT_EQ(result_b->fullname(), "p.abc:b"_str);
        EXPECT_EQ(result_b->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_b->callsig());
        EXPECT_EQ(result_b->consts().size(), 0);
    }

    if (result_c) {
        EXPECT_EQ(result_c->fullname(), "p.abc:c"_str);
        EXPECT_EQ(result_c->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_c->callsig());
        EXPECT_EQ(result_c->consts().size(), 0);
    }
}

TEST_F(DomainTests, Load_DisambiguationViaImportPaths) {
    auto our_parcel_A = yama::make_res<test_parcel2>();
    auto our_parcel_B = yama::make_res<test_parcel2>();
    our_parcel_A->mf.add_type(yama::type_info(a_info));
    our_parcel_B->mf.add_type(yama::type_info(a_info));

    yama::install_batch ib{};
    ib.install("A"_str, our_parcel_A);
    ib.install("B"_str, our_parcel_B);
    ASSERT_TRUE(dm->install(std::move(ib)));


    // same unqualified name 'a', but import paths disambiguate for us
    const auto result_A_a = dm->load("A.abc:a"_str);
    const auto result_B_a = dm->load("B.abc:a"_str);

    EXPECT_TRUE(result_A_a);
    EXPECT_TRUE(result_B_a);

    // types are not the same
    EXPECT_NE(result_A_a.value(), result_B_a.value());

    // types are as expected
    if (result_A_a) {
        EXPECT_EQ(result_A_a->fullname(), "A.abc:a"_str);
        EXPECT_EQ(result_A_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_A_a->callsig());
        EXPECT_EQ(result_A_a->consts().size(), 0);
    }
    if (result_B_a) {
        EXPECT_EQ(result_B_a->fullname(), "B.abc:a"_str);
        EXPECT_EQ(result_B_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_B_a->callsig());
        EXPECT_EQ(result_B_a->consts().size(), 0);
    }
}

TEST_F(DomainTests, Load_FailDueToLoadingError) {
    // f will fail loading due to type b not being available

    auto our_parcel = yama::make_res<test_parcel2>();
    our_parcel->mf
        .add_type(yama::type_info(f_info))
        .add_type(yama::type_info(a_info))
        //.add_type(yama::type_info(b_info))
        .add_type(yama::type_info(c_info));

    yama::install_batch ib{};
    ib.install("p"_str, our_parcel);
    ASSERT_TRUE(dm->install(std::move(ib)));


    // we don't care about the details of types a and c

    ASSERT_FALSE(dm->load("p.abc:f"_str));
    ASSERT_TRUE(dm->load("p.abc:a"_str));
    ASSERT_TRUE(dm->load("p.abc:c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::load_type_not_found), 1);
}


TEST_F(DomainTests, QuickAccessLoadMethods) {
    EXPECT_EQ(dm->none_type(), dm->load("yama:None"_str).value());
    EXPECT_EQ(dm->int_type(), dm->load("yama:Int"_str).value());
    EXPECT_EQ(dm->uint_type(), dm->load("yama:UInt"_str).value());
    EXPECT_EQ(dm->float_type(), dm->load("yama:Float"_str).value());
    EXPECT_EQ(dm->bool_type(), dm->load("yama:Bool"_str).value());
    EXPECT_EQ(dm->char_type(), dm->load("yama:Char"_str).value());
    EXPECT_EQ(dm->type_type(), dm->load("yama:Type"_str).value());
}

