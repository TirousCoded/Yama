

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/context.h>
#include <yama/debug-impls/stderr_debug.h>
#include <yama/mas-impls/heap_mas.h>
#include <yama/domain-impls/domain_st.h>


using namespace yama::string_literals;


// globals will help us launder data out of call behaviour functions

struct CallStateSnapshot final {
    yama::const_table consts;
    size_t crashes;
    bool is_crashing;
    bool is_user;
    size_t max_call_frames; // max call stk height
    size_t call_frames; // call stk height
    size_t args; // args called w/
    size_t locals; // local register table size
    
    std::optional<yama::object_ref> arg0; // call object
    std::optional<yama::object_ref> arg1;
    std::optional<yama::object_ref> arg2;
    std::optional<yama::object_ref> arg3;
    std::optional<yama::object_ref> arg4;

    std::optional<yama::object_ref> local0;
    std::optional<yama::object_ref> local1;
    std::optional<yama::object_ref> local2;
    std::optional<yama::object_ref> local3;
    std::optional<yama::object_ref> local4;
    std::optional<yama::object_ref> local5;
    std::optional<yama::object_ref> local6;
    std::optional<yama::object_ref> local7;
    std::optional<yama::object_ref> local8;
    std::optional<yama::object_ref> local9;
    std::optional<yama::object_ref> local10;
    std::optional<yama::object_ref> local11;
    std::optional<yama::object_ref> local12;
    std::optional<yama::object_ref> local13;
    std::optional<yama::object_ref> local14;
    std::optional<yama::object_ref> local15;


    static CallStateSnapshot make(yama::context& ctx, yama::const_table consts) {
        return CallStateSnapshot{
            consts,
            ctx.ll_crashes(),
            ctx.ll_crashing(),
            ctx.ll_is_user(),
            ctx.ll_max_call_frames(),
            ctx.ll_call_frames(),
            ctx.ll_args(),
            ctx.ll_locals(),
            ctx.ll_arg(0),
            ctx.ll_arg(1),
            ctx.ll_arg(2),
            ctx.ll_arg(3),
            ctx.ll_arg(4),
            ctx.ll_local(0),
            ctx.ll_local(1),
            ctx.ll_local(2),
            ctx.ll_local(3),
            ctx.ll_local(4),
            ctx.ll_local(5),
            ctx.ll_local(6),
            ctx.ll_local(7),
            ctx.ll_local(8),
            ctx.ll_local(9),
            ctx.ll_local(10),
            ctx.ll_local(11),
            ctx.ll_local(12),
            ctx.ll_local(13),
            ctx.ll_local(14),
            ctx.ll_local(15),
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
    
    yama::ctx_config config;
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::mas> mas;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


    bool build_push_and_load_f_type_for_call_tests();
    bool build_push_and_load_g_type_for_call_tests();


protected:

    void SetUp() override final {
        globals = Globals{};

        config = yama::ctx_config{
            .max_call_frames = 17,
            .user_locals = 13,
        };
        dbg = std::make_shared<yama::stderr_debug>();
        mas = std::make_shared<yama::heap_mas>(dbg);
        dm = std::make_shared<yama::domain_st>(yama::res(mas), dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), config, dbg);
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

TEST_F(ContextTests, GetConfig) {
    EXPECT_EQ(ctx->get_config(), config);
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
    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_args(), 0);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_arg(0), std::nullopt); // out-of-bounds

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(3), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(4), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(5), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(6), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(7), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(8), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(9), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(10), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(11), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(12), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(13), std::nullopt); // out-of-bounds
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

TEST_F(ContextTests, LLArg_OutsideOfCall) {
    EXPECT_EQ(ctx->ll_arg(0), std::nullopt); // no args
}

TEST_F(ContextTests, LLArg_InsideOfCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_ret(0).bad()) return; // return None object
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 0),
            .call_fn = f_call_fn,
            .locals = 3,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 31).good()); // argument #1
    ASSERT_TRUE(ctx->ll_load_float(2, 1.33).good()); // argument #2
    ASSERT_TRUE(ctx->ll_call(0, 3, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        // ss.arg# is sampled via ll_arg, so below tests its usage
        EXPECT_EQ(ss.arg0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.arg1, std::make_optional(ctx->ll_new_int(31)));
        EXPECT_EQ(ss.arg2, std::make_optional(ctx->ll_new_float(1.33)));
    }
}

