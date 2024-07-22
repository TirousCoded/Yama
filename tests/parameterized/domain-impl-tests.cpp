

#include "domain-impl-tests.h"

#include <yama/core/general.h>


using namespace yama::string_literals;


GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DomainImplTests);


static const yama::primitive_info a{ "a"_str, std::nullopt, {} };
static const yama::primitive_info b{ "b"_str, std::nullopt, {} };
static const yama::primitive_info c{ "c"_str, std::nullopt, {} };

static const std::vector<yama::linksym> f_linksyms{
    yama::make_linksym("a"_str, yama::kind::primitive),
    yama::make_linksym("b"_str, yama::kind::primitive),
    yama::make_linksym("c"_str, yama::kind::primitive),
};
static const auto f_callsig = yama::make_callsig_info({ 0, 1, 2 }, 1);
static const yama::function_info f{
    "f"_str,
    std::make_optional(f_callsig),
    f_linksyms,
};

// the type 'bad' will fail static verification during push

static const std::vector<yama::linksym> bad_linksyms{
    yama::make_linksym("a"_str, yama::kind::primitive),
    yama::make_linksym("b"_str, yama::kind::primitive),
    yama::make_linksym("c"_str, yama::kind::primitive),
};
static const auto bad_callsig = yama::make_callsig_info({ 0, 7, 2 }, 1); // <- link index 7 is out-of-bounds!
static const yama::function_info bad{
    "bad"_str,
    std::make_optional(bad_callsig),
    bad_linksyms,
};


// IMPORTANT:
//      these tests DO NOT cover get_mas AT ALL, and so each yama::domain impl must
//      test its proper functioning according to its own semantics


// IMPORTANT:
//      get_type tests presume that since type_instantiator is so well tested, that
//      so long as *basic* behaviour can be *broadly* ensured, that it can be presumed
//      that type_instantiator is the mechanism by which this behaviour is implemented,
//      such that its guarantees can be presumed to likewise apply here

TEST_P(DomainImplTests, GetType) {
    const auto dm = GetParam().factory(dbg);

    ASSERT_TRUE(dm->push(decltype(f)(f)));
    ASSERT_TRUE(dm->push(decltype(a)(a)));
    ASSERT_TRUE(dm->push(decltype(b)(b)));
    ASSERT_TRUE(dm->push(decltype(c)(c)));


    const auto result_f = dm->get_type("f"_str);
    const auto result_a = dm->get_type("a"_str);
    const auto result_b = dm->get_type("b"_str);
    const auto result_c = dm->get_type("c"_str);

    EXPECT_TRUE(result_f);
    EXPECT_TRUE(result_a);
    EXPECT_TRUE(result_b);
    EXPECT_TRUE(result_c);

    if (result_f) {
        EXPECT_TRUE(result_f->complete());
        EXPECT_EQ(result_f->fullname(), "f"_str);
        EXPECT_EQ(result_f->kind(), yama::kind::function);
        
        EXPECT_TRUE(result_f->callsig());
        if (result_f->callsig()) EXPECT_EQ(result_f->callsig().value(), yama::callsig(f_callsig, result_f->links()));

        EXPECT_EQ(result_f->links().size(), 3);
        if (result_f->links().size() == 3) {
            if (result_f->links()[0]) EXPECT_EQ(result_f->links()[0].value(), *result_a);
            if (result_f->links()[1]) EXPECT_EQ(result_f->links()[1].value(), *result_b);
            if (result_f->links()[2]) EXPECT_EQ(result_f->links()[2].value(), *result_c);
        }
    }

    if (result_a) {
        EXPECT_TRUE(result_a->complete());
        EXPECT_EQ(result_a->fullname(), "a"_str);
        EXPECT_EQ(result_a->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_a->callsig());
        EXPECT_EQ(result_a->links().size(), 0);
    }
    
    if (result_b) {
        EXPECT_TRUE(result_b->complete());
        EXPECT_EQ(result_b->fullname(), "b"_str);
        EXPECT_EQ(result_b->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_b->callsig());
        EXPECT_EQ(result_b->links().size(), 0);
    }
    
    if (result_c) {
        EXPECT_TRUE(result_c->complete());
        EXPECT_EQ(result_c->fullname(), "c"_str);
        EXPECT_EQ(result_c->kind(), yama::kind::primitive);
        EXPECT_FALSE(result_c->callsig());
        EXPECT_EQ(result_c->links().size(), 0);
    }
}

TEST_P(DomainImplTests, GetType_FailDueToInstantiationError) {
    const auto dm = GetParam().factory(dbg);

    // f will fail instantiation due to type b not being available

    ASSERT_TRUE(dm->push(decltype(f)(f)));
    ASSERT_TRUE(dm->push(decltype(a)(a)));
    //ASSERT_TRUE(dm->push(decltype(b)(b)));
    ASSERT_TRUE(dm->push(decltype(c)(c)));


    const auto result_f = dm->get_type("f"_str);
    const auto result_a = dm->get_type("a"_str);
    const auto result_c = dm->get_type("c"_str);


    // we don't care about the details of types a and c

    ASSERT_FALSE(result_f);
    ASSERT_TRUE(result_a);
    ASSERT_TRUE(result_c);
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

    ASSERT_TRUE(dm->push(f));
    ASSERT_TRUE(dm->push(a));
    ASSERT_TRUE(dm->push(b));
    ASSERT_TRUE(dm->push(c));

    // test that pushed types are acknowledged by the domain impl correctly

    const auto ff = dm->get_type("f"_str);
    const auto aa = dm->get_type("a"_str);
    const auto bb = dm->get_type("b"_str);
    const auto cc = dm->get_type("c"_str);
    ASSERT_TRUE(ff);
    ASSERT_TRUE(aa);
    ASSERT_TRUE(bb);
    ASSERT_TRUE(cc);

    // quickly sample state of types f and a to see that they pushed correctly

    EXPECT_TRUE(ff->complete());
    EXPECT_EQ(ff->fullname(), "f"_str);
    EXPECT_EQ(ff->kind(), yama::kind::function);

    ASSERT_TRUE(ff->callsig());
    EXPECT_EQ(ff->callsig().value(), yama::callsig(f_callsig, ff->links()));
    
    ASSERT_EQ(ff->links().size(), 3);
    if (ff->links()[0]) EXPECT_EQ(ff->links()[0].value(), *aa);
    if (ff->links()[1]) EXPECT_EQ(ff->links()[1].value(), *bb);
    if (ff->links()[2]) EXPECT_EQ(ff->links()[2].value(), *cc);

    EXPECT_TRUE(aa->complete());
    EXPECT_EQ(aa->fullname(), "a"_str);
    EXPECT_EQ(aa->kind(), yama::kind::primitive);
    EXPECT_FALSE(aa->callsig());
    EXPECT_EQ(aa->links().size(), 0);
}

TEST_P(DomainImplTests, Push_FailDueToStaticVerificationError) {
    const auto dm = GetParam().factory(dbg);

    EXPECT_FALSE(dm->push(bad));

    // test that no new type was made available

    EXPECT_FALSE(dm->get_type("bad"_str));
}

