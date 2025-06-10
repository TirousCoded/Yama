

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

    void upload(yama::type_info x) { our_parcel->mf.add(std::move(x)); }

    bool ready = false;


protected:

    void SetUp() override final {
        dbg = std::make_shared<yama::stderr_debug>();
        dm = std::make_shared<yama::domain>(dbg);
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

static_assert(yama::bc::opcodes == 14);

TEST_F(BCodeExecTests, Noop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Pop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Pop_Zero) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Pop_Many) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, Pop_MoreThanAreOnStack) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

TEST_F(BCodeExecTests, PutNone) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, PutNone_Newtop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, PutConst) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(1.01));
}

TEST_F(BCodeExecTests, PutConst_Newtop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(1.01));
}

TEST_F(BCodeExecTests, PutTypeConst) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_put_type_const(0, 1, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_type(ctx->bool_type()));
}

TEST_F(BCodeExecTests, PutTypeConst_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_type_const(yama::newtop, 1)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Type"_str)
        .add_primitive_type("yama:Bool"_str);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_type(ctx->bool_type()));
}

TEST_F(BCodeExecTests, PutArg) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0 }, 0),
        1,
        f_bcode);

    upload(f_info);

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

TEST_F(BCodeExecTests, PutArg_Newtop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 0 }, 0),
        1,
        f_bcode);

    upload(f_info);

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

TEST_F(BCodeExecTests, Copy) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));
}

TEST_F(BCodeExecTests, Copy_Newtop) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(101));
}

// TODO: currently, our default_init unit tests don't properly cover ALL the different
//       return values default_init could result in, instead just presuming that we
//       can just test that it forwards to a context::default_init call
//
//       I don't like how it makes this assumption, so at some point revise our tests
//       to properly test each particular return value for each type kind/ptype

TEST_F(BCodeExecTests, DefaultInit) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_put_none(yama::newtop)
        .add_default_init(0, 0, true)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(0));
}

TEST_F(BCodeExecTests, DefaultInit_Newtop) {
    ASSERT_TRUE(ready);

    const auto f_bcode =
        yama::bc::code()
        // block #1
        .add_default_init(yama::newtop, 0)
        .add_ret(0);
    std::cerr << f_bcode.fmt_disassembly() << "\n";
    const auto f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(0));
}

static bool was_called_0 = false;
static bool was_called_1 = false;

TEST_F(BCodeExecTests, Call) {
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
    const auto plus_info = yama::make_function(
        "plus"_str,
        plus_consts,
        yama::make_callsig({ 0, 0, 0 }, 0),
        1,
        plus_fn);

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
        .add_function_type("self:plus"_str, yama::make_callsig({ 0, 0, 0 }, 0))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    upload(plus_info);
    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(149));
}

TEST_F(BCodeExecTests, Call_Newtop) {
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
    const auto plus_info = yama::make_function(
        "plus"_str,
        plus_consts,
        yama::make_callsig({ 0, 0, 0 }, 0),
        1,
        plus_fn);

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
        .add_function_type("self:plus"_str, yama::make_callsig({ 0, 0, 0 }, 0))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        5,
        f_bcode);

    upload(plus_info);
    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(149));
}