TEST_F(ContextTests, LLLocal_OutsideOfCall) {
    ASSERT_TRUE(ctx->ll_load_int(0, 31).good());
    ASSERT_TRUE(ctx->ll_load_char(10, U'y').good());

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(31)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(3), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(4), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(5), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(6), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(7), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(8), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(9), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(10), std::make_optional(ctx->ll_new_char(U'y')));
    EXPECT_EQ(ctx->ll_local(11), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(12), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(13), std::nullopt); // out-of-bounds
}

TEST_F(ContextTests, LLLocal_InsideOfCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_ret(0).bad()) return; // return None object
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .locals = 3,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 31).good()); // argument
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        // ss.local# is sampled via ll_local, so below tests its usage
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local3, std::nullopt);
    }
}

TEST_F(ContextTests, LLCrash) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        ctx.ll_crash();
        globals.snapshot_1 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        ctx.ll_crash(); // should fail quietly
        globals.snapshot_2 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

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
    EXPECT_FALSE(ctx->ll_crashing());

    // don't need to test ll_crash failing quietly, as another ll_crash
    // call *should* succeed here, as the initial crash will have 
    // completed by time the first ll_crash returned

    ctx->ll_crash();

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);
}

TEST_F(ContextTests, LLCrash_MultiLevelCallStack) {
    // outer call
    const yama::const_table_info fa_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("fb"_str, yama::make_callsig_info({}, 0));
    auto fa_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_fn(0, consts.type(1).value()).bad()) return;
        if (ctx.ll_call(0, 1, yama::no_result).bad()) return;
        // prepare return obj (which won't be reached due to crash)
        if (ctx.ll_load_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        globals.even_reached_0 = true;
        };
    yama::type_info fa_info{
        .fullname = "fa"_str,
        .consts = fa_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fa_call_fn,
            .locals = 4,
        },
    };
    // inner call
    const yama::const_table_info fb_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto fb_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        ctx.ll_crash();
        globals.snapshot_1 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        ctx.ll_crash(); // should fail quietly
        globals.snapshot_2 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        };
    yama::type_info fb_info{
        .fullname = "fb"_str,
        .consts = fb_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fb_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(fa_info));
    ASSERT_TRUE(dm->push(fb_info));
    ASSERT_TRUE(dm->load("fa"_str));
    ASSERT_TRUE(dm->load("fb"_str));

    const yama::type fa = dm->load("fa"_str).value();
    const yama::type fb = dm->load("fb"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());

    ASSERT_TRUE(ctx->ll_load_fn(0, fa).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_FALSE(ctx->ll_crashing()); // crash already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

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

TEST_F(ContextTests, LLLoad_OutsideOfCall) {
    ASSERT_TRUE(ctx->ll_load(0, ctx->ll_new_int(-14)).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLLoad_InsideOfCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load(0, ctx.ll_new_int(-14)).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 2,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_int(-14)));
    }
}

TEST_F(ContextTests, LLLoad_CrashIfXIsOutOfBounds) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load(70, ctx->ll_new_none()).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

// NOTE: for the LLLoad# tests below, for each ll_load_# method, we
//       presume that they wrap calls to ll_load, and thus need-not
//       be tested in regards to things like x out-of-bounds
//
//       notice this only applies to the ll_load_# methods wrapping
//       compositions w/ ll_new_# method calls, and does NOT include
//       ll_load_arg, which is tested *fully*

TEST_F(ContextTests, LLLoadNone) {
    ASSERT_TRUE(ctx->ll_load_none(0).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_none()));
}

