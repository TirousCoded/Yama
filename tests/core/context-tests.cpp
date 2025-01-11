

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/domain.h>
#include <yama/core/context.h>


using namespace yama::string_literals;


// we use 'snapshots' to view the inner state of calls

struct CallStateSnapshot final {
    yama::const_table consts;
    size_t panics;
    bool panicking;
    bool is_user;
    size_t call_frames;
    size_t max_call_frames;
    size_t max_locals;
    std::vector<yama::object_ref> args, locals;
    bool out_of_bounds_arg_is_as_expected;
    bool out_of_bounds_local_is_as_expected;


    static CallStateSnapshot make(yama::context& ctx) {
        CallStateSnapshot result{
            .consts = ctx.ll_consts(),
            .panics = ctx.ll_panics(),
            .panicking = ctx.ll_panicking(),
            .is_user = ctx.ll_is_user(),
            .call_frames = ctx.ll_call_frames(),
            .max_call_frames = ctx.ll_max_call_frames(),
            .max_locals = ctx.ll_max_locals(),
        };
        for (size_t i = 0; i < ctx.ll_args(); i++) result.args.push_back(ctx.ll_arg(i).value());
        for (size_t i = 0; i < ctx.ll_locals(); i++) result.locals.push_back(ctx.ll_local(i).value());
        result.out_of_bounds_arg_is_as_expected = !ctx.ll_arg(ctx.ll_args());
        result.out_of_bounds_local_is_as_expected = !ctx.ll_local(ctx.ll_locals());
        return result;
    }
};

// globals will help us launder data out of call behaviour functions

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
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


    bool build_push_and_load_f_type_for_call_tests();
    bool build_push_and_load_g_type_for_call_tests();
    bool build_push_and_load_h_type_for_call_tests();


protected:

    void SetUp() override final {
        globals = Globals{};

        dbg = std::make_shared<yama::stderr_debug>();
        dm = std::make_shared<yama::default_domain>(dbg);
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

TEST_F(ContextTests, InitialState_UserCall) {
    EXPECT_FALSE(ctx->ll_panicking());
    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_args(), 0);
    EXPECT_EQ(ctx->ll_locals(), 0);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);

    EXPECT_EQ(ctx->ll_arg(0), std::nullopt); // out-of-bounds
    EXPECT_EQ(ctx->ll_local(0), std::nullopt); // out-of-bounds
}

TEST_F(ContextTests, InitialState_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 13,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // f
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.panics, 0);
        EXPECT_FALSE(ss.panicking);
        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 13);

        if (ss.args.size() == 1) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
        }
        EXPECT_TRUE(ss.locals.empty());

        EXPECT_TRUE(ss.out_of_bounds_arg_is_as_expected);
        EXPECT_TRUE(ss.out_of_bounds_local_is_as_expected);
    }
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

TEST_F(ContextTests, LLConsts) {
    // NOTE: ll_consts is tested via testing ll_call since our 'snapshot' type captures its behaviour
    // NOTE: this empty test is here to detail the above note
}

TEST_F(ContextTests, LLArg_UserCall) {
    EXPECT_EQ(ctx->ll_arg(0), std::nullopt); // no args
}

TEST_F(ContextTests, LLArg_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Float"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(31).good()); // argument #1
    ASSERT_TRUE(ctx->ll_push_float(1.33).good()); // argument #2
    ASSERT_TRUE(ctx->ll_call_nr(0, 3).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        // ss.arg# is sampled via ll_arg, so below tests its usage
        if (ss.args.size() == 3) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(31));
            EXPECT_EQ(ss.args[2], ctx->ll_new_float(1.33));
        }
        EXPECT_TRUE(ss.out_of_bounds_arg_is_as_expected);
    }
}

TEST_F(ContextTests, LLLocal_UserCall) {
    ASSERT_TRUE(ctx->ll_push_int(31).good());
    ASSERT_TRUE(ctx->ll_push_none().good());
    ASSERT_TRUE(ctx->ll_push_none().good());
    ASSERT_TRUE(ctx->ll_push_char(U'y').good());

    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(31)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(3), std::make_optional(ctx->ll_new_char(U'y')));
    EXPECT_EQ(ctx->ll_local(4), std::nullopt); // out-of-bounds
}

