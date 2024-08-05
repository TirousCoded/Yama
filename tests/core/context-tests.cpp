

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/debug-impls/stderr_debug.h>
#include <yama/mas-impls/heap_mas.h>
#include <yama/domain-impls/domain_st.h>


using namespace yama::string_literals;


// globals will help us launder data out of call behaviour functions

struct CallStateSnapshot final {
    yama::links_view links;
    size_t crashes;
    bool is_crashing;
    bool is_user;
    size_t max_call_frames; // max call stk height
    size_t call_frames; // call stk height
    size_t max_locals; // max local obj stk height
    size_t locals; // local obj stk height
    std::optional<yama::object_ref> local0; // call object
    std::optional<yama::object_ref> local1;
    std::optional<yama::object_ref> local2;
    std::optional<yama::object_ref> local3;
    std::optional<yama::object_ref> local4;


    static CallStateSnapshot make(yama::context& ctx, yama::links_view links) {
        return CallStateSnapshot{
            links,
            ctx.ll_crashes(),
            ctx.ll_is_crashing(),
            ctx.ll_is_user(),
            ctx.ll_max_call_frames(),
            ctx.ll_call_frames(),
            ctx.ll_max_locals(),
            ctx.ll_locals(),
            ctx.ll_local(0),
            ctx.ll_local(1),
            ctx.ll_local(2),
            ctx.ll_local(3),
            ctx.ll_local(4),
        };
    }
};

static struct Globals final {
    yama::int_t int_arg_value_called_with = 0;

    bool even_reached_0 = false;

    size_t call_depth = 0;

    std::optional<CallStateSnapshot> snapshot_0;
    std::optional<CallStateSnapshot> snapshot_1;
    std::optional<CallStateSnapshot> snapshot_2;
} globals;


class ContextTests : public testing::Test {
public:

    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::mas> mas;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


    bool build_push_and_load_f_type_for_call_tests();
    bool build_push_and_load_g_type_for_call_tests();


protected:

    void SetUp() override final {
        globals = Globals{};

        dbg = std::make_shared<yama::stderr_debug>();
        mas = std::make_shared<yama::heap_mas>(dbg);
        dm = std::make_shared<yama::domain_st>(yama::res(mas), dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), dbg);
    }

    void TearDown() override final {
        //
    }
};


TEST_F(ContextTests, GetDomain) {
    EXPECT_EQ(ctx->get_domain(), dm);
}

TEST_F(ContextTests, DM) {
    EXPECT_EQ(&(ctx->dm()), dm.get());
}

TEST_F(ContextTests, GetMAS) {
    EXPECT_EQ(ctx->get_mas(), mas);
}

TEST_F(ContextTests, Load) {
    // just quick-n'-dirty use this to get type f
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());

    // expect load to be equiv to forward to domain

    EXPECT_EQ(ctx->load("None"_str), dm->load("None"_str));
    EXPECT_EQ(ctx->load("Int"_str), dm->load("Int"_str));
    EXPECT_EQ(ctx->load("f"_str), dm->load("f"_str));

    EXPECT_EQ(ctx->load("missing"_str), dm->load("missing"_str)); // test equiv failure #1

    // ensure above failure test actually failed

    EXPECT_FALSE(ctx->load("missing"_str));
}

TEST_F(ContextTests, LoadNone) {
    EXPECT_EQ(ctx->load_none(), dm->load_none());
}

TEST_F(ContextTests, LoadInt) {
    EXPECT_EQ(ctx->load_int(), dm->load_int());
}

TEST_F(ContextTests, LoadUInt) {
    EXPECT_EQ(ctx->load_uint(), dm->load_uint());
}

TEST_F(ContextTests, LoadFloat) {
    EXPECT_EQ(ctx->load_float(), dm->load_float());
}

TEST_F(ContextTests, LoadBool) {
    EXPECT_EQ(ctx->load_bool(), dm->load_bool());
}

