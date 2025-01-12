

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

TEST_P(DomainImplTests, LockedAndFork) {
    const auto dm = GetParam().factory(dbg);

    ASSERT_TRUE(dm->upload(a_info));
    ASSERT_TRUE(dm->upload(b_info));

    const auto dm_a = dm->load("a"_str);
    const auto dm_b = dm->load("b"_str);

    ASSERT_TRUE(dm_a);
    ASSERT_TRUE(dm_b);


    ASSERT_FALSE(dm->locked()); // dm not locked yet

    auto subdm = dm->fork();

    ASSERT_TRUE(subdm);
    ASSERT_FALSE(subdm->locked());
    ASSERT_EQ(subdm->upstream().lock(), std::shared_ptr<yama::domain>(dm));

    ASSERT_TRUE(dm->locked()); // dm is now locked

    // upload c and d to subdm (but dm won't see them)

    ASSERT_TRUE(subdm->upload(c_info));
    ASSERT_TRUE(subdm->upload(d_info));

    const auto subdm_a = subdm->load("a"_str);
    const auto subdm_b = subdm->load("b"_str);
    const auto subdm_c = subdm->load("c"_str);
    const auto subdm_d = subdm->load("d"_str);

    ASSERT_TRUE(subdm_a);
    ASSERT_TRUE(subdm_b);
    ASSERT_TRUE(subdm_c);
    ASSERT_TRUE(subdm_d);

    // a and b are the same in both dm and subdm

    ASSERT_EQ(dm_a.value(), subdm_a.value());
    ASSERT_EQ(dm_b.value(), subdm_b.value());


    subdm = nullptr; // destroy subdm

    ASSERT_FALSE(dm->locked()); // dm no longer locked

    // dm does not have c or d, but subdm did

    ASSERT_FALSE(dm->load("c"_str));
    ASSERT_FALSE(dm->load("d"_str));
}

TEST_P(DomainImplTests, LockedAndFork_Committing) {
    const auto dm = GetParam().factory(dbg);

    ASSERT_TRUE(dm->upload(a_info));
    ASSERT_TRUE(dm->upload(b_info)); // defer loading so we can instantiate it in subdomain

    ASSERT_TRUE(dm->load("a"_str)); // instantiate a


    ASSERT_FALSE(dm->locked());
    
    auto subdm = dm->fork();
    
    ASSERT_TRUE(subdm);

    ASSERT_TRUE(dm->locked());


    ASSERT_TRUE(subdm->upload(c_info)); // upload in subdomain

    const auto subdm_a = subdm->load("a"_str);
    const auto subdm_b = subdm->load("b"_str); // uploaded to dm, but instantiated in subdm
    const auto subdm_c = subdm->load("c"_str);

    ASSERT_TRUE(subdm_a);
    ASSERT_TRUE(subdm_b);
    ASSERT_TRUE(subdm_c);

    subdm->commit(); // commit c into upstream dm

    // d will be lost due to uncommitted

    ASSERT_TRUE(subdm->upload(d_info)); // upload in subdomain

    const auto subdm_d = subdm->load("d"_str);

    ASSERT_TRUE(subdm_d);


    subdm = nullptr; // destroy subdm

    ASSERT_FALSE(dm->locked()); // dm no longer locked


    // c is now available to dm, and b should be the same as subdm_b

    const auto dm_a = dm->load("a"_str);
    const auto dm_b = dm->load("b"_str); // uploaded to dm, but instantiated in subdm
    const auto dm_c = dm->load("c"_str);

    ASSERT_TRUE(dm_a);
    ASSERT_TRUE(dm_b);
    ASSERT_TRUE(dm_c);

    ASSERT_EQ(dm_a.value(), subdm_a.value());
    ASSERT_EQ(dm_b.value(), subdm_b.value());
    ASSERT_EQ(dm_c.value(), subdm_c.value());

    // d does not exist in dm

    ASSERT_FALSE(dm->load("d"_str));
}