TEST_F(ContextTests, LLLocal_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_bool(true).bad()) return;
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_push_int(3).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_put_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // f
    ASSERT_TRUE(ctx->ll_push_int(31).good()); // 31
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 3);
        if (ss.locals.size() == 3) {
            EXPECT_EQ(ss.locals[0], ctx->ll_new_bool(true));
            EXPECT_EQ(ss.locals[1], ctx->ll_new_none());
            EXPECT_EQ(ss.locals[2], ctx->ll_new_int(3));
        }
        EXPECT_TRUE(ss.out_of_bounds_local_is_as_expected);
    }
}

TEST_F(ContextTests, LLPanic) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        ctx.ll_panic();
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        ctx.ll_panic(); // should fail quietly
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);
    EXPECT_FALSE(ctx->ll_panicking());

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
    EXPECT_FALSE(ctx->ll_panicking()); // panic already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_locals(), 0);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.panics, 0);
        EXPECT_FALSE(ss.panicking);
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.panics, 1);
        EXPECT_TRUE(ss.panicking);
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.panics, 1); // didn't incr
        EXPECT_TRUE(ss.panicking);
    }
}

TEST_F(ContextTests, LLPanic_UserCall) {
    EXPECT_EQ(ctx->ll_panics(), 0);
    EXPECT_FALSE(ctx->ll_panicking());

    // don't need to test ll_panic failing quietly, as another ll_panic
    // call *should* succeed here, as the initial panic will have 
    // completed by time the first ll_panic returned

    ctx->ll_panic();

    EXPECT_EQ(ctx->ll_panics(), 1);
    EXPECT_FALSE(ctx->ll_panicking()); // panic already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_locals(), 0);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);
}

TEST_F(ContextTests, LLPanic_MultiLevelCallStack) {
    // outer call
    const yama::const_table_info fa_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("fb"_str, yama::make_callsig_info({}, 0));
    auto fa_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_fn(yama::newtop, ctx.ll_consts().type(1).value()).bad()) return;
        if (ctx.ll_call_nr(0, 1).bad()) return;
        // prepare return obj (which won't be reached due to panic)
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        globals.even_reached_0 = true;
        };
    yama::type_info fa_info{
        .fullname = "fa"_str,
        .consts = fa_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fa_call_fn,
            .max_locals = 4,
        },
    };
    // inner call
    const yama::const_table_info fb_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto fb_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        ctx.ll_panic();
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        ctx.ll_panic(); // should fail quietly
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        };
    yama::type_info fb_info{
        .fullname = "fb"_str,
        .consts = fb_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fb_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(fa_info));
    ASSERT_TRUE(dm->upload(fb_info));
    ASSERT_TRUE(dm->load("fa"_str));
    ASSERT_TRUE(dm->load("fb"_str));

    const yama::type fa = dm->load("fa"_str).value();
    const yama::type fb = dm->load("fb"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);
    EXPECT_FALSE(ctx->ll_panicking());

    ASSERT_TRUE(ctx->ll_push_fn(fa).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
    EXPECT_FALSE(ctx->ll_panicking()); // panic already finished

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);
    EXPECT_EQ(ctx->ll_max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->ll_locals(), 0);
    EXPECT_EQ(ctx->ll_max_locals(), yama::user_max_locals);

    // outer call
    EXPECT_FALSE(globals.even_reached_0);

    // inner call
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.panics, 0);
        EXPECT_FALSE(ss.panicking);
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.panics, 1);
        EXPECT_TRUE(ss.panicking);
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.panics, 1); // didn't incr
        EXPECT_TRUE(ss.panicking);
    }
}

