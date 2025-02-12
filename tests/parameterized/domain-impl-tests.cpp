

#include "domain-impl-tests.h"

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>


using namespace yama::string_literals;


GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DomainImplTests);


yama::type_info a_info{
    .fullname = "a"_str,
    .consts = {},
    .info = yama::primitive_info{
        .ptype = yama::ptype::bool0,
    },
};
yama::type_info b_info{
    .fullname = "b"_str,
    .consts = {},
    .info = yama::primitive_info{
        .ptype = yama::ptype::bool0,
    },
};
yama::type_info c_info{
    .fullname = "c"_str,
    .consts = {},
    .info = yama::primitive_info{
        .ptype = yama::ptype::bool0,
    },
};
yama::type_info d_info{
    .fullname = "d"_str,
    .consts = {},
    .info = yama::primitive_info{
        .ptype = yama::ptype::bool0,
    },
};

static const auto f_consts =
yama::const_table_info()
.add_primitive_type("a"_str)
.add_primitive_type("b"_str)
.add_primitive_type("c"_str);
static const auto f_callsig = yama::make_callsig_info({ 0, 1, 2 }, 1);
yama::type_info f_info{
    .fullname = "f"_str,
    .consts = f_consts,
    .info = yama::function_info{
        .callsig = f_callsig,
        .call_fn = yama::noop_call_fn,
        .max_locals = 4,
    },
};

// the type 'bad' will fail static verification during upload

static const auto bad_consts =
yama::const_table_info()
.add_primitive_type("a"_str)
.add_primitive_type("b"_str)
.add_primitive_type("c"_str);
static const auto bad_callsig = yama::make_callsig_info({ 0, 7, 2 }, 1); // <- link index 7 is out-of-bounds!
yama::type_info bad_info{
    .fullname = "bad"_str,
    .consts = bad_consts,
    .info = yama::function_info{
        .callsig = bad_callsig,
        .call_fn = yama::noop_call_fn,
        .max_locals = 4,
    },
};


// IMPORTANT:
//      these tests DO NOT cover get_mas AT ALL, and so each yama::domain impl must
//      test its proper functioning according to its own semantics

// IMPORTANT:
//      load tests presume that since type_instantiator is so well tested, that
//      so long as *basic* behaviour can be *broadly* ensured, that it can be presumed
//      that type_instantiator is the mechanism by which this behaviour is implemented,
//      such that its guarantees can be presumed to likewise apply here


TEST_P(DomainImplTests, Builtins) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    const auto _None = dm->load("None"_str);
    const auto _Int = dm->load("Int"_str);
    const auto _UInt = dm->load("UInt"_str);
    const auto _Float = dm->load("Float"_str);
    const auto _Bool = dm->load("Bool"_str);
    const auto _Char = dm->load("Char"_str);

    ASSERT_TRUE(_None);
    ASSERT_TRUE(_Int);
    ASSERT_TRUE(_UInt);
    ASSERT_TRUE(_Float);
    ASSERT_TRUE(_Bool);
    ASSERT_TRUE(_Char);

    EXPECT_TRUE(_None->complete());
    EXPECT_TRUE(_Int->complete());
    EXPECT_TRUE(_UInt->complete());
    EXPECT_TRUE(_Float->complete());
    EXPECT_TRUE(_Bool->complete());
    EXPECT_TRUE(_Char->complete());

    EXPECT_EQ(_None->fullname(), "None"_str);
    EXPECT_EQ(_Int->fullname(), "Int"_str);
    EXPECT_EQ(_UInt->fullname(), "UInt"_str);
    EXPECT_EQ(_Float->fullname(), "Float"_str);
    EXPECT_EQ(_Bool->fullname(), "Bool"_str);
    EXPECT_EQ(_Char->fullname(), "Char"_str);

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


class test_parcel final : public yama::parcel {
public:
    yama::dep_reqs deps_v;


    test_parcel() = default;


    const yama::dep_reqs& deps() override final { return deps_v; }
    std::shared_ptr<const yama::module_info> import(yama::parcel_services, yama::str) override final { return nullptr; }
};

TEST_P(DomainImplTests, Install_PriorToFirstInstall) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_EQ(dm->install_count(), 0);
}

