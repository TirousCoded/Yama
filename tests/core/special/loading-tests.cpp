

#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/debug.h>
#include <yama/core/domain.h>
#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/module_info.h>


using namespace yama::string_literals;


class LoadingTests : public testing::Test {
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


// TODO: I'm fairly certain we lack tests covering things like inter-parcel dependency
//       relationships between loaded types

// TODO: also, maybe add in tests for things like whether impl can properly handle discerning
//       whether the parcel env import path defining a type ref constant is valid or not (w/
//       these tests only summarily testing the particulars of what constitutes validity of these)
//
//       we need this, as currently nothing is testing if impl can properly handle failing
//       to load due to invalid import path part
//
//       as part of this, be sure to look into the particulars about import dir rules in
//       compilation-tests.cpp for what it asserts

// TODO: alongside the above about type ref constant import paths being valid, we'd also do
//       best to assert things like impl failing a load of one of these types if the module
//       it's from is *invalid* (ie. static verif fail)


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


TEST_F(LoadingTests, EnsureWorksWithAllConstTypes) {
    static_assert(yama::const_types == 9);
    // dep graph:
    //      a <- Int -4         (not a dep)
    //        <- UInt 301       (not a dep)
    //        <- Float 3.14159  (not a dep)
    //        <- Bool true      (not a dep)
    //        <- Char 'y'       (not a dep)
    //        <- b0             (primitive)
    //        <- b1             (function)
    //        <- b0::m          (method)
    //        <- b2             (struct)

    // this test is a catch-all for us testing that loading works w/ all
    // constant types *generally*

    // this test covers that loading properly handles non-type constants
    // in the constant table

    // this test covers that loading properly handles type constants in
    // a general case, w/ the below Instantiate_# tests then testing linking
    // behaviour more thoroughly, but presuming that this test passing means
    // that linking behaviour can be expected to *generalize* across all the
    // different type constant types

    static_assert(yama::const_types == 9);
    auto consts =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("self:b0"_str)
        .add_function_type("self:b1"_str, yama::make_callsig({}, 5)) // ie. fn() -> b0
        .add_method_type("self:b0::m"_str, yama::make_callsig({}, 5)) // ie. fn() -> b0
        .add_struct_type("self:b2"_str);
    yama::module_factory mf{};
    mf
        .add_primitive("a"_str, consts, yama::ptype::bool0)
        .add_primitive("b0"_str, {}, yama::ptype::bool0)
        .add_function(
            "b1"_str, consts,
            yama::make_callsig({}, 5), // ie. fn() -> b0
            10,
            yama::noop_call_fn)
        .add_method(
            "b0::m"_str, consts,
            yama::make_callsig({}, 5), // ie. fn() -> b0
            10,
            yama::noop_call_fn)
        .add_struct("b2"_str, consts);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b0 = dm->load("p:b0"_str);
    const auto b1 = dm->load("p:b1"_str);
    const auto b0_m = dm->load("p:b0::m"_str);
    const auto b2 = dm->load("p:b2"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b0);
    ASSERT_TRUE(b1);
    ASSERT_TRUE(b0_m);
    ASSERT_TRUE(b2);

    const yama::type aa = a.value();
    const yama::type bb0 = b0.value();
    const yama::type bb1 = b1.value();
    const yama::type bb0_dot_m = b0_m.value();
    const yama::type bb2 = b2.value();

    // for this test we only really care that type 'a' loads correctly

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), yama::const_types);

    static_assert(yama::const_types == 9);
    EXPECT_EQ(aa.consts().get<yama::int_const>(0), std::make_optional(-4));
    EXPECT_EQ(aa.consts().get<yama::uint_const>(1), std::make_optional(301));
    EXPECT_EQ(aa.consts().get<yama::float_const>(2), std::make_optional(3.14159));
    EXPECT_EQ(aa.consts().get<yama::bool_const>(3), std::make_optional(true));
    EXPECT_EQ(aa.consts().get<yama::char_const>(4), std::make_optional(U'y'));
    EXPECT_EQ(aa.consts().get<yama::primitive_type_const>(5), std::make_optional(bb0));
    EXPECT_EQ(aa.consts().get<yama::function_type_const>(6), std::make_optional(bb1));
    EXPECT_EQ(aa.consts().get<yama::method_type_const>(7), std::make_optional(bb0_dot_m));
    EXPECT_EQ(aa.consts().get<yama::struct_type_const>(8), std::make_optional(bb2));
}

TEST_F(LoadingTests, WithNoDeps) {
    // dep graph:
    //      a

    yama::module_factory mf{};
    mf
        .add_primitive("a"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test

    ASSERT_TRUE(a);

    const yama::type aa = a.value();

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 0);
}

TEST_F(LoadingTests, WithDeps) {
    // dep graph:
    //      a -> b
    //        -> c

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    yama::module_factory mf{};
    mf
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_primitive("b"_str, {}, yama::ptype::bool0)
        .add_primitive("c"_str, {}, yama::ptype::bool0);

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

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 0);

    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 0);
}

