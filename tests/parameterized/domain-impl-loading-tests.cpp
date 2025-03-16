

#include "domain-impl-loading-tests.h"

#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/module_info.h>


using namespace yama::string_literals;


GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DomainImplLoadingTests);


// TODO: the below unit tests were initially written prior to the addition of parcels/modules,
//       and so *naively* operates w/ only tests for domain env, w/ NONE covering parcel env
//
//       this is a problem, and so we'd be best to figure out how we want to revise our test
//       suite in the future to address this

// TODO: another problem arising w/ the below tests is that I'm fairly certain we lack tests
//       covering things like inter-parcel dependency relationships between loaded types

// TODO: also, maybe add in tests for things like whether impl can properly handle discerning
//       whether the parcel env import path defining a type symbol is valid or not (w/ these
//       tests only summarily testing the particulars of what constitutes validity of these)
//
//       we need this, as currently nothing is testing if impl can properly handle failing
//       to load due to invalid import path part


// IMPORTANT:
//      our policy is gonna be to only testing a single error per unit test, not testing
//      multiple different errors arising in one unit test
//
//      the reason is so that the impl is free to forgo work checking for other errors
//      after one is found, nor any complexity about certain errors having to be grouped
//      together w/ others
//
//      one error may correspond to multiple dsignal raises


namespace {
    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        yama::res<yama::module_info> m;


        test_parcel(yama::module_factory& mf)
            : parcel(),
            m(yama::make_res<yama::module_info>(mf.done())) {}


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, {} };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& import_path) override final {
            if (import_path != ""_str) return std::nullopt;
            return decltype(m)(m);
        }
    };
}


// IMPORTANT:
//      below, the term 'indirectly referenced type' refers to a type which is
//      not specified in the constant table of the original type queried, but 
//      is nevertheless indirectly a part of its dep graph


TEST_P(DomainImplLoadingTests, EnsureWorksWithAllConstTypes) {
    const auto dm = GetParam().factory(dbg);

    static_assert(yama::const_types == 7);
    // dep graph:
    //      a <- Int -4         (not a dep)
    //        <- UInt 301       (not a dep)
    //        <- Float 3.14159  (not a dep)
    //        <- Bool true      (not a dep)
    //        <- Char U'y'      (not a dep)
    //        <- b0             (primitive)
    //        <- b1             (function)

    // this test is a catch-all for us testing that loading works w/ all
    // constant types *generally*

    // this test covers that loading properly handles non-type constants
    // in the constant table

    // this test covers that loading properly handles type constants in
    // a general case, w/ the below Instantiate_# tests then testing linking
    // behaviour more thoroughly, but presuming that this test passing means
    // that linking behaviour can be expected to *generalize* across all the
    // different type constant types

    static_assert(yama::const_types == 7);
    auto a_consts =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("self:b0"_str)
        .add_function_type("self:b1"_str, yama::make_callsig_info({}, 5)); // ie. fn() -> b0
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b0"_str, {}, yama::ptype::bool0)
        .add_function_type(
            "b1"_str, decltype(a_consts)(a_consts),
            yama::make_callsig_info({}, 5), // ie. fn() -> b0
            10,
            yama::noop_call_fn);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b0 = dm->load("p:b0"_str);
    const auto b1 = dm->load("p:b1"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b0);
    ASSERT_TRUE(b1);

    const yama::type aa = a.value();
    const yama::type bb0 = b0.value();
    const yama::type bb1 = b1.value();

    // for this test we only really care that type 'a' loads correctly

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), yama::const_types);

    static_assert(yama::const_types == 7);
    EXPECT_EQ(aa.consts().get<yama::int_const>(0), std::make_optional(-4));
    EXPECT_EQ(aa.consts().get<yama::uint_const>(1), std::make_optional(301));
    EXPECT_EQ(aa.consts().get<yama::float_const>(2), std::make_optional(3.14159));
    EXPECT_EQ(aa.consts().get<yama::bool_const>(3), std::make_optional(true));
    EXPECT_EQ(aa.consts().get<yama::char_const>(4), std::make_optional(U'y'));
    EXPECT_EQ(aa.consts().get<yama::primitive_type_const>(5), std::make_optional(bb0));
    EXPECT_EQ(aa.consts().get<yama::function_type_const>(6), std::make_optional(bb1));
}