TEST_P(DomainImplTests, Install_EmptyBatch) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_TRUE(dm->install(yama::install_batch{}));

    ASSERT_EQ(dm->install_count(), 0);

    ASSERT_TRUE(dm->install(yama::install_batch{})); // works w/ multiple installs

    ASSERT_EQ(dm->install_count(), 0);
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
            .install("b"_str, bb);

        ASSERT_TRUE(dm->install(std::move(b))); // w/out existing installs

        ASSERT_EQ(dm->install_count(), 2);
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_FALSE(dm->is_installed("c"_str));
        ASSERT_FALSE(dm->is_installed("d"_str));
    }
    {
        yama::install_batch b{};
        b
            .install("c"_str, cc)
            .install("d"_str, dd);

        ASSERT_TRUE(dm->install(std::move(b))); // w/ existing installs

        ASSERT_EQ(dm->install_count(), 4);
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

    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();
    bb->deps_v.add("A"_str);
    auto cc = yama::make_res<test_parcel>();
    cc->deps_v.add("A"_str);
    cc->deps_v.add("B"_str);
    auto dd = yama::make_res<test_parcel>();
    dd->deps_v.add("A"_str);
    dd->deps_v.add("B"_str);
    dd->deps_v.add("C"_str);

    {
        yama::install_batch b{};
        b
            .install("a"_str, aa)
            .install("b"_str, bb)
            .map_dep("b"_str, "A"_str, "a"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/out existing installs

        ASSERT_EQ(dm->install_count(), 2);
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_FALSE(dm->is_installed("c"_str));
        ASSERT_FALSE(dm->is_installed("d"_str));
    }
    {
        yama::install_batch b{};
        b
            .install("c"_str, cc)
            .map_dep("c"_str, "A"_str, "a"_str)
            .map_dep("c"_str, "B"_str, "b"_str)
            .install("d"_str, dd)
            .map_dep("d"_str, "A"_str, "a"_str)
            .map_dep("d"_str, "B"_str, "b"_str)
            .map_dep("d"_str, "C"_str, "c"_str);

        ASSERT_TRUE(dm->install(std::move(b))); // w/ existing installs

        ASSERT_EQ(dm->install_count(), 4);
        ASSERT_TRUE(dm->is_installed("a"_str));
        ASSERT_TRUE(dm->is_installed("b"_str));
        ASSERT_TRUE(dm->is_installed("c"_str));
        ASSERT_TRUE(dm->is_installed("d"_str));
    }
}

TEST_P(DomainImplTests, Install_Fail_InstallNameConflict) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("a"_str, aa);

    ASSERT_TRUE(dm->install(std::move(b0)));

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    yama::install_batch b1{};
    b1
        .install("a"_str, bb);

    ASSERT_FALSE(dm->install(std::move(b1))); // error

    ASSERT_EQ(dm->install_count(), 1);
    ASSERT_TRUE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_install_name_conflict), 1);
}

TEST_P(DomainImplTests, Install_Fail_MissingDepMapping) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();
    aa->deps_v.add("other"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa); // no dep mapping for other

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 0);
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_missing_dep_mapping), 1);
}

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_InstallName) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .map_dep("b"_str, "A"_str, "a"_str); // b is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 0);
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_InstallName_InstallNameRefersToAlreadyInstalledParcel) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();
    auto bb = yama::make_res<test_parcel>();

    yama::install_batch b0{};
    b0
        .install("b"_str, bb);

    ASSERT_TRUE(dm->install(std::move(b0)));

    yama::install_batch b1{};
    b1
        .install("a"_str, aa)
        .map_dep("b"_str, "A"_str, "a"_str); // b is valid install name, but NOT to anything in the batch

    ASSERT_FALSE(dm->install(std::move(b1))); // error

    ASSERT_EQ(dm->install_count(), 1);
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
        .map_dep("b"_str, "A"_str, "a"_str); // A is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 0);
    ASSERT_FALSE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_P(DomainImplTests, Install_Fail_InvalidDepMapping_MappedTo) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();
    aa->deps_v.add("B"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .map_dep("a"_str, "B"_str, "b"_str); // b is unknown

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 0);
    ASSERT_FALSE(dm->is_installed("a"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_invalid_dep_mapping), 1);
}