TEST_F(ContextTests, LoadChar) {
    EXPECT_EQ(ctx->load_char(), dm->load_char());
}


// LOW-LEVEL COMMAND INTERFACE TESTS

// TODO: add tests for 'll_#_ref' methods later

TEST_F(ContextTests, ObjectRef_Equality_None) {
    const yama::object_ref a = ctx->ll_new_none();
    const yama::object_ref b = ctx->ll_new_none();

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);

    EXPECT_EQ(b, b);
}

TEST_F(ContextTests, ObjectRef_Equality_Int) {
    const yama::object_ref a = ctx->ll_new_int(-41);
    const yama::object_ref b = ctx->ll_new_int(-41);
    const auto c = ctx->ll_new_int(106);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_UInt) {
    const yama::object_ref a = ctx->ll_new_uint(41);
    const yama::object_ref b = ctx->ll_new_uint(41);
    const auto c = ctx->ll_new_uint(106);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Float) {
    const yama::object_ref a = ctx->ll_new_float(-41.19);
    const yama::object_ref b = ctx->ll_new_float(-41.19);
    const auto c = ctx->ll_new_float(106.141);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Bool) {
    const yama::object_ref a = ctx->ll_new_bool(true);
    const yama::object_ref b = ctx->ll_new_bool(true);
    const auto c = ctx->ll_new_bool(false);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Char) {
    const yama::object_ref a = ctx->ll_new_char(U'a');
    const yama::object_ref b = ctx->ll_new_char(U'a');
    const auto c = ctx->ll_new_char(U'魂');

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Functions) {
    // remember that each fn type used w/ ll_new_fn creates a object_ref
    // of a DIFFERENT TYPE, so the *correct* place to test inequality
    // between differing function typed object_ref is below in the
    // ObjectRef_Equality_InequalityIfTypesDiffer test

    // just quick-n'-dirty use this to get type f
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = ctx->load("f"_str).value();

    const yama::object_ref a = ctx->ll_new_fn(f).value();
    const yama::object_ref b = ctx->ll_new_fn(f).value();

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);

    EXPECT_EQ(b, b);
}

TEST_F(ContextTests, ObjectRef_Equality_InequalityIfTypesDiffer) {
    // just quick-n'-dirty use this to get types f and g
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_g_type_for_call_tests());
    const yama::type f = ctx->load("f"_str).value();
    const yama::type g = ctx->load("g"_str).value();

    const auto _Int = ctx->ll_new_int(0);
    const auto _UInt = ctx->ll_new_uint(0);
    const auto _Float = ctx->ll_new_float(0.0);
    const auto _Bool = ctx->ll_new_bool(false);
    const auto _Char = ctx->ll_new_char(U'\0');

    const yama::object_ref _fn_f = ctx->ll_new_fn(f).value();
    const yama::object_ref _fn_g = ctx->ll_new_fn(g).value();

    EXPECT_NE(_Int, _UInt);
    EXPECT_NE(_Int, _Float);
    EXPECT_NE(_Int, _Bool);
    EXPECT_NE(_Int, _Char);
    EXPECT_NE(_Int, _fn_f);
    EXPECT_NE(_Int, _fn_g);

    EXPECT_NE(_UInt, _Float);
    EXPECT_NE(_UInt, _Bool);
    EXPECT_NE(_UInt, _Char);
    EXPECT_NE(_UInt, _fn_f);
    EXPECT_NE(_UInt, _fn_g);

    EXPECT_NE(_Float, _Bool);
    EXPECT_NE(_Float, _Char);
    EXPECT_NE(_Float, _fn_f);
    EXPECT_NE(_Float, _fn_g);

    EXPECT_NE(_Bool, _Char);
    EXPECT_NE(_Bool, _fn_f);
    EXPECT_NE(_Bool, _fn_g);

    EXPECT_NE(_Char, _fn_f);
    EXPECT_NE(_Char, _fn_g);

    EXPECT_NE(_fn_f, _fn_g);
}