TEST_P(DomainImplTests, LockedAndFork_ForkOverloadWithoutInjectedDebugLayer) {
    const auto debug_1 = yama::make_res<yama::stderr_debug>();

    const auto dm = GetParam().factory(debug_1);

    auto subdm = dm->fork();

    ASSERT_EQ(dm->dbg(), debug_1.base());
    ASSERT_EQ(subdm->dbg(), debug_1.base());

    subdm = nullptr; // destroy subdm
}

TEST_P(DomainImplTests, LockedAndFork_ForkOverloadWithoutInjectedDebugLayer_NoDebugLayer) {
    const auto dm = GetParam().factory(nullptr);

    auto subdm = dm->fork();

    ASSERT_EQ(dm->dbg(), nullptr);
    ASSERT_EQ(subdm->dbg(), nullptr);

    subdm = nullptr; // destroy subdm
}

TEST_P(DomainImplTests, LockedAndFork_ForkOverloadWithInjectedDebugLayer) {
    const auto debug_1 = yama::make_res<yama::stderr_debug>();
    const auto debug_2 = yama::make_res<yama::stderr_debug>();

    const auto dm = GetParam().factory(debug_1);

    auto subdm = dm->fork(debug_2);

    ASSERT_EQ(dm->dbg(), debug_1.base());
    ASSERT_EQ(subdm->dbg(), debug_2.base());

    subdm = nullptr; // destroy subdm
}

TEST_P(DomainImplTests, LockedAndFork_ForkOverloadWithInjectedDebugLayer_NoDebugLayer) {
    const auto debug_1 = yama::make_res<yama::stderr_debug>();

    const auto dm = GetParam().factory(debug_1);

    auto subdm = dm->fork(nullptr);

    ASSERT_EQ(dm->dbg(), debug_1.base());
    ASSERT_EQ(subdm->dbg(), nullptr);

    subdm = nullptr; // destroy subdm
}

TEST_P(DomainImplTests, LockedAndFork_SubdomainCanHandleBeingDestroyedAfterUpstreamDomainIs) {
    auto dm = GetParam().factory(dbg).base();
    auto subdm = dm->fork(); // forked subdomain

    dm = nullptr; // domain destroyed first
    subdm = nullptr; // subdomain destroyed second
}

TEST_P(DomainImplTests, Load) {
    const auto dm = GetParam().factory(dbg);

    ASSERT_TRUE(dm->upload(f_info));
    ASSERT_TRUE(dm->upload(a_info));
    ASSERT_TRUE(dm->upload(b_info));
    ASSERT_TRUE(dm->upload(c_info));


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

    ASSERT_TRUE(dm->upload(f_info));
    ASSERT_TRUE(dm->upload(a_info));
    //ASSERT_TRUE(dm->upload(b_info));
    ASSERT_TRUE(dm->upload(c_info));


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
//      upload tests presume that since static_verifier is so well tested, that so
//      long as *basic* behaviour can be *broadly* ensured, that it can be presumed
//      that static_verifier is the mechanism by which this behaviour is implemented,
//      such that its guarantees can be presumed to likewise apply here

// IMPORTANT:
//      take note also that these tests also presume that the templated overload
//      of upload working can be taken to imply that its type_data overload also works,
//      as it's presumed to be the mechanism by which the former is implemented

TEST_P(DomainImplTests, Upload_One) {
    const auto dm = GetParam().factory(dbg);

    // upload types f, a, b, and c, to test that upload works broadly

    ASSERT_TRUE(dm->upload(f_info));
    ASSERT_TRUE(dm->upload(a_info));
    ASSERT_TRUE(dm->upload(b_info));
    ASSERT_TRUE(dm->upload(c_info));

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
    const auto dm = GetParam().factory(dbg);

    EXPECT_FALSE(dm->upload(bad_info));

    // test that no new type was made available

    EXPECT_FALSE(dm->load("bad"_str));
}

TEST_P(DomainImplTests, Upload_MultiViaSpan) {
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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

TEST_P(DomainImplTests, Upload_SrcCode) {
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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
    const auto dm = GetParam().factory(dbg);

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

    const auto dm = GetParam().factory(dbg);

    ASSERT_FALSE(dm->upload(path));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_file_not_found), 1);
}

