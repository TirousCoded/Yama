

#include <gtest/gtest.h>

#include <yama/core/bcode.h>
#include <yama/core/context.h>
#include <yama/debug-impls/stderr_debug.h>
#include <yama/mas-impls/heap_mas.h>
#include <yama/domain-impls/domain_st.h>


using namespace yama::string_literals;


class BCodeExecTests : public testing::Test {
public:

    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::mas> mas;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


protected:

    void SetUp() override final {
        dbg = std::make_shared<yama::stderr_debug>();
        mas = std::make_shared<yama::heap_mas>(dbg);
        dm = std::make_shared<yama::domain_st>(yama::res(mas), dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), yama::default_ctx_config, dbg);
    }

    void TearDown() override final {
        //
    }
};


// PER-INSTRUCTION TESTS

TEST_F(BCodeExecTests, Instr_Noop) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true)
        .add_noop()
        .add_noop()
        .add_noop()
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_int(101));
}

TEST_F(BCodeExecTests, Instr_LoadNone) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true)
        .add_load_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_int(101);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_load_int(1, 10).good()); // make type change visible
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_none());
}

TEST_F(BCodeExecTests, Instr_LoadConst) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str)
        .add_float(1.01);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_float(1.01));
}

TEST_F(BCodeExecTests, Instr_LoadArg) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_arg(0, 1, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Float"_str);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_load_float(1, 3.14159).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());
    ASSERT_TRUE(ctx->ll_load_float(1, -0.765).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 3).good());

    EXPECT_EQ(ctx->ll_local(2).value(), ctx->ll_new_float(3.14159));
    EXPECT_EQ(ctx->ll_local(3).value(), ctx->ll_new_float(-0.765));
}

TEST_F(BCodeExecTests, Instr_Copy) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true)
        .add_load_const(1, 2, true)
        .add_copy(1, 0)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(0)
        .add_int(101);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_int(101));
}

static bool was_called_0 = false;
static bool was_called_1 = false;