TEST_F(ContextTests, InitialCtxState) {
    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 0);
}

TEST_F(ContextTests, LLNewNone) {
    const yama::object_ref a = ctx->ll_new_none();

    EXPECT_EQ(a.t, dm->load_none());

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewInt) {
    const yama::object_ref a = ctx->ll_new_int(-41);

    EXPECT_EQ(a.t, dm->load_int());
    EXPECT_EQ(a.as_int(), -41);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewUInt) {
    const yama::object_ref a = ctx->ll_new_uint(106);

    EXPECT_EQ(a.t, dm->load_uint());
    EXPECT_EQ(a.as_uint(), 106);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewFloat) {
    const yama::object_ref a = ctx->ll_new_float(10.6);

    EXPECT_EQ(a.t, dm->load_float());
    EXPECT_EQ(a.as_float(), 10.6);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewBool) {
    const yama::object_ref a = ctx->ll_new_bool(true);

    EXPECT_EQ(a.t, dm->load_bool());
    EXPECT_EQ(a.as_bool(), true);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewChar) {
    const yama::object_ref a = ctx->ll_new_char(U'魂');

    EXPECT_EQ(a.t, dm->load_char());
    EXPECT_EQ(a.as_char(), U'魂');

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, LLNewFn) {
    // just quick-n'-dirty use this to get type f
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    const auto a = ctx->ll_new_fn(f);

    ASSERT_TRUE(a);
    EXPECT_EQ(a->t, f);
}

TEST_F(ContextTests, LLNewFn_FailDueToArgNotAFunctionType) {
    EXPECT_FALSE(ctx->ll_new_fn(dm->load_int())); // Int is not a function type
}

TEST_F(ContextTests, LLLocal) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(31), yama::newtop).good());
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_char(U'y'), yama::newtop).good());

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(31)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_char(U'y')));
    EXPECT_EQ(ctx->ll_local(2), std::nullopt);
}

TEST_F(ContextTests, LLLocal_InsideOfCall) {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("Int"_str, yama::kind::primitive),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));
        // NOTE: first arg of the call will be used as its return value
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({ 0 }, 0)),
        f_linksyms,
        f_call_fn,
        3,
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(31), yama::newtop).good());
    ASSERT_TRUE(ctx->ll_call(f, { 0 }, yama::forget).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(31)));
        EXPECT_EQ(ss.local2, std::nullopt);
    }
}

TEST_F(ContextTests, LLCrash) {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));
        ctx.ll_crash();
        globals.snapshot_1 = std::make_optional(CallStateSnapshot::make(ctx, links));
        ctx.ll_crash(); // should fail quietly
        globals.snapshot_2 = std::make_optional(CallStateSnapshot::make(ctx, links));
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        f_linksyms,
        f_call_fn,
        1,
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_is_crashing());

    ASSERT_TRUE(ctx->ll_call(f, {}, yama::forget).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_is_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 0);

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.crashes, 0);
        EXPECT_FALSE(ss.is_crashing);
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.crashes, 1);
        EXPECT_TRUE(ss.is_crashing);
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.crashes, 1); // didn't incr
        EXPECT_TRUE(ss.is_crashing);
    }
}

TEST_F(ContextTests, LLCrash_OutsideOfCall) {
    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_is_crashing());

    // don't need to test ll_crash failing quietly, as another ll_crash
    // call *should* succeed here, as the initial crash will have 
    // completed by time the first ll_crash returned

    ctx->ll_crash();

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_is_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 0);
}