TEST_F(BCodeExecTests, Call_PanicIfCallBehaviourPanics) {
    ASSERT_TRUE(ready);

    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto panic_info = yama::make_function(
        "panic"_str,
        panic_consts,
        yama::make_callsig({}, 0),
        1,
        panic_fn);

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:panic"_str, yama::make_callsig({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(panic_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, Call_PanicIfCallBehaviourDoesNotReturnAnything) {
    ASSERT_TRUE(ready);

    auto panic_fn = 
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto panic_info = yama::make_function(
        "panic"_str,
        panic_consts,
        yama::make_callsig({}, 0),
        2,
        panic_fn);

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:panic"_str, yama::make_callsig({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(panic_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, Call_PanicIfCallStackWouldOverflow) {
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
    const auto dummy_info = yama::make_function(
        "dummy"_str,
        dummy_consts,
        yama::make_callsig({}, 0),
        1,
        dummy_fn);

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
    const auto fail_safe_info = yama::make_function(
        "fail_safe"_str,
        fail_safe_consts,
        yama::make_callsig({}, 0),
        1,
        fail_safe_fn);

    auto never_reached_fn = 
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:fail_safe"_str, yama::make_callsig({}, 1))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0))
        .add_function_type("self:f"_str, yama::make_callsig({}, 0))
        .add_function_type("self:dummy"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(dummy_info);
    upload(fail_safe_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, CallNR) {
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
    const auto plus_info = yama::make_function(
        "plus"_str,
        plus_consts,
        yama::make_callsig({ 0, 0, 0 }, 0),
        1,
        plus_fn);

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
        .add_function_type("self:plus"_str, yama::make_callsig({ 1, 1, 1 }, 1))
        .add_int(1)
        .add_int(48)
        .add_int(100);
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        4,
        f_bcode);

    upload(plus_info);
    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    was_called_0 = false;

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_TRUE(was_called_0);

    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
}

TEST_F(BCodeExecTests, CallNR_PanicIfCallBehaviourPanics) {
    ASSERT_TRUE(ready);

    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        ctx.panic();
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto panic_info = yama::make_function(
        "panic"_str,
        panic_consts,
        yama::make_callsig({}, 0),
        1,
        panic_fn);

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:panic"_str, yama::make_callsig({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(panic_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, CallNR_PanicIfCallBehaviourDoesNotReturnAnything) {
    ASSERT_TRUE(ready);

    auto panic_fn =
        [](yama::context& ctx) {
        was_called_0 = true;
        // should induce panic by failing to call ll_ret
        };
    const auto panic_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto panic_info = yama::make_function(
        "panic"_str,
        panic_consts,
        yama::make_callsig({}, 0),
        1,
        panic_fn);

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:panic"_str, yama::make_callsig({}, 0))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(panic_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, CallNR_PanicIfCallStackWouldOverflow) {
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
    const auto dummy_info = yama::make_function(
        "dummy"_str,
        dummy_consts,
        yama::make_callsig({}, 0),
        1,
        dummy_fn);

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
    const auto fail_safe_info = yama::make_function(
        "fail_safe"_str,
        fail_safe_consts,
        yama::make_callsig({}, 0),
        1,
        fail_safe_fn);

    auto never_reached_fn =
        [](yama::context& ctx) {
        was_called_1 = true;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    const auto never_reached_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    const auto never_reached_info = yama::make_function(
        "never_reached"_str,
        never_reached_consts,
        yama::make_callsig({}, 0),
        1,
        never_reached_fn);

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
        .add_function_type("self:fail_safe"_str, yama::make_callsig({}, 1))
        .add_function_type("self:never_reached"_str, yama::make_callsig({}, 0))
        .add_function_type("self:f"_str, yama::make_callsig({}, 0))
        .add_function_type("self:dummy"_str, yama::make_callsig({}, 0));
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        2,
        f_bcode);

    upload(dummy_info);
    upload(fail_safe_info);
    upload(never_reached_info);
    upload(f_info);

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

TEST_F(BCodeExecTests, Ret) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        3,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_uint(4));
}

TEST_F(BCodeExecTests, Jump) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({}, 0),
        1,
        f_bcode);

    upload(f_info);

    const auto f = ctx->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-7));
}

TEST_F(BCodeExecTests, JumpTrue) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 1 }, 0),
        1,
        f_bcode);

    upload(f_info);

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

TEST_F(BCodeExecTests, JumpFalse) {
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
    const auto f_info = yama::make_function(
        "f"_str,
        f_consts,
        yama::make_callsig({ 1 }, 0),
        1,
        f_bcode);

    upload(f_info);

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
    const auto subtract_info = yama::make_function(
        "subtract"_str,
        subtract_consts,
        yama::make_callsig({ 0, 0 }, 0),
        1,
        subtract_fn);

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
    const auto multiply_info = yama::make_function(
        "multiply"_str,
        multiply_consts,
        yama::make_callsig({ 0, 0 }, 0),
        1,
        multiply_fn);
    
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
    const auto greaterThanZero_info = yama::make_function(
        "greaterThanZero"_str,
        greaterThanZero_consts,
        yama::make_callsig({ 0 }, 1),
        1,
        greaterThanZero_fn);

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
        .add_function_type("self:subtract"_str, yama::make_callsig({ 0, 0 }, 0))
        .add_function_type("self:multiply"_str, yama::make_callsig({ 0, 0 }, 0))
        .add_function_type("self:greaterThanZero"_str, yama::make_callsig({ 0 }, 1))
        .add_function_type("self:factorial"_str, yama::make_callsig({ 0 }, 0))
        .add_uint(1);
    const auto factorial_info = yama::make_function(
        "factorial"_str,
        factorial_consts,
        yama::make_callsig({ 0 }, 0),
        6,
        factorial_bcode);

    upload(subtract_info);
    upload(multiply_info);
    upload(greaterThanZero_info);
    upload(factorial_info);

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
    const auto addOne_info = yama::make_function(
        "addOne"_str,
        addOne_consts,
        yama::make_callsig({ 0 }, 0),
        1,
        addOne_fn);

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
    const auto lessThan_info = yama::make_function(
        "lessThan"_str,
        lessThan_consts,
        yama::make_callsig({ 0, 0 }, 1),
        1,
        lessThan_fn);

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
        .add_function_type("self:addOne"_str, yama::make_callsig({ 0 }, 0))
        .add_function_type("self:lessThan"_str, yama::make_callsig({ 0, 0 }, 1))
        .add_uint(0);
    const auto counter_info = yama::make_function(
        "counter"_str,
        counter_consts,
        yama::make_callsig({ 0 }, 0),
        4,
        counter_bcode);

    upload(addOne_info);
    upload(lessThan_info);
    upload(counter_info);

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