TEST_F(ContextTests, LLLoadInt) {
    ASSERT_TRUE(ctx->ll_load_int(0, -14).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLLoadUInt) {
    ASSERT_TRUE(ctx->ll_load_uint(0, 14).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_uint(14)));
}

TEST_F(ContextTests, LLLoadFloat) {
    ASSERT_TRUE(ctx->ll_load_float(0, 14.3).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_float(14.3)));
}

TEST_F(ContextTests, LLLoadBool) {
    ASSERT_TRUE(ctx->ll_load_bool(0, true).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_bool(true)));
}

TEST_F(ContextTests, LLLoadChar) {
    ASSERT_TRUE(ctx->ll_load_char(0, U'y').good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_char(U'y')));
}

TEST_F(ContextTests, LLLoadFn) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_fn(f)));
}

TEST_F(ContextTests, LLLoadFn_CrashIfFIsNotCallable) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, ctx->load_bool()).bad()); // Bool is not callable!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLLoadConst) {
    static_assert(
        []() constexpr -> bool {
            // NOTE: extend this each time we add a new loadable object constant
            static_assert(yama::const_types == 7);
            return
                yama::is_object_const(yama::int_const) &&
                yama::is_object_const(yama::uint_const) &&
                yama::is_object_const(yama::float_const) &&
                yama::is_object_const(yama::bool_const) &&
                yama::is_object_const(yama::char_const) &&
                !yama::is_object_const(yama::primitive_type_const) &&
                yama::is_object_const(yama::function_type_const);
        }());
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_function_type("f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        // I wanna have the x and c args differ so the impl can't so easily get away w/
        // confusing the two (as actually happened to be on initial impl of ll_load_const)
        if (ctx.ll_load_const(0, 1).bad()) return;
        if (ctx.ll_load_const(1, 2).bad()) return;
        if (ctx.ll_load_const(2, 3).bad()) return;
        if (ctx.ll_load_const(3, 4).bad()) return;
        if (ctx.ll_load_const(4, 5).bad()) return;
        if (ctx.ll_load_const(5, 6).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_load_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = yama::const_types + 10, // just have a bunch of objects for us to use
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_int(-4)));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_uint(301)));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_float(3.14159)));
        EXPECT_EQ(ss.local3, std::make_optional(ctx->ll_new_bool(true)));
        EXPECT_EQ(ss.local4, std::make_optional(ctx->ll_new_char(U'y')));
        EXPECT_EQ(ss.local5, std::make_optional(ctx->ll_new_fn(f)));
    }
}

TEST_F(ContextTests, LLLoadConst_CrashIfInUserCallFrame) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    // the user call frame has no constants, so it's not really possible to test
    // this w/out having the constant be out-of-bounds

    ASSERT_TRUE(ctx->ll_load_const(0, 0).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLLoadConst_CrashIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_const(70, 0).bad()) return; // local register index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to crash
        if (ctx.ll_load_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLLoadConst_CrashIfCIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_const(0, 70).bad()) return; // constant index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to crash
        if (ctx.ll_load_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLLoadConst_CrashIfCIsNotIndexOfAnObjectConstant) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_const(0, 0).bad()) return; // constant at index 0 is NOT an object constant
        globals.even_reached_0 = true; // shouldn't reach due to crash
        if (ctx.ll_load_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLLoadArg) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_arg(0, 0).bad()) return;
        if (ctx.ll_load_arg(1, 1).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .locals = 2,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, -14).good()); // argument
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local0, ctx->ll_new_fn(f));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(-14)));
    }
}

TEST_F(ContextTests, LLLoadArg_CrashIfInUserCallFrame) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    // the user call frame has no arguments, so it's not really possible to test
    // this w/out having the arg param be out-of-bounds

    ASSERT_TRUE(ctx->ll_load_arg(0, 0).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLLoadArg_CrashIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_arg(70, 0).bad()) return; // 70 is out-of-bounds
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, -14).good()); // argument
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLLoadArg_CrashIfArgIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_arg(0, 70).bad()) return; // 70 is out-of-bounds
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, -14).good()); // argument
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::no_result).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLCopy_OutsideOfCall) {
    ASSERT_TRUE(ctx->ll_load_int(0, -14).good());

    ASSERT_TRUE(ctx->ll_copy(0, 1).good());

    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLCopy_InsideOfCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_int(0, -14).bad()) return;
        if (ctx.ll_copy(0, 1).bad()) return;
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 2,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call(0, 1, yama::no_result).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_int(-14)));
    }
}