TEST_F(ContextTests, LLCrash_MultiLevelCallStack) {
    // outer call
    std::vector<yama::linksym> fa_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
        yama::make_linksym("fb"_str, yama::kind::function, yama::make_callsig_info({}, 0)),
    };
    auto fa_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        if (ctx.ll_call(links[1].value(), {}, yama::forget).bad()) return;
        if (ctx.ll_put(ctx.ll_new_none(), yama::newtop).bad()) return;
        globals.even_reached_0 = true;
        };
    yama::function_info fa_info{
        "fa"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        fa_linksyms,
        fa_call_fn,
        2,
    };
    // inner call
    std::vector<yama::linksym> fb_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
    };
    auto fb_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));
        ctx.ll_crash();
        globals.snapshot_1 = std::make_optional(CallStateSnapshot::make(ctx, links));
        ctx.ll_crash(); // should fail quietly
        globals.snapshot_2 = std::make_optional(CallStateSnapshot::make(ctx, links));
        };
    yama::function_info fb_info{
        "fb"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        fb_linksyms,
        fb_call_fn,
        1,
    };
    ASSERT_TRUE(dm->push(fa_info));
    ASSERT_TRUE(dm->push(fb_info));
    ASSERT_TRUE(dm->load("fa"_str));
    ASSERT_TRUE(dm->load("fb"_str));

    const yama::type fa = dm->load("fa"_str).value();
    const yama::type fb = dm->load("fb"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_is_crashing());

    ASSERT_TRUE(ctx->ll_call(fa, {}, yama::forget).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_is_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 0);

    // outer call
    EXPECT_FALSE(globals.even_reached_0);

    // inner call
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.crashes, 0);
        EXPECT_FALSE(ss.is_crashing);
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.crashes, 1);
        EXPECT_TRUE(ss.is_crashing);
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.crashes, 1); // didn't incr
        EXPECT_TRUE(ss.is_crashing);
    }
}

// IMPORTANT:
//      in order to avoid a painful number of repeat tests for every single
//      overload, these tests are gonna make two assumptions:
//          1) under all ll_call overloads is one *core* behaviour impl which
//             governs the behaviour of all of them, w/ each overload then
//             differing only in terms of the code *mapping* arguments onto
//             the interface of this core (or in case of newtop_t, the param
//             *mapping* is responding to the core behaviour)
//          2) overloads w/ params in common have the same underlying *mapping*
//             code for said common params, meaning that testing one should be
//             enough to roughly test all of them

bool ContextTests::build_push_and_load_f_type_for_call_tests() {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("Int"_str, yama::kind::primitive),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));

        if (ctx.ll_put(1, yama::newtop).bad()) return;
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({ 0 }, 0)),
        f_linksyms,
        f_call_fn,
        3,
    };
    return
        dm->push(f_info) &&
        dm->load("f"_str); // <- skip tests if we can't even load f properly
}

bool ContextTests::build_push_and_load_g_type_for_call_tests() {
    std::vector<yama::linksym> g_linksyms{
        yama::make_linksym("Int"_str, yama::kind::primitive),
        yama::make_linksym("f"_str, yama::kind::function, yama::make_callsig_info({ 0 }, 0)),
    };
    auto g_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        if (ctx.ll_put(ctx.ll_new_fn(links[1].value()).value(), yama::newtop).bad()) return; // callobj for our f call
        if (ctx.ll_call(2, { 1 }, yama::newtop).bad()) return; // just call f and return
        };
    yama::function_info g_info{
        "g"_str,
        std::make_optional(yama::make_callsig_info({ 0 }, 0)),
        g_linksyms,
        g_call_fn,
        4,
    };
    return
        dm->push(g_info) &&
        dm->load("g"_str); // <- skip tests if we can't even load g properly
}