TEST_P(DomainImplLoadingTests, WithNoDeps) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a

    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test

    ASSERT_TRUE(a);

    const yama::type aa = a.value();

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 0);
}

TEST_P(DomainImplLoadingTests, WithDeps) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b
    //        -> c

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b"_str, {}, yama::ptype::bool0)
        .add_primitive_type("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b = dm->load("p:b"_str);
    const auto c = dm->load("p:c"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);

    const yama::type aa = a.value();
    const yama::type bb = b.value();
    const yama::type cc = c.value();

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 0);

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 0);
}

TEST_P(DomainImplLoadingTests, WithDeps_WithIndirectlyReferencedTypes) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> d
    //             -> e
    //        -> c -> f

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:d"_str)
        .add_primitive_type("self:e"_str);
    auto c_consts =
        yama::const_table_info()
        .add_primitive_type("self:f"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b"_str, decltype(b_consts)(b_consts), yama::ptype::bool0)
        .add_primitive_type("c"_str, decltype(c_consts)(c_consts), yama::ptype::bool0)
        .add_primitive_type("d"_str, {}, yama::ptype::bool0)
        .add_primitive_type("e"_str, {}, yama::ptype::bool0)
        .add_primitive_type("f"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b = dm->load("p:b"_str);
    const auto c = dm->load("p:c"_str);
    const auto d = dm->load("p:d"_str);
    const auto e = dm->load("p:e"_str);
    const auto f = dm->load("p:f"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);
    ASSERT_TRUE(e);
    ASSERT_TRUE(f);

    const yama::type aa = a.value();
    const yama::type bb = b.value();
    const yama::type cc = c.value();
    const yama::type dd = d.value();
    const yama::type ee = e.value();
    const yama::type ff = f.value();

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 2);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(bb.consts().type(1), std::make_optional(ee));

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 1);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(ff));

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "p:d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);

    EXPECT_TRUE(ee.complete());
    EXPECT_EQ(ee.fullname(), "p:e"_str);
    EXPECT_EQ(ee.kind(), yama::kind::primitive);
    EXPECT_EQ(ee.consts().size(), 0);

    EXPECT_TRUE(ff.complete());
    EXPECT_EQ(ff.fullname(), "p:f"_str);
    EXPECT_EQ(ff.kind(), yama::kind::primitive);
    EXPECT_EQ(ff.consts().size(), 0);
}

TEST_P(DomainImplLoadingTests, WithDeps_TypeReferencedMultipleTimesInAcyclicDepGraph) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> d
    //        -> c -> d
    //             -> d     <- test w/ multiple refs to 'd' on same 'c' node in graph
    //             -> d

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:d"_str);
    auto c_consts =
        yama::const_table_info()
        .add_primitive_type("self:d"_str)
        .add_primitive_type("self:d"_str)
        .add_primitive_type("self:d"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b"_str, decltype(b_consts)(b_consts), yama::ptype::bool0)
        .add_primitive_type("c"_str, decltype(c_consts)(c_consts), yama::ptype::bool0)
        .add_primitive_type("d"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b = dm->load("p:b"_str);
    const auto c = dm->load("p:c"_str);
    const auto d = dm->load("p:d"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);

    const yama::type aa = a.value();
    const yama::type bb = b.value();
    const yama::type cc = c.value();
    const yama::type dd = d.value();

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 3);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(1), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(2), std::make_optional(dd));

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "p:d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);
}

TEST_P(DomainImplLoadingTests, WithDeps_DepGraphCycle) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> a     (back ref)
    //        -> a          (back ref)

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:a"_str);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:a"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b"_str, decltype(b_consts)(b_consts), yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b = dm->load("p:b"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    const yama::type aa = a.value();
    const yama::type bb = b.value();

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(aa));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(aa));
}

