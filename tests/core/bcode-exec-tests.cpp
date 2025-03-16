

#include <gtest/gtest.h>

#include <yama/core/bcode.h>
#include <yama/core/domain.h>
#include <yama/core/context.h>


using namespace yama::string_literals;


// NOTE: when we're not focused on these tests, it's useful to turn off them logging

#define _DISABLE_LOGGING 1


class BCodeExecTests : public testing::Test {
public:

    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;


    // NOTE: the first load will *finalize* mf, so no more additions thereafter

    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        yama::module_factory mf;


        test_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) {
            if (relative_path != ""_str) return std::nullopt;
            return yama::make_res<yama::module_info>(std::move(mf.done()));
        }
    };

    std::shared_ptr<test_parcel> our_parcel;

    void upload(yama::type_info&& x) { our_parcel->mf.add_type(std::forward<yama::type_info>(x)); }

    bool ready = false;


protected:

    void SetUp() override final {
        dbg = std::make_shared<yama::stderr_debug>();
        dm = std::make_shared<yama::default_domain>(dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), dbg);

#if _DISABLE_LOGGING
        dbg->remove_cat(yama::bcode_exec_c | yama::ctx_llcmd_c);
#endif

        our_parcel = std::make_shared<test_parcel>();

        yama::install_batch ib{};
        ib
            .install("abc"_str, yama::res(our_parcel))
            .map_dep("abc"_str, "yama"_str, "yama"_str);

        ready = dm->install(std::move(ib));
    }

    void TearDown() override final {
        //
    }
};


// PER-INSTRUCTION TESTS

static_assert(yama::bc::opcodes == 12);

TEST_F(BCodeExecTests, Instr_Noop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_noop()
        .add_noop()
        .add_noop()
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Instr_Pop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_pop(1)
        .add_put_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Instr_Pop_Zero) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_pop(0)
        .add_put_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Instr_Pop_Many) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_none(yama::newtop)
        .add_put_none(yama::newtop)
        .add_put_none(yama::newtop)
        .add_pop(4)
        .add_put_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Instr_Pop_MoreThanAreOnStack) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_pop(100)
        .add_put_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Instr_PutNone) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_none(0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, Instr_PutNone_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, Instr_PutConst) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_const(0, 1, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(1.01);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(1.01));
}

TEST_F(BCodeExecTests, Instr_PutConst_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str)
        .add_float(1.01);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(1.01));
}

TEST_F(BCodeExecTests, Instr_PutArg) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_arg(0, 1, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_float(3.14159).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_float(-0.765).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(3.14159));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_float(-0.765));
}

TEST_F(BCodeExecTests, Instr_PutArg_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Float"_str);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_float(3.14159).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_float(-0.765).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(3.14159));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_float(-0.765));
}

TEST_F(BCodeExecTests, Instr_Copy) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_const(yama::newtop, 2)
        .add_copy(0, 1, true)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(10)
        .add_float(1.01);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));
}