TEST_F(ContextTests, LLPop_UserCall) {
    ASSERT_TRUE(ctx->ll_push_int(1).good());
    ASSERT_TRUE(ctx->ll_push_int(2).good());
    ASSERT_TRUE(ctx->ll_push_int(3).good());

    ASSERT_EQ(ctx->ll_locals(), 3);

    ASSERT_TRUE(ctx->ll_pop(2).good());

    ASSERT_EQ(ctx->ll_locals(), 1);

    ASSERT_TRUE(ctx->ll_pop(10).good()); // should stop prematurely

    ASSERT_EQ(ctx->ll_locals(), 0);
}

TEST_F(ContextTests, LLPop_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_int(1).bad()) return;
        if (ctx.ll_push_int(2).bad()) return;
        if (ctx.ll_push_int(3).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_pop(2).bad()) return;
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        if (ctx.ll_pop(10).bad()) return; // should stop prematurely
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 3);
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.locals.size(), 1);
    }
    EXPECT_TRUE(globals.snapshot_2);
    if (globals.snapshot_2) {
        const auto& ss = *globals.snapshot_2;
        EXPECT_EQ(ss.locals.size(), 0);
    }
}

TEST_F(ContextTests, LLPut_UserCall) {
    ASSERT_TRUE(ctx->ll_put(yama::newtop, ctx->ll_new_int(-14)).good());

    EXPECT_EQ(ctx->ll_locals(), 1);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));

    // overwrite at index
    ASSERT_TRUE(ctx->ll_put(0, ctx->ll_new_int(3)).good());

    EXPECT_EQ(ctx->ll_locals(), 1);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(3)));
}

TEST_F(ContextTests, LLPut_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put(yama::newtop, ctx.ll_new_int(-14)).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        // overwrite at index
        if (ctx.ll_put(0, ctx.ll_new_int(3)).bad()) return;
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put_fn(yama::newtop, f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 1);
        if (ss.locals.size() == 1) {
            EXPECT_EQ(ss.locals[0], ctx->ll_new_int(-14));
        }
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.locals.size(), 1);
        if (ss.locals.size() == 1) {
            EXPECT_EQ(ss.locals[0], ctx->ll_new_int(3));
        }
    }
}

TEST_F(ContextTests, LLPut_PanicIfXIsOutOfBounds) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_put(70, ctx->ll_new_none()).bad()); // 70 is out-of-bounds! (and not newtop)

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPut_PanicIfPushingOverflows) {
    // saturate the stack
    for (size_t i = 0; i < yama::user_max_locals; i++) {
        ASSERT_TRUE(ctx->ll_push_none().good()) << "i == " << i << "\n";
    }

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_put(yama::newtop, ctx->ll_new_none()).bad()); // overflow!

    EXPECT_EQ(ctx->ll_panics(), 1);
}

// NOTE: for the LLPut# tests below, for each ll_put_# method, we
//       presume that they wrap calls to ll_put, and thus need-not
//       be tested in regards to things like x out-of-bounds
//
//       notice this only applies to the ll_put_# methods wrapping
//       compositions w/ ll_new_# method calls, and does NOT include
//       ll_put_arg, which is tested *fully*

TEST_F(ContextTests, LLPutNone) {
    ASSERT_TRUE(ctx->ll_put_none(yama::newtop).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_int(33).good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_none(1).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_none()));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_none()));
}

TEST_F(ContextTests, LLPutInt) {
    ASSERT_TRUE(ctx->ll_put_int(yama::newtop, -14).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_int(1, 2).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(2)));
}

TEST_F(ContextTests, LLPutUInt) {
    ASSERT_TRUE(ctx->ll_put_uint(yama::newtop, 14).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_uint(1, 2).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_uint(14)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_uint(2)));
}

TEST_F(ContextTests, LLPutFloat) {
    ASSERT_TRUE(ctx->ll_put_float(yama::newtop, 14.3).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_float(1, 2.315).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_float(14.3)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_float(2.315)));
}

TEST_F(ContextTests, LLPutBool) {
    ASSERT_TRUE(ctx->ll_put_bool(yama::newtop, true).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_bool(1, false).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_bool(true)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_bool(false)));
}

TEST_F(ContextTests, LLPutChar) {
    ASSERT_TRUE(ctx->ll_put_char(yama::newtop, 'y').good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_char(1, '3').good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_char('y')));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_char('3')));
}