TEST_P(DomainImplTests, Install_Fail_DepGraphCycle) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto aa = yama::make_res<test_parcel>();
    aa->deps_v.add("other"_str);
    auto bb = yama::make_res<test_parcel>();
    bb->deps_v.add("other"_str);
    auto cc = yama::make_res<test_parcel>();
    cc->deps_v.add("other"_str);

    yama::install_batch b0{};
    b0
        .install("a"_str, aa)
        .install("b"_str, bb)
        .install("c"_str, cc)
        .map_dep("a"_str, "other"_str, "b"_str)
        .map_dep("b"_str, "other"_str, "c"_str)
        .map_dep("c"_str, "other"_str, "a"_str);

    ASSERT_FALSE(dm->install(std::move(b0))); // error

    ASSERT_EQ(dm->install_count(), 0);
    ASSERT_FALSE(dm->is_installed("a"_str));
    ASSERT_FALSE(dm->is_installed("b"_str));
    ASSERT_FALSE(dm->is_installed("c"_str));

    ASSERT_GE(dbg->count(yama::dsignal::install_dep_graph_cycle), 1);
}


class test_parcel1 final : public yama::parcel {
public:
    std::optional<yama::str> last_relative_path = {};

    std::optional<yama::dep_reqs> dr = {};
    std::shared_ptr<const yama::module_info> abc_m, def_m, root_m = {};


    test_parcel1() = default;


    const yama::dep_reqs& deps() override final {
        if (!dr) {
            dr = yama::dep_reqs();
        }
        return *dr;
    }

    std::shared_ptr<const yama::module_info> import(yama::parcel_services services, yama::str relative_path) override final {
        last_relative_path = relative_path;
        if (relative_path == ".a.b.c"_str) {
            if (!abc_m) {
                abc_m = std::make_shared<yama::module_info>(yama::module_factory().done());
            }
            return abc_m;
        }
        else if (relative_path == ".d.e.f"_str) {
            if (!def_m) {
                def_m = std::make_shared<yama::module_info>(yama::module_factory().done());
            }
            return def_m;
        }
        else if (relative_path == ""_str) {
            if (!root_m) {
                root_m = std::make_shared<yama::module_info>(yama::module_factory().done());
            }
            return root_m;
        }
        else return nullptr;
    }
};

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


TEST_P(DomainImplTests, Load) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_TRUE(dm->upload(yama::type_info(f_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(a_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(b_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(c_info)));


    const auto result_f = dm->load("f"_str);
    const auto result_a = dm->load("a"_str);
    const auto result_b = dm->load("b"_str);
    const auto result_c = dm->load("c"_str);

    EXPECT_TRUE(result_f);
    EXPECT_TRUE(result_a);
    EXPECT_TRUE(result_b);
    EXPECT_TRUE(result_c);

    if (result_f) {
        EXPECT_TRUE(result_f->complete());
        EXPECT_EQ(result_f->fullname(), "f"_str);
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
        EXPECT_EQ(result_a->fullname(), "a"_str);
        EXPECT_EQ(result_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_a->callsig());
        EXPECT_EQ(result_a->consts().size(), 0);
    }
    
    if (result_b) {
        EXPECT_TRUE(result_b->complete());
        EXPECT_EQ(result_b->fullname(), "b"_str);
        EXPECT_EQ(result_b->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_b->callsig());
        EXPECT_EQ(result_b->consts().size(), 0);
    }
    
    if (result_c) {
        EXPECT_TRUE(result_c->complete());
        EXPECT_EQ(result_c->fullname(), "c"_str);
        EXPECT_EQ(result_c->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_c->callsig());
        EXPECT_EQ(result_c->consts().size(), 0);
    }
}

TEST_P(DomainImplTests, Load_FailDueToInstantiationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // f will fail instantiation due to type b not being available

    ASSERT_TRUE(dm->upload(yama::type_info(f_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(a_info)));
    //ASSERT_TRUE(dm->upload(yama::type_info(b_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(c_info)));


    const auto result_f = dm->load("f"_str);
    const auto result_a = dm->load("a"_str);
    const auto result_c = dm->load("c"_str);


    // we don't care about the details of types a and c

    ASSERT_FALSE(result_f);
    ASSERT_TRUE(result_a);
    ASSERT_TRUE(result_c);

    ASSERT_GE(dbg->count(yama::dsignal::instant_type_not_found), 1);
}

TEST_P(DomainImplTests, LoadNone) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_none(), dm->load("None"_str).value());
}

TEST_P(DomainImplTests, LoadInt) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_int(), dm->load("Int"_str).value());
}