TEST_F(BCodeExecTests, Instr_Copy_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_copy(0, yama::newtop)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(101);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

static bool was_called_0 = false;
static bool was_called_1 = false;

TEST_F(BCodeExecTests, Instr_Call) {
    ASSERT_TRUE(ready);

    auto plus_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        yama::int_t result =
            ctx.arg(1).value().as_int() +
            ctx.arg(2).value().as_int() +
            ctx.arg(3).value().as_int();
        if (ctx.push_int(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto plus_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    yama::type_info plus_info{
        .unqualified_name = "plus"_str,
        .consts = plus_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0, 0 }, 0),
            .call_fn = plus_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_const(yama::newtop, 1)
        .add_put_const(yama::newtop, 4)
        .add_put_const(yama::newtop, 3)
        .add_put_const(yama::newtop, 2)
        .add_call(4, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:plus"_str, yama::make_callsig_info({ 0, 0, 0 }, 0))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 5,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(plus_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(149));
}

TEST_F(BCodeExecTests, Instr_Call_Newtop) {
    ASSERT_TRUE(ready);

    auto plus_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        yama::int_t result =
            ctx.arg(1).value().as_int() +
            ctx.arg(2).value().as_int() +
            ctx.arg(3).value().as_int();
        if (ctx.push_int(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto plus_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    yama::type_info plus_info{
        .unqualified_name = "plus"_str,
        .consts = plus_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0, 0 }, 0),
            .call_fn = plus_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_const(yama::newtop, 4)
        .add_put_const(yama::newtop, 3)
        .add_put_const(yama::newtop, 2)
        .add_call(4, yama::newtop)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:plus"_str, yama::make_callsig_info({ 0, 0, 0 }, 0))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 5,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(plus_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(149));
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallBehaviourPanics) {
    ASSERT_TRUE(ready);

    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info panic_info{
        .unqualified_name = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .max_locals = 1,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // panic
        .add_call(1, yama::newtop)
        .add_put_const(yama::newtop, 2, true) // never_reached
        .add_call(1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(panic_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallBehaviourDoesNotReturnAnything) {
    ASSERT_TRUE(ready);

    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info panic_info{
        .unqualified_name = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .max_locals = 2,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // panic
        .add_call(1, yama::newtop, true) // just dump result into R(0)
        .add_put_const(yama::newtop, 2, true) // never_reached
        .add_call(1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(panic_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Call_PanicIfCallStackWouldOverflow) {
    ASSERT_TRUE(ready);

    ctx->dbg()->remove_cat(yama::bcode_exec_c); // make output easier to read

    // this 'dummy' function is here as it will be the thing that will actually trigger
    // the call stack overflow, which otherwise would be fail_safe, but I don't want it
    // to be the thing triggering it, as that's not its role (also the fact that it returns
    // something means it wouldn't work for testing call_nr)

    auto dummy_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto dummy_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info dummy_info{
        .unqualified_name = "dummy"_str,
        .consts = dummy_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = dummy_fn,
            .max_locals = 1,
        },
    };

    auto fail_safe_fn = 
        [](yama::context& ctx) {
        // in the event of the impl actually allowing for infinite (or at least until
        // the VM breaks) number of calls, this *fail safe* function will cause it to 
        // stop after proving its point, so we can get useful testing data
        const bool triggered = ctx.call_frames() > ctx.max_call_frames();
        if (triggered) {
            YAMA_LOG(ctx.dbg(), yama::general_c, "** fail safe triggered! **");
        }
        if (ctx.push_bool(triggered).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto fail_safe_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str);
    yama::type_info fail_safe_info{
        .unqualified_name = "fail_safe"_str,
        .consts = fail_safe_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fail_safe_fn,
            .max_locals = 1,
        },
    };

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 5) // dummy
        .add_call(1, yama::newtop, true) // <- the call instr under test
        .add_put_const(yama::newtop, 2, true) // fail_safe
        .add_call(1, 0, true)
        .add_jump_false(0, 2)
        // block #2
        .add_put_none(0, true)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 4, true) // f (recurse)
        .add_call(1, 0, true)
        // infinite recursion should induce panic by this point
        .add_put_const(yama::newtop, 3, true) // never_reached
        .add_call(1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_function_type("self:fail_safe"_str, yama::make_callsig_info({}, 1))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:dummy"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(dummy_info));
    upload(yama::type_info(fail_safe_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0); // fail-safe to ensure that test didn't pass due to f failing to fire
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR) {
    ASSERT_TRUE(ready);

    auto plus_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        yama::int_t result =
            ctx.arg(1).value().as_int() +
            ctx.arg(2).value().as_int() +
            ctx.arg(3).value().as_int();
        if (ctx.push_int(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto plus_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    yama::type_info plus_info{
        .unqualified_name = "plus"_str,
        .consts = plus_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0, 0 }, 0),
            .call_fn = plus_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 2)
        .add_put_const(yama::newtop, 3)
        .add_put_const(yama::newtop, 4)
        .add_put_const(yama::newtop, 5)
        .add_call_nr(4)
        .add_put_none(yama::newtop)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:plus"_str, yama::make_callsig_info({ 1, 1, 1 }, 1))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(plus_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallBehaviourPanics) {
    ASSERT_TRUE(ready);

    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info panic_info{
        .unqualified_name = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .max_locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // panic
        .add_call_nr(1)
        .add_put_const(yama::newtop, 2) // never_reached
        .add_call(1, yama::newtop)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(panic_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallBehaviourDoesNotReturnAnything) {
    ASSERT_TRUE(ready);

    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info panic_info{
        .unqualified_name = "panic"_str,
        .consts = panic_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = panic_fn,
            .max_locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1) // panic
        .add_call_nr(1)
        .add_put_const(yama::newtop, 2, true) // never_reached
        .add_call(1, yama::newtop, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:panic"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(panic_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0);
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_CallNR_PanicIfCallStackWouldOverflow) {
    ASSERT_TRUE(ready);

    ctx->dbg()->remove_cat(yama::bcode_exec_c); // make output easier to read

    // this 'dummy' function is here as it will be the thing that will actually trigger
    // the call stack overflow, which otherwise would be fail_safe, but I don't want it
    // to be the thing triggering it, as that's not its role (also the fact that it returns
    // something means it wouldn't work for testing call_nr)

    auto dummy_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto dummy_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info dummy_info{
        .unqualified_name = "dummy"_str,
        .consts = dummy_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = dummy_fn,
            .max_locals = 1,
        },
    };

    auto fail_safe_fn =
        [](yama::context& ctx) {
        // in the event of the impl actually allowing for infinite (or at least until
        // the VM breaks) number of calls, this *fail safe* function will cause it to 
        // stop after proving its point, so we can get useful testing data
        const bool triggered = ctx.call_frames() > ctx.max_call_frames();
        if (triggered) {
            YAMA_LOG(ctx.dbg(), yama::general_c, "** fail safe triggered! **");
        }
        if (ctx.push_bool(triggered).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto fail_safe_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Bool"_str);
    yama::type_info fail_safe_info{
        .unqualified_name = "fail_safe"_str,
        .consts = fail_safe_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fail_safe_fn,
            .max_locals = 1,
        },
    };

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    yama::type_info never_reached_info{
        .unqualified_name = "never_reached"_str,
        .consts = never_reached_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = never_reached_fn,
            .max_locals = 1,
        },
    };

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 5) // dummy
        .add_call_nr(1) // <- the call_nr instr under test
        .add_put_const(yama::newtop, 2, true) // fail_safe
        .add_call(1, yama::newtop, true)
        .add_jump_false(0, 2)
        // block #2
        .add_put_none(0, true)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 4, true) // f (recurse)
        .add_call(1, 0, true)
        // infinite recursion should induce panic by this point
        .add_put_const(yama::newtop, 3, true) // never_reached
        .add_call(1, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_function_type("self:fail_safe"_str, yama::make_callsig_info({}, 1))
        .add_function_type("self:never_reached"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0))
        .add_function_type("self:dummy"_str, yama::make_callsig_info({}, 0));
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(dummy_info));
    upload(yama::type_info(fail_safe_info));
    upload(yama::type_info(never_reached_info));
    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;
    was_called_1 = false;

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // expect panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(was_called_0); // fail-safe to ensure that test didn't pass due to f failing to fire
    EXPECT_FALSE(was_called_1); // should abort before never_reached call
}

TEST_F(BCodeExecTests, Instr_Ret) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_const(yama::newtop, 2)
        .add_put_const(yama::newtop, 3)
        .add_ret(1);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str)
        .add_int(-10)
        .add_uint(4)
        .add_float(1.01);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 3,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_uint(4));
}

TEST_F(BCodeExecTests, Instr_Jump) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_jump(2)
        // block #2
        .add_put_const(yama::newtop, 1)
        .add_ret(0)
        // block #3
        .add_put_const(yama::newtop, 2)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_int(70)
        .add_int(-7);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-7));
}

TEST_F(BCodeExecTests, Instr_JumpTrue) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 1) // push Bool to index 0
        .add_jump_true(1, 2) // pop Bool
        // block #2
        .add_put_const(yama::newtop, 2) // push Int to index 0
        .add_ret(0) // assert index 0 register is Int
        // block #3
        .add_put_const(yama::newtop, 3) // push Int to index 0
        .add_ret(0); // assert index 0 register is Int
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_int(-10)
        .add_int(4);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_bool(true).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_bool(false).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(4));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_int(-10));
}