TEST_F(ContextTests, LLCopy_OverwritesStateOfDest) {
    // ll_copy is to overwrite uint 301 w/ a int -4

    ASSERT_TRUE(ctx->ll_load_int(0, -4).good());
    ASSERT_TRUE(ctx->ll_load_uint(1, 301).good()); // old obj state
    
    ASSERT_TRUE(ctx->ll_copy(0, 1).good());

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLCopy_CrashIfSrcOutOfBounds) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_copy(70, 1).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLCopy_CrashIfDestIsOutOfBounds) {
    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_copy(0, 70).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

bool ContextTests::build_push_and_load_f_type_for_call_tests() {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred
        globals.snapshot_0 = std::make_optional(CallStateSnapshot::make(ctx, consts));

        if (ctx.ll_load_arg(0, 1).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .locals = 3,
        },
    };
    return
        dm->push(f_info) &&
        dm->load("f"_str); // <- skip tests if we can't even load f properly
}

bool ContextTests::build_push_and_load_g_type_for_call_tests() {
    const yama::const_table_info g_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_function_type("f"_str, yama::make_callsig_info({ 0 }, 0));
    auto g_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        if (ctx.ll_load_fn(0, consts.type(1).value()).bad()) return; // callobj (ie. of type f)
        if (ctx.ll_load_arg(1, 1).bad()) return; // argument
        if (ctx.ll_load_int(2, 0).bad()) return; // result
        if (ctx.ll_call(0, 2, 3).bad()) return; // just call f and return
        if (ctx.ll_ret(3).bad()) return;
        };
    yama::type_info g_info{
        .fullname = "g"_str,
        .consts = g_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = g_call_fn,
            .locals = 5,
        },
    };
    return
        dm->push(g_info) &&
        dm->load("g"_str); // <- skip tests if we can't even load g properly
}

// NOTE: the below tests sorta conflate the two ll_call overloads,
//       w/ them presuming that the impl of them share the same
//       underlying core behaviour

TEST_F(ContextTests, LLCall) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument
    ASSERT_TRUE(ctx->ll_load_int(2, 0).good()); // result
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, config.max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.args, 2);
        EXPECT_EQ(ss.locals, 3);

        EXPECT_EQ(ss.arg0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.arg1, std::make_optional(ctx->ll_new_int(3)));
        EXPECT_EQ(ss.arg2, std::nullopt);

        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local3, std::nullopt);
    }
}

TEST_F(ContextTests, LLCall_MultiLevelCallStack) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_g_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type g = dm->load("g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->ll_load_fn(0, g).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument
    ASSERT_TRUE(ctx->ll_load_int(2, 0).good()); // result
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(3)));

    // call to g should have called f

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, config.max_call_frames);
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.args, 2);
        EXPECT_EQ(ss.locals, 3);

        EXPECT_EQ(ss.arg0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.arg1, std::make_optional(ctx->ll_new_int(3)));
        EXPECT_EQ(ss.arg2, std::nullopt);

        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local3, std::nullopt);
    }
}

TEST_F(ContextTests, LLCall_OverwritesStateOfRet) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, -4).good()); // argument
    // ll_call is to overwrite uint 301 w/ a int -4
    ASSERT_TRUE(ctx->ll_load_uint(2, 301).good()); // result (ie. overwrite old obj state)

    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLCall_NoResultOverload) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::no_result).good()); // will output no result object

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->load_int());
        }
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.max_call_frames, config.max_call_frames);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.args, 2);
        EXPECT_EQ(ss.locals, 3);

        EXPECT_EQ(ss.arg0, std::make_optional(ctx->ll_new_fn(f).value()));
        EXPECT_EQ(ss.arg1, std::make_optional(ctx->ll_new_int(3)));
        EXPECT_EQ(ss.arg2, std::nullopt);

        EXPECT_EQ(ss.local0, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local1, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local2, std::make_optional(ctx->ll_new_none()));
        EXPECT_EQ(ss.local3, std::nullopt);
    }
}