TEST_P(DomainImplTests, LoadUInt) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_uint(), dm->load("UInt"_str).value());
}

TEST_P(DomainImplTests, LoadFloat) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_float(), dm->load("Float"_str).value());
}

TEST_P(DomainImplTests, LoadBool) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_bool(), dm->load("Bool"_str).value());
}

TEST_P(DomainImplTests, LoadChar) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_EQ(dm->load_char(), dm->load("Char"_str).value());
}


// IMPORTANT:
//      upload tests presume that since static_verifier is so well tested, that so
//      long as *basic* behaviour can be *broadly* ensured, that it can be presumed
//      that static_verifier is the mechanism by which this behaviour is implemented,
//      such that its guarantees can be presumed to likewise apply here

// IMPORTANT:
//      take note also that these tests also presume that the templated overload
//      of upload working can be taken to imply that its type_data overload also works,
//      as it's presumed to be the mechanism by which the former is implemented

TEST_P(DomainImplTests, Upload_One) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    ASSERT_TRUE(dm->upload(yama::type_info(f_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(a_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(b_info)));
    ASSERT_TRUE(dm->upload(yama::type_info(c_info)));

    // test that uploaded types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f, a, b, and c, to see that they uploaded correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->consts()));
    
    ASSERT_EQ(ff->consts().size(), 3);
    EXPECT_EQ(ff->consts().type(0), aa);
    EXPECT_EQ(ff->consts().type(1), bb);
    EXPECT_EQ(ff->consts().type(2), cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->consts().size(), 0);

    EXPECT_TRUE(bb->complete());
    EXPECT_EQ(bb->fullname(), "b"_str);
    EXPECT_EQ(bb->kind(), yama::kind::primitive);
    EXPECT_FALSE(bb->callsig());
    EXPECT_EQ(bb->consts().size(), 0);

    EXPECT_TRUE(cc->complete());
    EXPECT_EQ(cc->fullname(), "c"_str);
    EXPECT_EQ(cc->kind(), yama::kind::primitive);
    EXPECT_FALSE(cc->callsig());
    EXPECT_EQ(cc->consts().size(), 0);
}

TEST_P(DomainImplTests, Upload_One_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    EXPECT_FALSE(dm->upload(yama::type_info(bad_info)));

    // test that no new type was made available

    EXPECT_FALSE(dm->load("bad"_str));
}

TEST_P(DomainImplTests, Upload_MultiViaSpan) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    std::vector<yama::type_info> group{
        f_info,
        a_info,
        b_info,
        c_info,
    };

    ASSERT_TRUE(dm->upload(std::span(group.begin(), group.end())));

    // test that uploaded types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f, a, b, and c, to see that they uploaded correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->consts()));
    
    ASSERT_EQ(ff->consts().size(), 3);
    EXPECT_EQ(ff->consts().type(0), aa);
    EXPECT_EQ(ff->consts().type(1), bb);
    EXPECT_EQ(ff->consts().type(2), cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->consts().size(), 0);

    EXPECT_TRUE(bb->complete());
    EXPECT_EQ(bb->fullname(), "b"_str);
    EXPECT_EQ(bb->kind(), yama::kind::primitive);
    EXPECT_FALSE(bb->callsig());
    EXPECT_EQ(bb->consts().size(), 0);

    EXPECT_TRUE(cc->complete());
    EXPECT_EQ(cc->fullname(), "c"_str);
    EXPECT_EQ(cc->kind(), yama::kind::primitive);
    EXPECT_FALSE(cc->callsig());
    EXPECT_EQ(cc->consts().size(), 0);
}