TEST_F(BCodeExecTests, Instr_JumpFalse) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_arg(yama::newtop, 1) // push Bool to index 0
        .add_jump_false(1, 2) // pop Bool
        // block #2
        .add_put_const(yama::newtop, 2) // push Int to index 0
        .add_ret(0) // assert index 0 register is Int
        // block #3
        .add_put_const(yama::newtop, 3) // push Int to index 0
        .add_ret(0); // assert index 0 register is Int
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_int(-10)
        .add_int(4);
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 1 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 1,
            .bcode = f_bcode,
        },
    };

    upload(yama::type_info(f_info));

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_bool(true).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->push_bool(false).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-10));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_int(4));
}

// EXAMPLE TESTS

static yama::uint_t example_fac(yama::uint_t n) noexcept {
    return n == 0 ? 1 : n * example_fac(n - 1);
}

TEST_F(BCodeExecTests, Example_Factorial) {
    ASSERT_TRUE(ready);

    // test impl of a basic factorial function for testing recursion

    const auto subtract_fn =
        [](yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_uint();
        const auto b = ctx.arg(2).value().as_uint();
        const auto result = a - b;
        if (ctx.push_uint(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto subtract_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str);
    yama::type_info subtract_info{
        .unqualified_name = "subtract"_str,
        .consts = subtract_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 0),
            .call_fn = subtract_fn,
            .max_locals = 1,
        },
    };

    const auto multiply_fn =
        [](yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_uint();
        const auto b = ctx.arg(2).value().as_uint();
        const auto result = a * b;
        if (ctx.push_uint(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto multiply_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str);
    yama::type_info multiply_info{
        .unqualified_name = "multiply"_str,
        .consts = multiply_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 0),
            .call_fn = multiply_fn,
            .max_locals = 1,
        },
    };
    
    const auto greaterThanZero_fn =
        [](yama::context& ctx) {
        const auto success = ctx.arg(1).value().as_uint() > 0;
        if (ctx.push_bool(success).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto greaterThanZero_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Bool"_str);
    yama::type_info greaterThanZero_info{
        .unqualified_name = "greaterThanZero"_str,
        .consts = greaterThanZero_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = greaterThanZero_fn,
            .max_locals = 1,
        },
    };

    const auto factorial_bcode =
        yama::bc::code()
        // NOTE: below, the arg of the fn will be called 'n'
        // block #1
        .add_put_const(yama::newtop, 4) // greaterThanZero
        .add_put_arg(yama::newtop, 1) // n
        .add_call(2, yama::newtop, true) // check if n > 0
        .add_jump_true(1, 2) // jump to block #3 if true, fallthrough to block #2 otherwise
        // block #2
        .add_put_const(yama::newtop, 6) // 1
        .add_ret(0) // return 1 if n == 0
        // block #3
        // (begin) eval n * (n - 1)!
        .add_put_const(yama::newtop, 3) // multiply (index 0)
        .add_put_arg(yama::newtop, 1) // n (index 1)
        // (begin) eval (n - 1)!
        .add_put_const(yama::newtop, 5) // factorial (index 2)
        // (begin) eval n - 1
        .add_put_const(yama::newtop, 2) // subtract (index 3)
        .add_put_arg(yama::newtop, 1) // n (index 4)
        .add_put_const(yama::newtop, 6) // 1 (index 5)
        .add_call(3, yama::newtop, true) // n - 1 (index 3)
        // (end) eval n - 1
        .add_call(2, yama::newtop, true) // (n - 1)! (index 2)
        // (end) eval (n - 1)!
        .add_call(3, yama::newtop, true) // n * (n - 1)! (index 0)
        // (end) eval n * (n - 1)!
        .add_ret(0); // return n * (n - 1)! if n > 0
    std::cerr << factorial_bcode.fmt_disassembly() << "\n";
    const auto factorial_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_function_type("self:subtract"_str, yama::make_callsig_info({ 0, 0 }, 0))
        .add_function_type("self:multiply"_str, yama::make_callsig_info({ 0, 0 }, 0))
        .add_function_type("self:greaterThanZero"_str, yama::make_callsig_info({ 0 }, 1))
        .add_function_type("self:factorial"_str, yama::make_callsig_info({ 0 }, 0))
        .add_uint(1);
    yama::type_info factorial_info{
        .unqualified_name = "factorial"_str,
        .consts = factorial_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 6,
            .bcode = factorial_bcode,
        },
    };

    upload(yama::type_info(subtract_info));
    upload(yama::type_info(multiply_info));
    upload(yama::type_info(greaterThanZero_info));
    upload(yama::type_info(factorial_info));

    const auto factorial = ctx->load("abc:factorial"_str).value();

    for (yama::uint_t i = 0; i <= 11; i++) {
        const auto expected = example_fac(i);

        YAMA_LOG(dbg, yama::general_c, "{}! == {}", size_t(i), size_t(expected));

        ASSERT_TRUE(ctx->push_fn(factorial).good());
        ASSERT_TRUE(ctx->push_uint(i).good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        const auto result = ctx->local(0).value();

        ASSERT_TRUE(ctx->pop(1).good());

        YAMA_LOG(dbg, yama::general_c, "bcode result: {}", result.fmt());

        EXPECT_EQ(result, ctx->new_uint(expected));
    }
}

TEST_F(BCodeExecTests, Example_Counter) {
    ASSERT_TRUE(ready);

    // test impl of a function which increments a counter local
    // variable register some number of times, returning its final
    // value as result, in order to test looping

    const auto addOne_fn =
        [](yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_uint();
        const auto result = a + 1;
        if (ctx.push_uint(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto addOne_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str);
    yama::type_info addOne_info{
        .unqualified_name = "addOne"_str,
        .consts = addOne_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = addOne_fn,
            .max_locals = 1,
        },
    };

    const auto lessThan_fn =
        [](yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_uint();
        const auto b = ctx.arg(2).value().as_uint();
        const auto result = a < b;
        if (ctx.push_bool(result).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto lessThan_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Bool"_str);
    yama::type_info lessThan_info{
        .unqualified_name = "lessThan"_str,
        .consts = lessThan_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 0 }, 1),
            .call_fn = lessThan_fn,
            .max_locals = 1,
        },
    };

    const auto counter_bcode =
        yama::bc::code()
        // NOTE: below, the arg of the fn will be called 'n'
        // block #1
        .add_put_const(yama::newtop, 4) // init 'counter' to 0 (index 0)
        // block #2
        // eval counter < n
        .add_put_const(yama::newtop, 3) // lessThan (index 1)
        .add_copy(0, yama::newtop) // counter (index 2)
        .add_put_arg(yama::newtop, 1) // n (index 3)
        .add_call(3, yama::newtop, true) // counter < n (index 1)
        .add_jump_false(1, 4) // jump to return instr if counter >= n
        // block #3
        // eval counter = addOne(counter)
        .add_put_const(yama::newtop, 2) // addOne (index 1)
        .add_copy(0, yama::newtop) // counter (index 2)
        .add_call(2, 0) // counter = addOne(counter)
        .add_jump(-9) // jump back to block #2 to begin next iter of loop
        // block #4
        .add_ret(0); // return final value of counter
    std::cerr << counter_bcode.fmt_disassembly() << "\n";
    const auto counter_consts =
        yama::const_table_info()
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_function_type("self:addOne"_str, yama::make_callsig_info({ 0 }, 0))
        .add_function_type("self:lessThan"_str, yama::make_callsig_info({ 0, 0 }, 1))
        .add_uint(0);
    yama::type_info counter_info{
        .unqualified_name = "counter"_str,
        .consts = counter_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 4,
            .bcode = counter_bcode,
        },
    };

    upload(yama::type_info(addOne_info));
    upload(yama::type_info(lessThan_info));
    upload(yama::type_info(counter_info));

    const auto counter = ctx->dm().load("abc:counter"_str).value();

#define _EXAMPLE_TEST(n) \
{ \
    YAMA_LOG(dbg, yama::general_c, "counter({0}) == {0}", n); \
    \
    ASSERT_TRUE(ctx->push_fn(counter).good()); \
    ASSERT_TRUE(ctx->push_uint(n).good()); \
    ASSERT_TRUE(ctx->call(2, yama::newtop).good()); \
    \
    const auto result = ctx->local(0).value(); \
    \
    ASSERT_TRUE(ctx->pop(1).good()); \
    \
    YAMA_LOG(dbg, yama::general_c, "bcode result: {}", result.fmt()); \
    \
    EXPECT_EQ(result, ctx->new_uint(n)); \
} (void)0

    _EXAMPLE_TEST(0);
    _EXAMPLE_TEST(1);
    _EXAMPLE_TEST(10);

    dbg->remove_cat(yama::bcode_exec_c | yama::ctx_llcmd_c);

    _EXAMPLE_TEST(100);
    _EXAMPLE_TEST(1000);

#undef _EXAMPLE_TEST
}