TEST_F(ContextTests, LLCall) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 2).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 3);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_MultiLevelCallStack) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_g_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type g = dm->load("g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(g).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 2).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 3);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    // call to g should have called f

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverwritesStateOfRet) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-4), yama::newtop).good()); // argument

    // ll_call is to overwrite uint 301 w/ a int -4

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_uint(301), yama::newtop).good()); // result (ie. overwrite old obj state)

    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 2).good());

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLCall_CrashIfArgsIndicesOutOfBounds) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 7 }, yama::newtop).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfTooManyArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1, 1 }, yama::newtop).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfTooFewArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, {}, yama::newtop).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfCallStackOverflow) {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
        yama::make_linksym("f"_str, yama::kind::function, yama::make_callsig_info({}, 0)),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and crash
        if (ctx.ll_call(links[1].value(), {}, yama::newtop).bad()) return;
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        f_linksyms,
        f_call_fn,
        2,
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(f, {}, yama::forget).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_EQ(globals.call_depth, yama::max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, LLCall_OverloadsWithParamXAsStackIndex) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 2).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 3);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverloadsWithParamXAsStackIndex_CrashIfOutOfBounds) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(7, { 1 }, 2).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamXAsStackIndex_CrashIfNotCallable) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(301), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(7, { 1 }, 2).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamXAsObjectRef) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(ctx->ll_new_fn(f).value(), { 0 }, 1).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 2);

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverloadsWithParamXAsObjectRef_CrashIfNotCallable) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(ctx->ll_new_int(301), { 0 }, 1).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamF) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(f, { 0 }, 1).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 2);

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverloadsWithParamF_CrashIfNotCallable) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(dm->load_int(), { 0 }, 1).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamRet) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 2).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 3);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverloadsWithParamRet_CrashIfOutOfBounds) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, 7).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamNewTop) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, yama::newtop).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 3);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

TEST_F(ContextTests, LLCall_OverloadsWithParamNewTop_CrashIfOverflows) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    // keep pushing *dummy* objects to saturate user local object stack
    // to force overflow by ll_call

    const size_t n = yama::user_max_locals - 2;
    YAMA_LOG(dbg, yama::general_c, "pushing {} dummy objs to saturate user local object stack...", n);
    for (size_t i = 0; i < n; i++) {
        ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good());
    }

    ASSERT_EQ(ctx->ll_locals(), yama::user_max_locals); // <- will force overflow

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, yama::newtop).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_OverloadsWithParamForget) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_fn(f).value(), yama::newtop).good()); // call object
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good()); // argument

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, { 1 }, yama::forget).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 2);

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.links.size(), 1);
        EXPECT_TRUE(ss.links[0]);
        if (ss.links[0]) {
            EXPECT_EQ(ss.links[0].value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_locals, 3);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(3)));
    }
}

// IMPORTANT:
//      in order to avoid a painful number of repeat tests for every single
//      overload, these tests are gonna make two assumptions:
//          1) under all ll_put overloads is one *core* behaviour impl which
//             governs the behaviour of all of them, w/ each overload then
//             differing only in terms of the code *mapping* arguments onto
//             the interface of this core (or in case of newtop_t, the param
//             *mapping* is responding to the core behaviour)
//          2) overloads w/ params in common have the same underlying *mapping*
//             code for said common params, meaning that testing one should be
//             enough to roughly test all of them

TEST_F(ContextTests, LLPut) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 1);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLPut_InsideOfCall) {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("Int"_str, yama::kind::primitive),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        if (ctx.ll_put(ctx.ll_new_int(-14), yama::newtop).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        f_linksyms,
        f_call_fn,
        2,
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_call(f, {}, yama::forget).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(-14)));
    }
}