TEST_F(ContextTests, LLPutFn) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_put_fn(yama::newtop, f).good()); // via newtop
    ASSERT_TRUE(ctx->ll_push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->ll_put_fn(1, f).good()); // via index

    EXPECT_EQ(ctx->ll_locals(), 2);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_fn(f)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_fn(f)));
}

TEST_F(ContextTests, LLPutFn_PanicIfFIsNotCallable) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_put_fn(yama::newtop, ctx->load_bool()).bad()); // Bool is not callable!

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutConst) {
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
    constexpr size_t loadable_object_constant_types = yama::const_types - 1;
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
        [](yama::context& ctx) {
        for (size_t i = 0; i < loadable_object_constant_types; i++) {
            if (ctx.ll_push_none().bad()) return;
        }
        // I wanna have the x and c args differ so the impl can't so easily get away w/
        // confusing the two (as actually happened to be on initial impl of ll_put_const)
        static_assert(loadable_object_constant_types == 6);
        if (ctx.ll_put_const(0, 1).bad()) return; // via index
        if (ctx.ll_put_const(1, 2).bad()) return; // via index
        if (ctx.ll_put_const(2, 3).bad()) return; // via index
        if (ctx.ll_put_const(3, 4).bad()) return; // via index
        if (ctx.ll_put_const(4, 5).bad()) return; // via index
        if (ctx.ll_put_const(5, 6).bad()) return; // via index
        static_assert(loadable_object_constant_types == 6);
        if (ctx.ll_put_const(yama::newtop, 1).bad()) return; // via newtop
        if (ctx.ll_put_const(yama::newtop, 2).bad()) return; // via newtop
        if (ctx.ll_put_const(yama::newtop, 3).bad()) return; // via newtop
        if (ctx.ll_put_const(yama::newtop, 4).bad()) return; // via newtop
        if (ctx.ll_put_const(yama::newtop, 5).bad()) return; // via newtop
        if (ctx.ll_put_const(yama::newtop, 6).bad()) return; // via newtop
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_put_none(0).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = yama::const_types * 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), loadable_object_constant_types * 2);
        if (ss.locals.size() == loadable_object_constant_types * 2) {
            static_assert(loadable_object_constant_types == 6);
            
            EXPECT_EQ(ss.locals[0], std::make_optional(ctx->ll_new_int(-4)));
            EXPECT_EQ(ss.locals[1], std::make_optional(ctx->ll_new_uint(301)));
            EXPECT_EQ(ss.locals[2], std::make_optional(ctx->ll_new_float(3.14159)));
            EXPECT_EQ(ss.locals[3], std::make_optional(ctx->ll_new_bool(true)));
            EXPECT_EQ(ss.locals[4], std::make_optional(ctx->ll_new_char(U'y')));
            EXPECT_EQ(ss.locals[5], std::make_optional(ctx->ll_new_fn(f)));
            
            EXPECT_EQ(ss.locals[6], std::make_optional(ctx->ll_new_int(-4)));
            EXPECT_EQ(ss.locals[7], std::make_optional(ctx->ll_new_uint(301)));
            EXPECT_EQ(ss.locals[8], std::make_optional(ctx->ll_new_float(3.14159)));
            EXPECT_EQ(ss.locals[9], std::make_optional(ctx->ll_new_bool(true)));
            EXPECT_EQ(ss.locals[10], std::make_optional(ctx->ll_new_char(U'y')));
            EXPECT_EQ(ss.locals[11], std::make_optional(ctx->ll_new_fn(f)));
        }
    }
}