TEST_P(DomainImplLoadingTests, Fail_OriginalTypeNotFound) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a           <- error is that no type_info 'a' provided

    yama::module_factory mf{};

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail

    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_P(DomainImplLoadingTests, Fail_ReferencedTypeNotFound) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b      <- error is that no type_info 'b' provided
    //        -> c

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        //.add_primitive_type("b"_str, {}, yama::ptype::bool0)
        .add_primitive_type("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail

    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_P(DomainImplLoadingTests, Fail_IndirectlyReferencedTypeNotFound) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> d     <- error is that no type_info 'd' provided
    //             -> e
    //        -> c -> f

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:d"_str) // <- import of 'a' indirectly refers to 'd'
        .add_primitive_type("self:e"_str);
    auto c_consts =
        yama::const_table_info()
        .add_primitive_type("self:f"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_primitive_type("b"_str, decltype(b_consts)(b_consts), yama::ptype::bool0)
        .add_primitive_type("c"_str, decltype(c_consts)(c_consts), yama::ptype::bool0)
        //.add_primitive_type("d"_str, {}, yama::ptype::bool0) <- missing
        .add_primitive_type("e"_str, {}, yama::ptype::bool0)
        .add_primitive_type("f"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail
    
    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_P(DomainImplLoadingTests, Fail_KindMismatch) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> c

    // a's b symbol's kind is primitive
    // b's kind is function

    auto a_consts =
        yama::const_table_info()
        // kind is intended to mismatch w/ the indirectly referenced type b
        .add_primitive_type("self:b"_str);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:c"_str);
    yama::module_factory mf{};
    mf
        .add_primitive_type("a"_str, decltype(a_consts)(a_consts), yama::ptype::bool0)
        .add_function_type(
            "b"_str, decltype(b_consts)(b_consts),
            yama::make_callsig_info({ 0 }, 0),
            10,
            yama::noop_call_fn)
        .add_primitive_type("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail (due to kind mismatch between 'a' and 'b')

    EXPECT_EQ(dbg->count(yama::dsignal::load_kind_mismatch), 1);
}

// TODO: maybe decompose below test into a few tests for each of the following cases:
//          1) differ by param count
//          2) differ by param type indices (same const table)
//          2) differ by return type indices (same const table)
//          3) differ by const table (but have otherwise identical callsigs)

TEST_P(DomainImplLoadingTests, Fail_CallSigMismatch) {
    const auto dm = GetParam().factory(dbg);

    // dep graph:
    //      a -> b -> c

    // a's b function type ref symbol's callsig is 'fn(b) -> b'
    // b's callsig is 'fn(c) -> c'

    // TODO: while clever, after re-reading this unit test code, I think we'd
    //       do well to rewrite this and other unit tests herein to be a lot
    //       easier to read, as right now it's a bit *confusing* to me

    // IMPORTANT:
    //      I actually really like the below *cleverness*, as it actually
    //      caught me during the initial impl, w/ me not otherwise noticing
    //      the subtle error in my code w/out it pointing it out to me

    // the thing that makes this test a bit *clever* is that the two
    // callsigs are actually identical in terms of their index numbers,
    // taking the form 'fn( *0* ) -> *0*', and the thing that makes them
    // different is the INTERPRETATION of these indices, as each of the
    // two respective callsigs' indices refer to different constant tables

    // put another way, type 'a' will use clever_callsig_info_for_test w/
    // a_consts, while 'b' will use clever_callsig_info_for_test w/
    // b_consts, w/ the different #_consts used by each causing
    // the above bout of *cleverness*
    auto clever_callsig_info_for_test = yama::make_callsig_info({ 0 }, 0);

    auto a_consts =
        yama::const_table_info()
        // callsig is intended to mismatch w/ the indirectly referenced type b
        .add_function_type("self:b"_str, clever_callsig_info_for_test);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:c"_str); // <- c exists for b's constant indices to differ from a's
    yama::module_factory mf{};
    mf
        .add_function_type(
            "a"_str, decltype(a_consts)(a_consts),
            decltype(clever_callsig_info_for_test)(clever_callsig_info_for_test),
            10,
            yama::noop_call_fn)
        .add_function_type(
            "b"_str, decltype(b_consts)(b_consts),
            decltype(clever_callsig_info_for_test)(clever_callsig_info_for_test),
            10,
            yama::noop_call_fn)
        .add_primitive_type("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail (due to callsig mismatch between 'a' and 'b')

    EXPECT_EQ(dbg->count(yama::dsignal::load_callsig_mismatch), 1);
}