TEST_P(DomainImplTests, Upload_MultiViaSpan_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    std::vector<yama::type_info> group{
        f_info,
        a_info,
        bad_info, // <- should cause whole group to fail upload
        c_info,
    };

    EXPECT_FALSE(dm->upload(std::span(group.begin(), group.end())));

    // test that no new types were made available

    EXPECT_FALSE(dm->load("f"_str));
    EXPECT_FALSE(dm->load("a"_str));
    EXPECT_FALSE(dm->load("bad"_str));
    EXPECT_FALSE(dm->load("c"_str));
}

TEST_P(DomainImplTests, Upload_MultiViaVector) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    std::vector<yama::type_info> group{
        f_info,
        a_info,
        b_info,
        c_info,
    };

    ASSERT_TRUE(dm->upload(group));

    // test that uploaded types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f, a, b, and c, to see that they uploaded correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->consts()));
    
    ASSERT_EQ(ff->consts().size(), 3);
    EXPECT_EQ(ff->consts().type(0), aa);
    EXPECT_EQ(ff->consts().type(1), bb);
    EXPECT_EQ(ff->consts().type(2), cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->consts().size(), 0);

    EXPECT_TRUE(bb->complete());
    EXPECT_EQ(bb->fullname(), "b"_str);
    EXPECT_EQ(bb->kind(), yama::kind::primitive);
    EXPECT_FALSE(bb->callsig());
    EXPECT_EQ(bb->consts().size(), 0);

    EXPECT_TRUE(cc->complete());
    EXPECT_EQ(cc->fullname(), "c"_str);
    EXPECT_EQ(cc->kind(), yama::kind::primitive);
    EXPECT_FALSE(cc->callsig());
    EXPECT_EQ(cc->consts().size(), 0);
}

TEST_P(DomainImplTests, Upload_MultiViaVector_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    std::vector<yama::type_info> group{
        f_info,
        a_info,
        bad_info, // <- should cause whole group to fail upload
        c_info,
    };

    EXPECT_FALSE(dm->upload(group));

    // test that no new types were made available

    EXPECT_FALSE(dm->load("f"_str));
    EXPECT_FALSE(dm->load("a"_str));
    EXPECT_FALSE(dm->load("bad"_str));
    EXPECT_FALSE(dm->load("c"_str));
}

TEST_P(DomainImplTests, Upload_MultiViaInitList) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    std::initializer_list<yama::type_info> group{
        f_info,
        a_info,
        b_info,
        c_info,
    };

    ASSERT_TRUE(dm->upload(group));

    // test that uploaded types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f, a, b, and c, to see that they uploaded correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->consts()));
    
    ASSERT_EQ(ff->consts().size(), 3);
    EXPECT_EQ(ff->consts().type(0), aa);
    EXPECT_EQ(ff->consts().type(1), bb);
    EXPECT_EQ(ff->consts().type(2), cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->consts().size(), 0);

    EXPECT_TRUE(bb->complete());
    EXPECT_EQ(bb->fullname(), "b"_str);
    EXPECT_EQ(bb->kind(), yama::kind::primitive);
    EXPECT_FALSE(bb->callsig());
    EXPECT_EQ(bb->consts().size(), 0);

    EXPECT_TRUE(cc->complete());
    EXPECT_EQ(cc->fullname(), "c"_str);
    EXPECT_EQ(cc->kind(), yama::kind::primitive);
    EXPECT_FALSE(cc->callsig());
    EXPECT_EQ(cc->consts().size(), 0);
}

TEST_P(DomainImplTests, Upload_MultiViaInitList_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    std::initializer_list<yama::type_info> group{
        f_info,
        a_info,
        bad_info, // <- should cause whole group to fail upload
        c_info,
    };

    EXPECT_FALSE(dm->upload(group));

    // test that no new types were made available

    EXPECT_FALSE(dm->load("f"_str));
    EXPECT_FALSE(dm->load("a"_str));
    EXPECT_FALSE(dm->load("bad"_str));
    EXPECT_FALSE(dm->load("c"_str));
}