TEST_F(ContextTests, LLPutConst_PanicIfInUserCallFrame) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    // the user call frame has no constants, so it's not really possible to test
    // this w/out having the constant be out-of-bounds

    ASSERT_TRUE(ctx->ll_put_const(yama::newtop, 0).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutConst_PanicIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_const(0, 0).bad()) return; // local register index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ll_put_none(yama::newtop).bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLPutConst_PanicIfPushingOverflows) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_none().bad()) return; // saturate the stack
        if (ctx.ll_put_const(yama::newtop, 0).bad()) return; // overflow!
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutConst_PanicIfCIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_const(yama::newtop, 70).bad()) return; // constant index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLPutConst_PanicIfCIsNotIndexOfAnObjectConstant) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_const(yama::newtop, 0).bad()) return; // constant at index 0 is NOT an object constant
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLPutArg) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_arg(yama::newtop, 0).bad()) return; // via newtop (callobj)
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_put_arg(1, 1).bad()) return; // via index (argument)
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(2).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 2);
        if (ss.locals.size() == 2) {
            EXPECT_EQ(ss.locals[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.locals[1], ctx->ll_new_int(-14));
        }
    }
}

TEST_F(ContextTests, LLPutArg_PanicIfInUserCallFrame) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    // the user call frame has no arguments, so it's not really possible to test
    // this w/out having the arg param be out-of-bounds

    ASSERT_TRUE(ctx->ll_put_arg(yama::newtop, 0).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutArg_PanicIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_arg(70, 0).bad()) return; // 70 is out-of-bounds
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutArg_PanicIfPushingOverflows) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_none().bad()) return; // saturate the stack
        if (ctx.ll_put_arg(yama::newtop, 0).bad()) return; // overflow!
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLPutArg_PanicIfArgIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_put_arg(yama::newtop, 70).bad()) return; // 70 is out-of-bounds
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(1).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLCopy_UserCall) {
    ASSERT_TRUE(ctx->ll_push_int(-14).good());
    ASSERT_TRUE(ctx->ll_push_none().good());

    ASSERT_TRUE(ctx->ll_copy(0, 1).good()); // via index
    ASSERT_TRUE(ctx->ll_copy(0, yama::newtop).good()); // via newtop

    EXPECT_EQ(ctx->ll_locals(), 3);
    EXPECT_EQ(ctx->ll_local(0), std::make_optional(ctx->ll_new_int(-14)));
    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(-14)));
    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(-14)));
}

TEST_F(ContextTests, LLCopy_NonUserCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_int(-14).bad()) return;
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_copy(0, 1).bad()) return; // via index
        if (ctx.ll_copy(0, yama::newtop).bad()) return; // via newtop
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(3).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 4,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 3);
        if (ss.locals.size() == 3) {
            EXPECT_EQ(ss.locals[0], std::make_optional(ctx->ll_new_int(-14)));
            EXPECT_EQ(ss.locals[1], std::make_optional(ctx->ll_new_int(-14)));
            EXPECT_EQ(ss.locals[2], std::make_optional(ctx->ll_new_int(-14)));
        }
    }
}

TEST_F(ContextTests, LLCopy_OverwritesStateOfDest) {
    // ll_copy is to overwrite uint 301 w/ a int -4

    ASSERT_TRUE(ctx->ll_push_int(-4).good());
    ASSERT_TRUE(ctx->ll_push_uint(301).good()); // old obj state
    
    ASSERT_TRUE(ctx->ll_copy(0, 1).good());

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLCopy_PanicIfSrcOutOfBounds) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_copy(70, yama::newtop).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLCopy_PanicIfDestIsOutOfBounds) {
    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_int(3).good());

    ASSERT_TRUE(ctx->ll_copy(0, 70).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLCopy_PanicIfPushingOverflows) {
    // saturate the stack
    for (size_t i = 0; i < yama::user_max_locals; i++) {
        ASSERT_TRUE(ctx->ll_push_none().good()) << "i == " << i << "\n";
    }

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_copy(0).bad()); // overflow!

    EXPECT_EQ(ctx->ll_panics(), 1);
}

bool ContextTests::build_push_and_load_f_type_for_call_tests() {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred
        globals.snapshot_0 = CallStateSnapshot::make(ctx);

        if (ctx.ll_push_arg(1).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 6,
        },
    };
    return
        dm->upload(f_info) &&
        dm->load("f"_str); // <- skip tests if we can't even load f properly
}