TEST_F(ContextTests, LLPut_OverwritesStateOfDest) {
    // ll_put is to overwrite uint 301 w/ a int -4

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_uint(301), yama::newtop).good()); // old obj state
    
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-4), 0).good());

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLPut_OverloadsWithParamSrcAsStackIndex) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).good()); // copy this
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // target

    ASSERT_TRUE(ctx->ll_put(0, 1).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 2);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLPut_OverloadsWithParamSrcAsStackIndex_CrashIfOutOfBounds) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).good()); // copy this
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // target

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_put(7, 1).bad()); // 7 is out-of-bounds!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLPut_OverloadsWithParamSrcAsObjectRef) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // target

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), 0).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 1);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLPut_OverloadsWithParamDest) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // target

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), 0).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 1);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLPut_OverloadsWithParamDest_CrashIfOutOfBounds) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).good()); // copy this
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good()); // target

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_put(0, 7).bad()); // 7 is out-of-bounds!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLPut_OverloadsWithParamNewTop) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 1);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLPut_OverloadsWithParamNewTop_CrashIfOverflows) {
    // keep pushing *dummy* objects to saturate user local object stack
    // to force overflow by ll_put

    const size_t n = yama::user_max_locals;
    YAMA_LOG(dbg, yama::general_c, "pushing {} dummy objs to saturate user local object stack...", n);
    for (size_t i = 0; i < n; i++) {
        ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(0), yama::newtop).good());
    }

    ASSERT_EQ(ctx->ll_locals(), yama::user_max_locals); // <- will force overflow

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::newtop).bad()); // overflow!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLPut_OverloadsWithParamForget) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(-14), yama::forget).good());

    EXPECT_FALSE(ctx->ll_is_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
    EXPECT_EQ(ctx->ll_locals(), 0);
}

TEST_F(ContextTests, LLPop) {
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(1), yama::newtop).good());
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(2), yama::newtop).good());
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(3), yama::newtop).good());
    ASSERT_TRUE(ctx->ll_put(ctx->ll_new_int(4), yama::newtop).good());

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_EQ(ctx->ll_locals(), 4);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(1)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(2)));
    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));
    EXPECT_EQ(ctx->ll_local(3), std::make_optional(ctx->ll_new_int(4)));

    ASSERT_TRUE(ctx->ll_pop(3).good());

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_EQ(ctx->ll_locals(), 1);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(1)));

    ASSERT_TRUE(ctx->ll_pop(3).good()); // will run out of objs after popping 1

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_EQ(ctx->ll_locals(), 0);
}

TEST_F(ContextTests, LLPop_InsideOfCall) {
    std::vector<yama::linksym> f_linksyms{
        yama::make_linksym("None"_str, yama::kind::primitive),
    };
    auto f_call_fn =
        [](yama::context& ctx, yama::links_view links) {
        if (ctx.ll_put(ctx.ll_new_int(1), yama::newtop).bad()) return;
        if (ctx.ll_put(ctx.ll_new_int(2), yama::newtop).bad()) return;
        if (ctx.ll_put(ctx.ll_new_int(3), yama::newtop).bad()) return;
        if (ctx.ll_put(ctx.ll_new_int(4), yama::newtop).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, links));
        if (ctx.ll_pop(3).bad()) return;
        globals.snapshot_1 = std::make_optional(CallStateSnapshot::make(ctx, links));
        if (ctx.ll_pop(3).bad()) return; // will run out of objs after popping 1
        globals.snapshot_2 = std::make_optional(CallStateSnapshot::make(ctx, links));
        if (ctx.ll_put(ctx.ll_new_none(), yama::newtop).bad()) return; // return value
        };
    yama::function_info f_info{
        "f"_str,
        std::make_optional(yama::make_callsig_info({}, 0)),
        f_linksyms,
        f_call_fn,
        5,
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_call(f, {}, yama::forget).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.crashes, 0);
        EXPECT_EQ(ss.locals, 5);
        EXPECT_EQ(ss.local0, ctx->ll_new_fn(f));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(1)));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_int(2)));
        EXPECT_EQ(ss.local3, std::make_optional(ctx->ll_new_int(3)));
        EXPECT_EQ(ss.local4, std::make_optional(ctx->ll_new_int(4)));
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.crashes, 0);
        EXPECT_EQ(ss.locals, 2);
        EXPECT_EQ(ss.local0, ctx->ll_new_fn(f));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(1)));
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.crashes, 0);
        EXPECT_EQ(ss.locals, 0); // we even popped the call object
    }
}

