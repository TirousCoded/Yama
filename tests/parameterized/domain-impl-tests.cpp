

#include "domain-impl-tests.h"

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/parcel.h>


using namespace yama::string_literals;


GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DomainImplTests);


TEST_P(DomainImplTests, YamaParcelIsPreinstalled) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_P(DomainImplTests, Builtins) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    const auto _None = dm->load("yama:None"_str);
    const auto _Int = dm->load("yama:Int"_str);
    const auto _UInt = dm->load("yama:UInt"_str);
    const auto _Float = dm->load("yama:Float"_str);
    const auto _Bool = dm->load("yama:Bool"_str);
    const auto _Char = dm->load("yama:Char"_str);

    ASSERT_TRUE(_None);
    ASSERT_TRUE(_Int);
    ASSERT_TRUE(_UInt);
    ASSERT_TRUE(_Float);
    ASSERT_TRUE(_Bool);
    ASSERT_TRUE(_Char);

    EXPECT_EQ(_None, dm->load_none());
    EXPECT_EQ(_Int, dm->load_int());
    EXPECT_EQ(_UInt, dm->load_uint());
    EXPECT_EQ(_Float, dm->load_float());
    EXPECT_EQ(_Bool, dm->load_bool());
    EXPECT_EQ(_Char, dm->load_char());

    EXPECT_TRUE(_None->complete());
    EXPECT_TRUE(_Int->complete());
    EXPECT_TRUE(_UInt->complete());
    EXPECT_TRUE(_Float->complete());
    EXPECT_TRUE(_Bool->complete());
    EXPECT_TRUE(_Char->complete());

    EXPECT_EQ(_None->fullname(), "yama:None"_str);
    EXPECT_EQ(_Int->fullname(), "yama:Int"_str);
    EXPECT_EQ(_UInt->fullname(), "yama:UInt"_str);
    EXPECT_EQ(_Float->fullname(), "yama:Float"_str);
    EXPECT_EQ(_Bool->fullname(), "yama:Bool"_str);
    EXPECT_EQ(_Char->fullname(), "yama:Char"_str);

    EXPECT_EQ(_None->kind(), yama::kind::primitive);
    EXPECT_EQ(_Int->kind(), yama::kind::primitive);
    EXPECT_EQ(_UInt->kind(), yama::kind::primitive);
    EXPECT_EQ(_Float->kind(), yama::kind::primitive);
    EXPECT_EQ(_Bool->kind(), yama::kind::primitive);
    EXPECT_EQ(_Char->kind(), yama::kind::primitive);

    EXPECT_EQ(_None->ptype(), std::make_optional(yama::ptype::none));
    EXPECT_EQ(_Int->ptype(), std::make_optional(yama::ptype::int0));
    EXPECT_EQ(_UInt->ptype(), std::make_optional(yama::ptype::uint));
    EXPECT_EQ(_Float->ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(_Bool->ptype(), std::make_optional(yama::ptype::bool0));
    EXPECT_EQ(_Char->ptype(), std::make_optional(yama::ptype::char0));
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

TEST_P(DomainImplTests, Install_PriorToFirstInstall) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_P(DomainImplTests, Install_EmptyBatch) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_TRUE(dm->install(yama::install_batch{}));

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));

    ASSERT_TRUE(dm->install(yama::install_batch{})); // works w/ multiple installs

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("yama"_str));
}