TEST_P(DomainImplTests, Upload_MultiViaModule) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    yama::module_info m =
        yama::module_factory()
        .add_type(yama::type_info(f_info))
        .add_type(yama::type_info(a_info))
        .add_type(yama::type_info(b_info))
        .add_type(yama::type_info(c_info))
        .done();

    ASSERT_TRUE(dm->upload(yama::make_res<yama::module_info>(std::move(m))));

    // test that uploaded types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f, a, b, and c, to see that they uploaded correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->consts()));
    
    ASSERT_EQ(ff->consts().size(), 3);
    EXPECT_EQ(ff->consts().type(0), aa);
    EXPECT_EQ(ff->consts().type(1), bb);
    EXPECT_EQ(ff->consts().type(2), cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->consts().size(), 0);

    EXPECT_TRUE(bb->complete());
    EXPECT_EQ(bb->fullname(), "b"_str);
    EXPECT_EQ(bb->kind(), yama::kind::primitive);
    EXPECT_FALSE(bb->callsig());
    EXPECT_EQ(bb->consts().size(), 0);

    EXPECT_TRUE(cc->complete());
    EXPECT_EQ(cc->fullname(), "c"_str);
    EXPECT_EQ(cc->kind(), yama::kind::primitive);
    EXPECT_FALSE(cc->callsig());
    EXPECT_EQ(cc->consts().size(), 0);
}

TEST_P(DomainImplTests, Upload_MultiViaModule_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    yama::module_info m =
        yama::module_factory()
        .add_type(yama::type_info(f_info))
        .add_type(yama::type_info(a_info))
        .add_type(yama::type_info(bad_info)) // <- should cause whole group to fail upload
        .add_type(yama::type_info(c_info))
        .done();

    ASSERT_FALSE(dm->upload(yama::make_res<yama::module_info>(std::move(m))));

    // test that no new types were made available

    EXPECT_FALSE(dm->load("f"_str));
    EXPECT_FALSE(dm->load("a"_str));
    EXPECT_FALSE(dm->load("bad"_str));
    EXPECT_FALSE(dm->load("c"_str));
}

TEST_P(DomainImplTests, Upload_SrcCode) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    std::string txt = R"(

fn pi() -> Float {
    return 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    ASSERT_TRUE(dm->upload(src));

    // test that no new types were made available

    EXPECT_TRUE(dm->load("pi"_str));

    yama::context ctx(dm, dbg);

    ASSERT_TRUE(ctx.push_fn(dm->load("pi"_str).value()).good());
    ASSERT_TRUE(ctx.call(1, yama::newtop).good());

    EXPECT_TRUE(ctx.local(0));
    if (const auto x = ctx.local(0); x->t == dm->load_float()) {
        EXPECT_DOUBLE_EQ(x->as_float(), 3.14159);
    }
}

TEST_P(DomainImplTests, Upload_Str) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    std::string txt = R"(

fn pi() -> Float {
    return 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    ASSERT_TRUE(dm->upload(src.str())); // <- compile w/ the string part only

    // test that no new types were made available

    EXPECT_TRUE(dm->load("pi"_str));

    yama::context ctx(dm, dbg);

    ASSERT_TRUE(ctx.push_fn(dm->load("pi"_str).value()).good());
    ASSERT_TRUE(ctx.call(1, yama::newtop).good());

    EXPECT_TRUE(ctx.local(0));
    if (const auto x = ctx.local(0); x->t == dm->load_float()) {
        EXPECT_DOUBLE_EQ(x->as_float(), 3.14159);
    }
}

TEST_P(DomainImplTests, Upload_FilePath) {
    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_TRUE(dm->upload(std::filesystem::current_path() / "support-files/domain-impl-tests-helper.yama"));

    // test that no new types were made available

    EXPECT_TRUE(dm->load("pi"_str));

    yama::context ctx(dm, dbg);

    ASSERT_TRUE(ctx.push_fn(dm->load("pi"_str).value()).good());
    ASSERT_TRUE(ctx.call(1, yama::newtop).good());

    EXPECT_TRUE(ctx.local(0));
    if (const auto x = ctx.local(0); x->t == dm->load_float()) {
        EXPECT_DOUBLE_EQ(x->as_float(), 3.14159);
    }
}

TEST_P(DomainImplTests, Upload_FilePath_FileNotFound) {
    const auto path = std::filesystem::current_path() / "support-files/some-file-that-does-not-exist.yama";
    ASSERT_FALSE(std::filesystem::exists(path));

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    ASSERT_FALSE(dm->upload(path));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_file_not_found), 1);
}