bool ContextTests::build_push_and_load_g_type_for_call_tests() {
    const yama::const_table_info g_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_function_type("f"_str, yama::make_callsig_info({ 0 }, 0));
    auto g_call_fn =
        [](yama::context& ctx) {
        if (ctx.ll_push_fn(ctx.ll_consts().type(1).value()).bad()) return; // f
        if (ctx.ll_push_arg(1).bad()) return; // argument
        if (ctx.ll_push_int(0).bad()) return; // result
        if (ctx.ll_call(0, 2, 0).bad()) return; // call f
        if (ctx.ll_pop(1).bad()) return; // pop excess objs
        if (ctx.ll_ret(0).bad()) return; // return result of call f
        };
    yama::type_info g_info{
        .fullname = "g"_str,
        .consts = g_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = g_call_fn,
            .max_locals = 4,
        },
    };
    return
        dm->upload(g_info) &&
        dm->load("g"_str); // <- skip tests if we can't even load g properly
}

bool ContextTests::build_push_and_load_h_type_for_call_tests() {
    const auto h_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_arg(yama::newtop, 1)
        .add_call(0, 2, 0, true)
        .add_pop(1)
        .add_ret(0);
    std::cerr << h_bcode.fmt_disassembly() << "\n";
    const yama::const_table_info h_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_function_type("f"_str, yama::make_callsig_info({ 0 }, 0));
    yama::type_info h_info{
        .fullname = "h"_str,
        .consts = h_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = h_bcode,
        },
    };
    return
        dm->upload(h_info) &&
        dm->load("h"_str); // <- skip tests if we can't even load h properly
}

TEST_F(ContextTests, LLCall_ViaIndex) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good()); // via index

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

    EXPECT_EQ(ctx->ll_locals(), 3);
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
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCall_ViaNewtop) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, yama::newtop).good()); // via newtop

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

    EXPECT_EQ(ctx->ll_locals(), 3);
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
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCall_MultiLevelCallStack) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_g_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type g = dm->load("g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->ll_push_fn(g).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

    EXPECT_EQ(ctx->ll_locals(), 3);
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
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCall_OverwritesStateOfRet) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(-4).good()); // argument
    // ll_call is to overwrite uint 301 w/ a int -4
    ASSERT_TRUE(ctx->ll_push_uint(301).good()); // result (ie. overwrite old obj state)

    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(-4)));
}

TEST_F(ContextTests, LLCall_BCodeExec) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_h_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type h = dm->load("h"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(h).good()); // h
    ASSERT_TRUE(ctx->ll_push_int(-4).good()); // -4
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result goes here
    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

    EXPECT_EQ(ctx->ll_local(2), std::make_optional(ctx->ll_new_int(-4)));

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
        EXPECT_EQ(ss.call_frames, 3);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(-4));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCall_PanicIfArgsProvidesNoCallObj) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;

        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // <- would be callobj (but ll_call won't specify having it)

    ASSERT_TRUE(ctx->ll_call(0, 0, 0).bad()); // no callobj -> panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfArgsIndexRangeIsOutOfBounds_DueToBadArgStart) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(70, 2, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfArgsIndexRangeIsOutOfBounds_DueToBadArgN) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 70, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfRetIsOutOfBounds) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2, 70).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfPushingOverflows) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    // saturate the stack (but leave space for callobj/argument)
    for (size_t i = 0; i < yama::user_max_locals - 2; i++) {
        ASSERT_TRUE(ctx->ll_push_none().good()) << "i == " << i << "\n";
    }

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 2).bad()); // overflow!

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfCallObjIsNotCallableType) {
    ASSERT_TRUE(ctx->ll_push_float(13.02).good()); // call object, but float can't be!
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLCall_PanicIfUnexpectedArgCount_TooManyArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument #1
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument #2
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 3, 3).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfUnexpectedArgCount_TooFewArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCall_PanicIfCallStackOverflow) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and panic
        if (ctx.ll_push_fn(ctx.ll_consts().type(1).value()).bad()) return; // call object
        if (ctx.ll_push_none().bad()) return; // result
        if (ctx.ll_call(0, 1).bad()) return;
        if (ctx.ll_ret(2).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_none().good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
    EXPECT_EQ(globals.call_depth, yama::max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, LLCall_PanicIfNoReturnValueObjectProvided) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        // return w/out ever calling ll_ret
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_none().good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(globals.even_reached_0); // unlike others, this panic doesn't prevent call behaviour
}