TEST_P(DomainImplTests, Install_NoDeps) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Deps) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InvalidParcel_SelfNameDepNameConflict) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InstallNameConflict) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_MissingDepMapping) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_InstallName) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_InstallName_InstallNameRefersToAlreadyInstalledParcel) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_DepName) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_MappedTo) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Install_Fail_DepGraphCycle) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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


    // test_compiler0 always fails compilation, as its purpose is solely to launder in run_test_asserts_fn
    // to perform the behaviour of the unit test using the compiler services object

    class test_compiler0 final : public yama::compiler {
    public:
        using run_test_asserts_fn_t = void(*)(test_compiler0& host, yama::res<yama::compiler_services> services);


        run_test_asserts_fn_t run_test_asserts_fn;

        std::shared_ptr<yama::dsignal_debug> dsignal_dbg;

        bool was_invoked = false;

        std::shared_ptr<test_parcel1> parcel1;

        // these are for testing compiler services loading impl
        std::optional<yama::type> result_f, result_a, result_b, result_c;


        test_compiler0(run_test_asserts_fn_t run_test_asserts_fn, std::shared_ptr<yama::dsignal_debug> dsignal_dbg, std::shared_ptr<test_parcel1> parcel1)
            : compiler(),
            run_test_asserts_fn(run_test_asserts_fn),
            dsignal_dbg(dsignal_dbg),
            parcel1(parcel1) {}

        std::optional<yama::compile_result> compile(
            yama::res<yama::compiler_services> services,
            const taul::source_code& src,
            const yama::import_path& src_import_path) {
            was_invoked = true;
            run_test_asserts_fn(*this, services);
            return std::nullopt; // always fail
        }
    };
}

TEST_P(DomainImplTests, Import) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Import_Fail_ModuleNotFound_NoHead_WithNoRelativePath) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import(""_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_P(DomainImplTests, Import_Fail_ModuleNotFound_NoHead_WithRelativePath) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import(".a.b.c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_P(DomainImplTests, Import_Fail_ModuleNotFound_BadHead) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    const auto parcel1 = yama::make_res<test_parcel1>();

    ASSERT_FALSE(parcel1->last_relative_path);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1);

    ASSERT_TRUE(dm->install(std::move(ib)));


    ASSERT_FALSE(dm->import("bad.a.b.c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::import_module_not_found), 1);
}

TEST_P(DomainImplTests, Import_Fail_ModuleNotFound_BadRelativePath) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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

TEST_P(DomainImplTests, Import_Fail_InvalidModule_StaticVerificationFailure) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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


TEST_P(DomainImplTests, Load) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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
        EXPECT_TRUE(result_f->complete());
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
        EXPECT_TRUE(result_a->complete());
        EXPECT_EQ(result_a->fullname(), "p.abc:a"_str);
        EXPECT_EQ(result_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_a->callsig());
        EXPECT_EQ(result_a->consts().size(), 0);
    }

    if (result_b) {
        EXPECT_TRUE(result_b->complete());
        EXPECT_EQ(result_b->fullname(), "p.abc:b"_str);
        EXPECT_EQ(result_b->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_b->callsig());
        EXPECT_EQ(result_b->consts().size(), 0);
    }

    if (result_c) {
        EXPECT_TRUE(result_c->complete());
        EXPECT_EQ(result_c->fullname(), "p.abc:c"_str);
        EXPECT_EQ(result_c->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_c->callsig());
        EXPECT_EQ(result_c->consts().size(), 0);
    }
}

TEST_P(DomainImplTests, Load_DisambiguationViaImportPaths) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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
        EXPECT_TRUE(result_A_a->complete());
        EXPECT_EQ(result_A_a->fullname(), "A.abc:a"_str);
        EXPECT_EQ(result_A_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_A_a->callsig());
        EXPECT_EQ(result_A_a->consts().size(), 0);
    }
    if (result_B_a) {
        EXPECT_TRUE(result_B_a->complete());
        EXPECT_EQ(result_B_a->fullname(), "B.abc:a"_str);
        EXPECT_EQ(result_B_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_B_a->callsig());
        EXPECT_EQ(result_B_a->consts().size(), 0);
    }
}

TEST_P(DomainImplTests, Load_FailDueToLoadingError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

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


TEST_P(DomainImplTests, QuickAccessLoadMethods) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_none(), dm->load("yama:None"_str).value());
    EXPECT_EQ(dm->load_int(), dm->load("yama:Int"_str).value());
    EXPECT_EQ(dm->load_uint(), dm->load("yama:UInt"_str).value());
    EXPECT_EQ(dm->load_float(), dm->load("yama:Float"_str).value());
    EXPECT_EQ(dm->load_bool(), dm->load("yama:Bool"_str).value());
    EXPECT_EQ(dm->load_char(), dm->load("yama:Char"_str).value());
}