TEST_F(ContextTests, LLCall_CrashIfArgsProvidesNoCallObj) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true;

        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 2,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 0, 0).bad()); // no callobj -> crash

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfArgsIndexRangeIsOutOfBounds_DueToBadArgStart) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument
    ASSERT_TRUE(ctx->ll_load_int(2, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(70, 2, 2).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfArgsIndexRangeIsOutOfBounds_DueToBadArgN) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument
    ASSERT_TRUE(ctx->ll_load_int(2, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 70, 2).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfRetIsOutOfBounds) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument
    ASSERT_TRUE(ctx->ll_load_int(2, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 70).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfCallObjIsNotCallableType) {
    ASSERT_TRUE(ctx->ll_load_float(0, 13.02).good()); // call object, but float can't be!
    ASSERT_TRUE(ctx->ll_load_int(1, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLCall_CrashIfUnexpectedArgCount_TooManyArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 3).good()); // argument #1
    ASSERT_TRUE(ctx->ll_load_int(2, 3).good()); // argument #2
    ASSERT_TRUE(ctx->ll_load_int(3, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 3, 3).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfUnexpectedArgCount_TooFewArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 0).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_CrashIfCallStackOverflow) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and crash
        if (ctx.ll_load_fn(0, consts.type(1).value()).bad()) return; // call object
        if (ctx.ll_load_none(1).bad()) return; // result
        if (ctx.ll_call(0, 1, 2).bad()) return;
        if (ctx.ll_ret(2).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 3,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_none(1).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
    EXPECT_EQ(globals.call_depth, config.max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, LLCall_CrashIfNoReturnValueObjectProvided) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true;
        // return w/out ever calling ll_ret
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_none(1).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause crash

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_TRUE(globals.even_reached_0); // unlike others, this crash doesn't prevent call behaviour
}

// NOTE: most usage of ll_ret will be tested as part of testing ll_call

TEST_F(ContextTests, LLRet_AllowsReturningWrongTypedObject) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred

        // tell ll_ret to return a UInt, even though f should return a Int,
        // but the VM will allow for returning the wrong typed object
        
        // checking return type of return value object is beyond the scope 
        // of this level of abstraction, and so for safety this is allowed
        // as a almost a kind of *feature* of the low-level command interface

        if (ctx.ll_load_uint(0, 301).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 3,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_int(1, 0).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_crashes(), 0);
    EXPECT_FALSE(ctx->ll_crashing());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_max_call_frames(), config.max_call_frames);
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_locals(), config.user_locals);

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_uint(301)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
}

TEST_F(ContextTests, LLRet_CrashIfInUserCallFrame) {
    ASSERT_TRUE(ctx->ll_is_user());

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_ret(0).bad());

    EXPECT_EQ(ctx->ll_crashes(), 1);
}

TEST_F(ContextTests, LLRet_CrashIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true;
        if (ctx.ll_ret(70).bad()) return; // 70 is out-of-bounds
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_none(1).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause crash

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

TEST_F(ContextTests, LLRet_CrashIfCalledMultipleTimesInOneCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx, yama::const_table consts) {
        globals.even_reached_0 = true;
        if (ctx.ll_ret(0).bad()) return; // okay
        if (ctx.ll_ret(0).bad()) return; // illegal
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .locals = 1,
        },
    };
    ASSERT_TRUE(dm->push(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good()); // call object
    ASSERT_TRUE(ctx->ll_load_none(1).good()); // result

    EXPECT_EQ(ctx->ll_crashes(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause crash

    EXPECT_EQ(ctx->ll_crashes(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