class test_parcel0 final : public yama::parcel {
public:
    using run_test_asserts_fn_t = void(*)(test_parcel0& host, yama::parcel_services services);


    run_test_asserts_fn_t run_test_asserts_fn;

    std::shared_ptr<yama::dsignal_debug> dsignal_dbg;

    std::shared_ptr<const yama::module_info> result_module;

    yama::dep_reqs dr;


    test_parcel0(run_test_asserts_fn_t run_test_asserts_fn, std::shared_ptr<yama::dsignal_debug> dsignal_dbg)
        : parcel(),
        run_test_asserts_fn(run_test_asserts_fn),
        dsignal_dbg(dsignal_dbg) {}


    const yama::dep_reqs& deps() override final { return dr; }

    std::shared_ptr<const yama::module_info> import(yama::parcel_services services, yama::str relative_path) override final {
        if (relative_path != ""_str) return nullptr;
        run_test_asserts_fn(*this, services);
        // launder result_module to domain so we can test behaviour to see if compiled correctly
        return yama::res(result_module);
    }
};

TEST_P(DomainImplTests, ParcelServices_Methods) {
    auto our_run_test_asserts_fn =
        [](test_parcel0& host, yama::parcel_services services) {

        ASSERT_EQ(services.install_name(), "a"_str);

        std::string txt = R"(

fn weed_day() -> Int {
    return 420;
}

)";
        taul::source_code src{};
        src.add_str("src"_str, yama::str(txt));

        // this will get laundered to main test fn where we behaviourally test it for correctness
        host.result_module = services.compile(src);

        ASSERT_TRUE(host.result_module);
        };

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    auto our_parcel = yama::make_res<test_parcel0>(our_run_test_asserts_fn, dbg);

    yama::install_batch b{};
    b.install("a"_str, our_parcel);

    ASSERT_TRUE(dm->install(std::move(b)));

    const auto m = dm->import("a"_str);
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn

    ASSERT_TRUE(m);
    ASSERT_TRUE(dm->upload(yama::make_res<yama::module_info>(m->info())));

    const auto weed_day = dm->load("weed_day"_str);
    ASSERT_TRUE(weed_day);

    ASSERT_EQ(weed_day->kind(), yama::kind::function);
    ASSERT_EQ(weed_day->callsig().value().fmt(), "fn() -> Int");

    yama::context ctx(dm, dbg);

    ASSERT_TRUE(ctx.push_fn(*weed_day).good());
    ASSERT_TRUE(ctx.call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx.local(0).value(), ctx.new_int(420));
}


// test_compiler always fails compilation, as its purpose is solely to launder in run_test_asserts_fn
// to perform the behaviour of the unit test using the compiler services object

class test_compiler0 final : public yama::compiler {
public:
    using run_test_asserts_fn_t = void(*)(test_compiler0& host, yama::compiler_services services);


    run_test_asserts_fn_t run_test_asserts_fn;

    std::shared_ptr<yama::dsignal_debug> dsignal_dbg;

    yama::res<test_parcel1> parcel1;

    std::optional<yama::module> result_abc_1, result_def_1, result_root_1;


    test_compiler0(run_test_asserts_fn_t run_test_asserts_fn, std::shared_ptr<yama::dsignal_debug> dsignal_dbg, yama::res<test_parcel1> parcel1)
        : compiler(),
        run_test_asserts_fn(run_test_asserts_fn),
        dsignal_dbg(dsignal_dbg),
        parcel1(parcel1) {}

    std::shared_ptr<const yama::module_info> compile(yama::compiler_services services, const taul::source_code& src) {
        run_test_asserts_fn(*this, services);
        return nullptr; // always fail
    }
};

class test_parcel2 final : public yama::parcel {
public:
    std::optional<yama::dep_reqs> dr = {};


    test_parcel2() = default;


    const yama::dep_reqs& deps() override final {
        if (!dr) {
            dr = yama::dep_reqs();
            dr->add("other"_str); // <- expected to be parcel1
        }
        return *dr;
    }

