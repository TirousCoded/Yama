

#include "domain-impl-tests.h"

#include <yama/core/general.h>
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
        .locals = 4,
    },
};

// the type 'bad' will fail static verification during push

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
        .locals = 4,
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
    const auto dm = GetParam().factory(dbg);

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

TEST_P(DomainImplTests, Load) {
    const auto dm = GetParam().factory(dbg);

    ASSERT_TRUE(dm->push(f_info));
    ASSERT_TRUE(dm->push(a_info));
    ASSERT_TRUE(dm->push(b_info));
    ASSERT_TRUE(dm->push(c_info));


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
    const auto dm = GetParam().factory(dbg);

    // f will fail instantiation due to type b not being available

    ASSERT_TRUE(dm->push(f_info));
    ASSERT_TRUE(dm->push(a_info));
    //ASSERT_TRUE(dm->push(b_info));
    ASSERT_TRUE(dm->push(c_info));


    const auto result_f = dm->load("f"_str);
    const auto result_a = dm->load("a"_str);
    const auto result_c = dm->load("c"_str);


    // we don't care about the details of types a and c

    ASSERT_FALSE(result_f);
    ASSERT_TRUE(result_a);
    ASSERT_TRUE(result_c);
}

TEST_P(DomainImplTests, LoadNone) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_none(), dm->load("None"_str).value());
}

TEST_P(DomainImplTests, LoadInt) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_int(), dm->load("Int"_str).value());
}

TEST_P(DomainImplTests, LoadUInt) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_uint(), dm->load("UInt"_str).value());
}

TEST_P(DomainImplTests, LoadFloat) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_float(), dm->load("Float"_str).value());
}

TEST_P(DomainImplTests, LoadBool) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_bool(), dm->load("Bool"_str).value());
}

TEST_P(DomainImplTests, LoadChar) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_EQ(dm->load_char(), dm->load("Char"_str).value());
}


// IMPORTANT:
//      push tests presume that since static_verifier is so well tested, that so
//      long as *basic* behaviour can be *broadly* ensured, that it can be presumed
//      that static_verifier is the mechanism by which this behaviour is implemented,
//      such that its guarantees can be presumed to likewise apply here

// IMPORTANT:
//      take note also that these tests also presume that the templated overload
//      of push working can be taken to imply that its type_data overload also works,
//      as it's presumed to be the mechanism by which the former is implemented

TEST_P(DomainImplTests, Push) {
    const auto dm = GetParam().factory(dbg);

    // push types f, a, b, and c, to test that push works broadly

    ASSERT_TRUE(dm->push(f_info));
    ASSERT_TRUE(dm->push(a_info));
    ASSERT_TRUE(dm->push(b_info));
    ASSERT_TRUE(dm->push(c_info));

    // test that pushed types are acknowledged by the domain impl correctly

    const auto ff = dm->load("f"_str);
    const auto aa = dm->load("a"_str);
    const auto bb = dm->load("b"_str);
    const auto cc = dm->load("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f and a to see that they pushed correctly

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
}

TEST_P(DomainImplTests, Push_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_FALSE(dm->push(bad_info));

    // test that no new type was made available

    EXPECT_FALSE(dm->load("bad"_str));
}