TEST_P(DomainImplTests, CompilerServices_Import) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::res<yama::compiler_services> services) {
        const auto abc_1 = services->import(yama::import_path::parse(services->env(), "other.a.b.c"_str).value());

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".a.b.c"_str);
        
        const auto def_1 = services->import(yama::import_path::parse(services->env(), "other.d.e.f"_str).value());

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".d.e.f"_str);

        const auto root_1 = services->import(yama::import_path::parse(services->env(), "other"_str).value()); // root

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ""_str);

        const auto selfstuff_1 = services->import(yama::import_path::parse(services->env(), "self.selfstuff"_str).value()); // via self-name, not from other

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ""_str); // <- accessed self-name, not other

        // lookup of memoized will NOT result in parcel import method call
        host.parcel1->last_relative_path.reset();

        const auto abc_2 = services->import(yama::import_path::parse(services->env(), "other.a.b.c"_str).value()); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);

        const auto def_2 = services->import(yama::import_path::parse(services->env(), "other.d.e.f"_str).value()); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);

        const auto root_2 = services->import(yama::import_path::parse(services->env(), "other"_str).value()); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);

        const auto selfstuff_2 = services->import(yama::import_path::parse(services->env(), "self.selfstuff"_str).value()); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path); // should never even have accessed other


        ASSERT_TRUE(abc_1);
        ASSERT_TRUE(def_1);
        ASSERT_TRUE(root_1);
        ASSERT_TRUE(selfstuff_1);
        ASSERT_TRUE(abc_2);
        ASSERT_TRUE(def_2);
        ASSERT_TRUE(root_2);
        ASSERT_TRUE(selfstuff_2);

        ASSERT_TRUE(abc_1->result.holds_module());
        ASSERT_TRUE(def_1->result.holds_module());
        ASSERT_TRUE(root_1->result.holds_module());
        ASSERT_TRUE(selfstuff_1->result.holds_module());
        ASSERT_TRUE(abc_2->result.holds_module());
        ASSERT_TRUE(def_2->result.holds_module());
        ASSERT_TRUE(root_2->result.holds_module());
        ASSERT_TRUE(selfstuff_2->result.holds_module());

        ASSERT_EQ(*abc_1->result.get_module(), modinf_abc);
        ASSERT_EQ(*def_1->result.get_module(), modinf_def);
        ASSERT_EQ(*root_1->result.get_module(), modinf_root);
        ASSERT_EQ(*selfstuff_1->result.get_module(), modinf_selfstuff);
        ASSERT_EQ(*abc_2->result.get_module(), modinf_abc);
        ASSERT_EQ(*def_2->result.get_module(), modinf_def);
        ASSERT_EQ(*root_2->result.get_module(), modinf_root);
        ASSERT_EQ(*selfstuff_2->result.get_module(), modinf_selfstuff);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel3>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(config, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "yama"_str, "yama"_str)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto result = dm->import("parcel2"_str); // trigger compiler, expect failure
    ASSERT_TRUE(our_compiler->was_invoked);
    ASSERT_FALSE(result);
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

// TODO: maybe add unit tests for more precisely testing module-not-found due to the HEAD
//       being bad, or the RELATIVE PATH being bad

TEST_P(DomainImplTests, CompilerServices_Import_Fail_ModuleNotFound) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::res<yama::compiler_services> services) {
        ASSERT_FALSE(services->import(yama::import_path::parse(services->env(), "other.does.not.exist"_str).value()));

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_module_not_found), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel3>();
    std::cerr << "-- " << parcel2->id() << "\n";
    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(config, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "yama"_str, "yama"_str)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto result = dm->import("parcel2"_str); // trigger compiler, expect failure
    ASSERT_TRUE(our_compiler->was_invoked);
    ASSERT_FALSE(result);
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

TEST_P(DomainImplTests, CompilerServices_Import_Fail_InvalidModule_StaticVerificationFailure) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::res<yama::compiler_services> services) {
        ASSERT_FALSE(services->import(yama::import_path::parse(services->env(), "other.bad"_str).value()));

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".bad"_str);

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_invalid_module), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel3>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(config, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "yama"_str, "yama"_str)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto result = dm->import("parcel2"_str); // trigger compiler, expect failure
    ASSERT_TRUE(our_compiler->was_invoked);
    ASSERT_FALSE(result);
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