    std::shared_ptr<const yama::module_info> import(yama::parcel_services services, yama::str relative_path) override final {
        // this parcel exists solely to trigger our compiler, and carry an 'other' dep mapping
        (void)services.compile(taul::source_code{});
        return nullptr;
    }
};

TEST_P(DomainImplTests, CompilerServices_Import) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::compiler_services services) {
        const auto abc_1 = services.import("other.a.b.c"_str);

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".a.b.c"_str);
        
        const auto def_1 = services.import("other.d.e.f"_str);

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".d.e.f"_str);

        const auto root_1 = services.import("other"_str); // root

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ""_str);

        // lookup of memoized will NOT result in parcel import method call
        host.parcel1->last_relative_path.reset();

        const auto abc_2 = services.import("other.a.b.c"_str); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);

        const auto def_2 = services.import("other.d.e.f"_str); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);

        const auto root_2 = services.import("other"_str); // test imports memoize

        ASSERT_FALSE(host.parcel1->last_relative_path);


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

        // launder these to main test body so we can safely use dm->import
        // to compare against them
        host.result_abc_1 = abc_1;
        host.result_def_1 = def_1;
        host.result_root_1 = root_1;
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel2>();

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
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    ASSERT_FALSE(dm->import("parcel2"_str)); // trigger compiler, expect failure
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn

    // test are same as domain env imported versions
    ASSERT_EQ(our_compiler->result_abc_1, dm->import("parcel1.a.b.c"_str));
    ASSERT_EQ(our_compiler->result_def_1, dm->import("parcel1.d.e.f"_str));
    ASSERT_EQ(our_compiler->result_root_1, dm->import("parcel1"_str));
}

TEST_P(DomainImplTests, CompilerServices_Import_Fail_ModuleNotFound_NoHead_WithNoRelativePath) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::compiler_services services) {
        ASSERT_FALSE(services.import(""_str));

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_module_not_found), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel2>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    ASSERT_FALSE(dm->import("parcel2"_str)); // trigger compiler, expect failure
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

TEST_P(DomainImplTests, CompilerServices_Import_Fail_ModuleNotFound_NoHead_WithRelativePath) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::compiler_services services) {
        ASSERT_FALSE(services.import(".a.b.c"_str));

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_module_not_found), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel2>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    ASSERT_FALSE(dm->import("parcel2"_str)); // trigger compiler, expect failure
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

TEST_P(DomainImplTests, CompilerServices_Import_Fail_ModuleNotFound_BadHead) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::compiler_services services) {
        ASSERT_FALSE(services.import("bad.a.b.c"_str));

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_module_not_found), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel2>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    ASSERT_FALSE(dm->import("parcel"_str)); // trigger compiler, expect failure
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

TEST_P(DomainImplTests, CompilerServices_Import_Fail_ModuleNotFound_BadRelativePath) {
    auto our_run_test_asserts_fn =
        [](test_compiler0& host, yama::compiler_services services) {
        ASSERT_FALSE(services.import("other.b.a.d"_str));

        ASSERT_TRUE(host.parcel1->last_relative_path);
        ASSERT_EQ(*host.parcel1->last_relative_path, ".b.a.d"_str);

        ASSERT_GE(host.dsignal_dbg->count(yama::dsignal::import_module_not_found), 1);
        };

    const auto parcel1 = yama::make_res<test_parcel1>();
    const auto parcel2 = yama::make_res<test_parcel2>();

    ASSERT_FALSE(parcel1->last_relative_path);

    auto our_compiler = std::make_shared<test_compiler0>(our_run_test_asserts_fn, dbg, parcel1);

    yama::domain_config config{
        .compiler = our_compiler,
    };

    const auto dm = GetParam().factory(yama::domain_config{}, dbg);

    yama::install_batch ib{};
    ib
        .install("parcel1"_str, parcel1)
        .install("parcel2"_str, parcel2)
        .map_dep("parcel2"_str, "other"_str, "parcel1"_str);

    ASSERT_TRUE(dm->install(std::move(ib)));

    ASSERT_FALSE(dm->import("parcel2"_str)); // trigger compiler, expect failure
    if (HasFatalFailure()) return; // if fatal assertion failure occurred within our_run_test_asserts_fn
}