TEST_F(BCodeExecTests, Instr_Call) {
    auto plus_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        yama::int_t result =
            ctx.ll_arg(1).value().as_int() +
            ctx.ll_arg(2).value().as_int() +
            ctx.ll_arg(3).value().as_int();
        if (ctx.ll_load_int(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto plus_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    yama::type_info plus_info{
        .fullname = "plus"_str,
        .consts = plus_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0, 0 }, 0),
            .call_fn = plus_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(1, 1, true)
        .add_load_const(2, 4, true)
        .add_load_const(3, 3, true)
        .add_load_const(4, 2, true)
        .add_call(1, 4, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_function_type("plus"_str, yama::make_callsig_info({ 0, 0, 0 }, 0))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(plus_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_int(149));
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallBehaviourPanics) {
    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.ll_panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info panic_info{
        .fullname = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // panic
        .add_call(0, 1, 0, true)
        .add_load_const(0, 2, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(panic_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallBehaviourDoesNotReturnAnything) {
    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info panic_info{
        .fullname = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // panic
        .add_call(0, 1, 0, true)
        .add_load_const(0, 2, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(panic_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallStackWouldOverflow) {
    ctx->dbg()->cats &= ~yama::bcode_exec_c; // make output easier to read

    // this 'dummy' function is here as it will be the thing that will actually trigger
    // the call stack overflow, which otherwise would be fail_safe, but I don't want it
    // to be the thing triggering it, as that's not its role (also the fact that it returns
    // something means it wouldn't work for testing call_nr)

    auto dummy_fn =
        [](yama::context& ctx) {
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto dummy_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info dummy_info{
        .fullname = "dummy"_str,
        .consts = dummy_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = dummy_fn,
            .locals = 1,
        },
    };

    auto fail_safe_fn = 
        [](yama::context& ctx) {
        // in the event of the impl actually allowing for infinite (or at least until
        // the VM breaks) number of calls, this *fail safe* function will cause it to 
        // stop after proving its point, so we can get useful testing data
        const bool triggered = ctx.ll_call_frames() > ctx.ll_max_call_frames();
        if (triggered) {
            YAMA_LOG(ctx.dbg(), yama::general_c, "** fail safe triggered! **");
        }
        if (ctx.ll_load_bool(0, triggered).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto fail_safe_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str);
    yama::type_info fail_safe_info{
        .fullname = "fail_safe"_str,
        .consts = fail_safe_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fail_safe_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 5, true) // dummy
        .add_call(0, 1, 0, true) // <- the call instr under test
        .add_load_const(0, 2, true) // fail_safe
        .add_call(0, 1, 0, true)
        .add_jump_false(0, 2)
        // block #2
        .add_load_none(0, true)
        .add_ret(0)
        // block #3
        .add_load_const(0, 4, true) // f (recurse)
        .add_call(0, 1, 0, true)
        // infinite recursion should induce panic by this point
        .add_load_const(0, 3, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_primitive_type("Bool"_str)
        .add_function_type("fail_safe"_str, yama::make_callsig_info({}, 1))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0))
        .add_function_type("f"_str, yama::make_callsig_info({}, 0))
        .add_function_type("dummy"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(dummy_info));
    ASSERT_TRUE(ctx->dm().push(fail_safe_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR) {
    auto plus_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        yama::int_t result =
            ctx.ll_arg(1).value().as_int() +
            ctx.ll_arg(2).value().as_int() +
            ctx.ll_arg(3).value().as_int();
        if (ctx.ll_load_int(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto plus_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str);
    yama::type_info plus_info{
        .fullname = "plus"_str,
        .consts = plus_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0, 0 }, 0),
            .call_fn = plus_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 2, true)
        .add_load_const(1, 3, true)
        .add_load_const(2, 4, true)
        .add_load_const(3, 5, true)
        .add_call_nr(0, 4)
        .add_ret(4);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_primitive_type("Int"_str)
        .add_function_type("plus"_str, yama::make_callsig_info({ 1, 1, 1 }, 1))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 5,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(plus_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_none());
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallBehaviourPanics) {
    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.ll_panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info panic_info{
        .fullname = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // panic
        .add_call_nr(0, 1)
        .add_load_const(0, 2, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(panic_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallBehaviourDoesNotReturnAnything) {
    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info panic_info{
        .fullname = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true) // panic
        .add_call_nr(0, 1)
        .add_load_const(0, 2, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_function_type("panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(panic_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallStackWouldOverflow) {
    ctx->dbg()->cats &= ~yama::bcode_exec_c; // make output easier to read

    // this 'dummy' function is here as it will be the thing that will actually trigger
    // the call stack overflow, which otherwise would be fail_safe, but I don't want it
    // to be the thing triggering it, as that's not its role (also the fact that it returns
    // something means it wouldn't work for testing call_nr)

    auto dummy_fn =
        [](yama::context& ctx) {
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto dummy_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info dummy_info{
        .fullname = "dummy"_str,
        .consts = dummy_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = dummy_fn,
            .locals = 1,
        },
    };

    auto fail_safe_fn =
        [](yama::context& ctx) {
        // in the event of the impl actually allowing for infinite (or at least until
        // the VM breaks) number of calls, this *fail safe* function will cause it to 
        // stop after proving its point, so we can get useful testing data
        const bool triggered = ctx.ll_call_frames() > ctx.ll_max_call_frames();
        if (triggered) {
            YAMA_LOG(ctx.dbg(), yama::general_c, "** fail safe triggered! **");
        }
        if (ctx.ll_load_bool(0, triggered).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto fail_safe_consts =
        yama::const_table_info()
        .add_primitive_type("Bool"_str);
    yama::type_info fail_safe_info{
        .fullname = "fail_safe"_str,
        .consts = fail_safe_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fail_safe_fn,
            .locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str);
    yama::type_info never_reached_info{
        .fullname = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 5, true) // dummy
        .add_call_nr(0, 1) // <- the call_nr instr under test
        .add_load_const(0, 2, true) // fail_safe
        .add_call(0, 1, 0, true)
        .add_jump_false(0, 2)
        // block #2
        .add_load_none(0, true)
        .add_ret(0)
        // block #3
        .add_load_const(0, 4, true) // f (recurse)
        .add_call(0, 1, 0, true)
        // infinite recursion should induce panic by this point
        .add_load_const(0, 3, true) // never_reached
        .add_call(0, 1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("None"_str)
        .add_primitive_type("Bool"_str)
        .add_function_type("fail_safe"_str, yama::make_callsig_info({}, 1))
        .add_function_type("never_reached"_str, yama::make_callsig_info({}, 0))
        .add_function_type("f"_str, yama::make_callsig_info({}, 0))
        .add_function_type("dummy"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(dummy_info));
    ASSERT_TRUE(ctx->dm().push(fail_safe_info));
    ASSERT_TRUE(ctx->dm().push(never_reached_info));
    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    was_called_1 = false;

    ASSERT_EQ(ctx->ll_panics(), 0);

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).bad()); // expect panic

    EXPECT_EQ(ctx->ll_panics(), 1);

    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Ret) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_const(0, 1, true)
        .add_load_const(1, 2, true)
        .add_load_const(2, 3, true)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str)
        .add_int(-10)
        .add_uint(4)
        .add_float(1.01);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 3,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_uint(4));
}

TEST_F(BCodeExecTests, Instr_Jump) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(2)
        // block #2
        .add_load_const(0, 1, true)
        .add_ret(0)
        // block #3
        .add_load_const(0, 2, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_int(70)
        .add_int(-7);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 1,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_call(0, 1, 1).good());

    EXPECT_EQ(ctx->ll_local(1).value(), ctx->ll_new_int(-7));
}

TEST_F(BCodeExecTests, Instr_JumpTrue) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_arg(0, 1, true)
        .add_jump_true(0, 2)
        // block #2
        .add_load_const(1, 2, true)
        .add_ret(1)
        // block #3
        .add_load_const(1, 3, true)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Bool"_str)
        .add_int(-10)
        .add_int(4);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_load_bool(1, true).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());
    ASSERT_TRUE(ctx->ll_load_bool(1, false).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 3).good());

    EXPECT_EQ(ctx->ll_local(2).value(), ctx->ll_new_int(4));
    EXPECT_EQ(ctx->ll_local(3).value(), ctx->ll_new_int(-10));
}

TEST_F(BCodeExecTests, Instr_JumpFalse) {
    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_load_arg(0, 1, true)
        .add_jump_false(0, 2)
        // block #2
        .add_load_const(1, 2, true)
        .add_ret(1)
        // block #3
        .add_load_const(1, 3, true)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("Int"_str)
        .add_primitive_type("Bool"_str)
        .add_int(-10)
        .add_int(4);
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 2,
            .bcode = f_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(f_info));

    const auto f = ctx->load("f"_str).value();

    ASSERT_TRUE(ctx->ll_load_fn(0, f).good());
    ASSERT_TRUE(ctx->ll_load_bool(1, true).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 2).good());
    ASSERT_TRUE(ctx->ll_load_bool(1, false).good());
    ASSERT_TRUE(ctx->ll_call(0, 2, 3).good());

    EXPECT_EQ(ctx->ll_local(2).value(), ctx->ll_new_int(-10));
    EXPECT_EQ(ctx->ll_local(3).value(), ctx->ll_new_int(4));
}

// EXAMPLE TESTS

static yama::uint_t example_fac(yama::uint_t n) noexcept {
    return
        n == 0
        ? 1
        : n * example_fac(n - 1);
}

TEST_F(BCodeExecTests, Example_Factorial) {
    // test impl of a basic factorial function for testing recursion

    const auto subtract_fn =
        [](yama::context& ctx) {
        const auto a = ctx.ll_arg(1).value().as_uint();
        const auto b = ctx.ll_arg(2).value().as_uint();
        const auto result = a - b;
        if (ctx.ll_load_uint(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto subtract_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str);
    yama::type_info subtract_info{
        .fullname = "subtract"_str,
        .consts = subtract_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 0),
            .call_fn = subtract_fn,
            .locals = 1,
        },
    };

    const auto multiply_fn =
        [](yama::context& ctx) {
        const auto a = ctx.ll_arg(1).value().as_uint();
        const auto b = ctx.ll_arg(2).value().as_uint();
        const auto result = a * b;
        if (ctx.ll_load_uint(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto multiply_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str);
    yama::type_info multiply_info{
        .fullname = "multiply"_str,
        .consts = multiply_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 0),
            .call_fn = multiply_fn,
            .locals = 1,
        },
    };
    
    const auto greaterThanZero_fn =
        [](yama::context& ctx) {
        const auto success = ctx.ll_arg(1).value().as_uint() > 0;
        if (ctx.ll_load_bool(0, success).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto greaterThanZero_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str)
        .add_primitive_type("Bool"_str);
    yama::type_info greaterThanZero_info{
        .fullname = "greaterThanZero"_str,
        .consts = greaterThanZero_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = greaterThanZero_fn,
            .locals = 1,
        },
    };

    const auto factorial_bcode =
        yama::bc::code()
        // NOTE: below, the arg of the fn will be called 'n'
        // block #1
        .add_load_const(0, 4, true) // greaterThanZero
        .add_load_arg(1, 1, true) // n
        .add_call(0, 2, 0, true) // check if n > 0
        .add_jump_true(0, 2) // jump to block #2 if false, fallthrough to block #3 otherwise
        // block #2
        .add_load_const(0, 6, true) // 1
        .add_ret(0) // return 1 if n == 0
        // block #3
        // eval n - 1
        .add_load_const(0, 2, true) // subtract
        .add_load_arg(1, 1, true) // n
        .add_load_const(2, 6, true) // 1
        .add_call(0, 3, 1) // is call arg 1 in next eval
        // eval (n - 1)!
        .add_load_const(0, 5, true) // factorial
        .add_call(0, 2, 2) // is call arg 2 in next eval
        // eval n * (n - 1)!
        .add_load_const(0, 3, true) // multiply
        .add_load_arg(1, 1, true) // n
        .add_call(0, 3, 0, true)
        .add_ret(0); // return n * (n - 1)! if n > 0
    std::cerr << factorial_bcode.fmt_disassembly() << "\n";
    const auto factorial_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str)
        .add_primitive_type("Bool"_str)
        .add_function_type("subtract"_str, yama::make_callsig_info({ 0, 0 }, 0))
        .add_function_type("multiply"_str, yama::make_callsig_info({ 0, 0 }, 0))
        .add_function_type("greaterThanZero"_str, yama::make_callsig_info({ 0 }, 1))
        .add_function_type("factorial"_str, yama::make_callsig_info({ 0 }, 0))
        .add_uint(1);
    yama::type_info factorial_info{
        .fullname = "factorial"_str,
        .consts = factorial_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 4,
            .bcode = factorial_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(subtract_info));
    ASSERT_TRUE(ctx->dm().push(multiply_info));
    ASSERT_TRUE(ctx->dm().push(greaterThanZero_info));
    ASSERT_TRUE(ctx->dm().push(factorial_info));

    const auto factorial = ctx->load("factorial"_str).value();

    for (yama::uint_t i = 0; i <= 11; i++) {
        const auto expected = example_fac(i);

        YAMA_LOG(dbg, yama::general_c, "{}! == {}", size_t(i), size_t(expected));

        ASSERT_TRUE(ctx->ll_load_fn(0, factorial).good());
        ASSERT_TRUE(ctx->ll_load_uint(1, i).good());
        ASSERT_TRUE(ctx->ll_call(0, 2, 0).good());

        const auto result = ctx->ll_local(0).value();

        YAMA_LOG(dbg, yama::general_c, "bcode result: {}", result.fmt());

        EXPECT_EQ(result, ctx->ll_new_uint(expected));
    }
}

TEST_F(BCodeExecTests, Example_Counter) {
    // test impl of a function which increments a counter local
    // variable register some number of times, returning its final
    // value as result, in order to test looping

    const auto addOne_fn =
        [](yama::context& ctx) {
        const auto a = ctx.ll_arg(1).value().as_uint();
        const auto result = a + 1;
        if (ctx.ll_load_uint(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto addOne_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str);
    yama::type_info addOne_info{
        .fullname = "addOne"_str,
        .consts = addOne_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = addOne_fn,
            .locals = 1,
        },
    };

    const auto lessThan_fn =
        [](yama::context& ctx) {
        const auto a = ctx.ll_arg(1).value().as_uint();
        const auto b = ctx.ll_arg(2).value().as_uint();
        const auto result = a < b;
        if (ctx.ll_load_bool(0, result).bad()) return;
        if (ctx.ll_ret(0).bad()) return;
        };
    const auto lessThan_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str)
        .add_primitive_type("Bool"_str);
    yama::type_info lessThan_info{
        .fullname = "lessThan"_str,
        .consts = lessThan_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 1),
            .call_fn = lessThan_fn,
            .locals = 1,
        },
    };

    const auto counter_bcode =
        yama::bc::code()
        // NOTE: below, the arg of the fn will be called 'n'
        // block #1
        .add_load_const(0, 4, true) // init 'counter' to 0
        // block #2
        // eval counter < n
        .add_load_const(1, 3, true) // lessThan
        .add_copy(0, 2, true) // counter
        .add_load_arg(3, 1, true) // n
        .add_call(1, 3, 1, true) // counter < n
        .add_jump_false(1, 7) // jump to return instr if counter >= n
        // block #3
        // eval counter = addOne(counter)
        .add_load_const(1, 2, true) // addOne
        .add_copy(0, 2, true) // counter
        .add_call(1, 2, 0) // counter = addOne(counter)
        // set locals 1 through 3 to None object to ensure register coherence
        .add_load_none(1, true)
        .add_load_none(2, true)
        .add_load_none(3, true)
        .add_jump(-12) // jump back to block #2 to begin next iter of loop
        // block #4
        .add_ret(0); // return final value of counter
    std::cerr << counter_bcode.fmt_disassembly() << "\n";
    const auto counter_consts =
        yama::const_table_info()
        .add_primitive_type("UInt"_str)
        .add_primitive_type("Bool"_str)
        .add_function_type("addOne"_str, yama::make_callsig_info({ 0 }, 0))
        .add_function_type("lessThan"_str, yama::make_callsig_info({ 0, 0 }, 1))
        .add_uint(0);
    yama::type_info counter_info{
        .fullname = "counter"_str,
        .consts = counter_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .locals = 4,
            .bcode = counter_bcode,
        },
    };

    ASSERT_TRUE(ctx->dm().push(addOne_info));
    ASSERT_TRUE(ctx->dm().push(lessThan_info));
    ASSERT_TRUE(ctx->dm().push(counter_info));

    const auto counter = ctx->dm().load("counter"_str).value();

#define _EXAMPLE_TEST(n) \
{ \
    YAMA_LOG(dbg, yama::general_c, "counter({0}) == {0}", n); \
    \
    ASSERT_TRUE(ctx->ll_load_fn(0, counter).good()); \
    ASSERT_TRUE(ctx->ll_load_uint(1, n).good()); \
    ASSERT_TRUE(ctx->ll_call(0, 2, 0).good()); \
    \
    const auto result = ctx->ll_local(0).value(); \
    \
    YAMA_LOG(dbg, yama::general_c, "bcode result: {}", result.fmt()); \
    \
    EXPECT_EQ(result, ctx->ll_new_uint(n)); \
} (void)0

    _EXAMPLE_TEST(0);
    _EXAMPLE_TEST(1);
    _EXAMPLE_TEST(10);

    dbg->cats &= ~yama::bcode_exec_c & ~yama::ctx_llcmd_c;

    _EXAMPLE_TEST(100);
    _EXAMPLE_TEST(1000);

#undef _EXAMPLE_TEST
}