TEST_F(ContextTests, LLCallNR) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).good());

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

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
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCallNR_MultiLevelCallStack) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_g_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type g = dm->load("g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->ll_push_fn(g).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).good());

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

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
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCallNR_BCodeExec) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    ASSERT_TRUE(build_push_and_load_h_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();
    const yama::type h = dm->load("h"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(h).good()); // h
    ASSERT_TRUE(ctx->ll_push_int(-4).good()); // -4
    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->ll_call_nr(0, 2).good());

    EXPECT_TRUE(ctx->ll_is_user());
    EXPECT_EQ(ctx->ll_call_frames(), 1);

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
        EXPECT_EQ(ss.call_frames, 3);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->ll_new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->ll_new_int(-4));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, LLCallNR_PanicIfArgsProvidesNoCallObj) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;

        if (ctx.ll_push_none().bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // <- would be callobj (but ll_call_nr won't specify having it)

    ASSERT_TRUE(ctx->ll_call_nr(0, 0).bad()); // no callobj -> panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCallNR_PanicIfArgsIndexRangeIsOutOfBounds_DueToBadArgStart) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(70, 2).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCallNR_PanicIfArgsIndexRangeIsOutOfBounds_DueToBadArgN) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(0, 70).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCallNR_PanicIfCallObjIsNotCallableType) {
    ASSERT_TRUE(ctx->ll_push_float(13.02).good()); // call object, but float can't be!

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLCallNR_PanicIfUnexpectedArgCount_TooManyArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument #1
    ASSERT_TRUE(ctx->ll_push_int(3).good()); // argument #2

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(0, 3).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCallNR_PanicIfUnexpectedArgCount_TooFewArgs) {
    ASSERT_TRUE(build_push_and_load_f_type_for_call_tests());
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object

    EXPECT_EQ(ctx->ll_panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, LLCallNR_PanicIfCallStackOverflow) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and panic
        if (ctx.ll_push_fn(ctx.ll_consts().type(1).value()).bad()) return; // call object
        if (ctx.ll_push_none().bad()) return; // result
        if (ctx.ll_call(0, 1).bad()) return;
        if (ctx.ll_ret(2).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
    EXPECT_EQ(globals.call_depth, yama::max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, LLCallNR_PanicIfNoReturnValueObjectProvided) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        // return w/out ever calling ll_ret
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call_nr(0, 1).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(globals.even_reached_0); // unlike others, this panic doesn't prevent call behaviour
}

// NOTE: most usage of ll_ret will be tested as part of testing ll_call

TEST_F(ContextTests, LLRet_AllowsReturningWrongTypedObject) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred

        // tell ll_ret to return a UInt, even though f should return a Int,
        // but the VM will allow for returning the wrong typed object
        
        // checking return type of return value object is beyond the scope 
        // of this level of abstraction, and so for safety this is allowed
        // as almost a kind of *feature* of the low-level command interface

        if (ctx.ll_push_uint(301).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_int(0).good()); // result

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1), std::make_optional(ctx->ll_new_uint(301)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
}

TEST_F(ContextTests, LLRet_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ctx->ll_is_user());

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_ret(0).bad());

    EXPECT_EQ(ctx->ll_panics(), 1);
}

TEST_F(ContextTests, LLRet_PanicIfXIsOutOfBounds) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        if (ctx.ll_ret(70).bad()) return; // 70 is out-of-bounds
        };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_none().good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

TEST_F(ContextTests, LLRet_PanicIfCalledMultipleTimesInOneCall) {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
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
            .max_locals = 1,
        },
    };
    ASSERT_TRUE(dm->upload(f_info) && dm->load("f"_str));
    const yama::type f = dm->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->ll_push_none().good()); // result

    EXPECT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