TEST_F(LoadingTests, WithDeps_WithIndirectlyReferencedTypes) {
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
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_primitive("b"_str, b_consts, yama::ptype::bool0)
        .add_primitive("c"_str, c_consts, yama::ptype::bool0)
        .add_primitive("d"_str, {}, yama::ptype::bool0)
        .add_primitive("e"_str, {}, yama::ptype::bool0)
        .add_primitive("f"_str, {}, yama::ptype::bool0);

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

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 2);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(bb.consts().type(1), std::make_optional(ee));

    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 1);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(ff));

    EXPECT_EQ(dd.fullname(), "p:d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);

    EXPECT_EQ(ee.fullname(), "p:e"_str);
    EXPECT_EQ(ee.kind(), yama::kind::primitive);
    EXPECT_EQ(ee.consts().size(), 0);

    EXPECT_EQ(ff.fullname(), "p:f"_str);
    EXPECT_EQ(ff.kind(), yama::kind::primitive);
    EXPECT_EQ(ff.consts().size(), 0);
}

TEST_F(LoadingTests, WithDeps_TypeReferencedMultipleTimesInAcyclicDepGraph) {
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
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_primitive("b"_str, b_consts, yama::ptype::bool0)
        .add_primitive("c"_str, c_consts, yama::ptype::bool0)
        .add_primitive("d"_str, {}, yama::ptype::bool0);

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

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));

    EXPECT_EQ(cc.fullname(), "p:c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 3);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(1), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(2), std::make_optional(dd));

    EXPECT_EQ(dd.fullname(), "p:d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);
}

TEST_F(LoadingTests, WithDeps_DepGraphCycle) {
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
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_primitive("b"_str, b_consts, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    const auto a = dm->load("p:a"_str); // <- main one under test
    const auto b = dm->load("p:b"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    const yama::type aa = a.value();
    const yama::type bb = b.value();

    EXPECT_EQ(aa.fullname(), "p:a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(aa));

    EXPECT_EQ(bb.fullname(), "p:b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(aa));
}

TEST_F(LoadingTests, Fail_OriginalTypeNotFound) {
    // dep graph:
    //      a           <- error is that no type_info 'a' provided

    yama::module_factory mf{};

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail

    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_F(LoadingTests, Fail_ReferencedTypeNotFound) {
    // dep graph:
    //      a -> b      <- error is that no type_info 'b' provided
    //        -> c

    auto a_consts =
        yama::const_table_info()
        .add_primitive_type("self:b"_str)
        .add_primitive_type("self:c"_str);
    yama::module_factory mf{};
    mf
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        //.add_primitive("b"_str, {}, yama::ptype::bool0)
        .add_primitive("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail

    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_F(LoadingTests, Fail_IndirectlyReferencedTypeNotFound) {
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
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_primitive("b"_str, b_consts, yama::ptype::bool0)
        .add_primitive("c"_str, c_consts, yama::ptype::bool0)
        //.add_primitive("d"_str, {}, yama::ptype::bool0) <- missing
        .add_primitive("e"_str, {}, yama::ptype::bool0)
        .add_primitive("f"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail
    
    EXPECT_EQ(dbg->count(yama::dsignal::load_type_not_found), 1);
}

TEST_F(LoadingTests, Fail_KindMismatch) {
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
        .add_primitive("a"_str, a_consts, yama::ptype::bool0)
        .add_function(
            "b"_str, b_consts,
            yama::make_callsig({ 0 }, 0),
            10,
            yama::noop_call_fn)
        .add_primitive("c"_str, {}, yama::ptype::bool0);

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

TEST_F(LoadingTests, Fail_CallSigMismatch) {
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
    auto clever_callsig_info_for_test = yama::make_callsig({ 0 }, 0);

    auto a_consts =
        yama::const_table_info()
        // callsig is intended to mismatch w/ the indirectly referenced type b
        .add_function_type("self:b"_str, clever_callsig_info_for_test);
    auto b_consts =
        yama::const_table_info()
        .add_primitive_type("self:c"_str); // <- c exists for b's constant indices to differ from a's
    yama::module_factory mf{};
    mf
        .add_function(
            "a"_str,
            a_consts,
            clever_callsig_info_for_test,
            10,
            yama::noop_call_fn)
        .add_function(
            "b"_str,
            b_consts,
            clever_callsig_info_for_test,
            10,
            yama::noop_call_fn)
        .add_primitive("c"_str, {}, yama::ptype::bool0);

    yama::install_batch ib{};
    ib.install("p"_str, yama::make_res<test_parcel>(mf));
    ASSERT_TRUE(dm->install(std::move(ib)));

    EXPECT_FALSE(dm->load("p:a"_str)); // <- should fail (due to callsig mismatch between 'a' and 'b')

    EXPECT_EQ(dbg->count(yama::dsignal::load_callsig_mismatch), 1);
}

