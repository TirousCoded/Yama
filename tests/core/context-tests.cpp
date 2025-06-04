

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
            .consts = ctx.consts(),
            .panics = ctx.panics(),
            .panicking = ctx.panicking(),
            .is_user = ctx.is_user(),
            .call_frames = ctx.call_frames(),
            .max_call_frames = ctx.max_call_frames(),
            .max_locals = ctx.max_locals(),
        };
        for (size_t i = 0; i < ctx.args(); i++) result.args.push_back(ctx.arg(i).value());
        for (size_t i = 0; i < ctx.locals(); i++) result.locals.push_back(ctx.local(i).value());
        result.out_of_bounds_arg_is_as_expected = !ctx.arg(ctx.args());
        result.out_of_bounds_local_is_as_expected = !ctx.local(ctx.locals());
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
    class testing_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        yama::module_factory mf;


        testing_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& import_path) override final {
            if (import_path != ""_str) return std::nullopt;
            return yama::make_res<yama::module_info>(std::move(mf.done()));
        }
    };

    
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;

    std::shared_ptr<testing_parcel> our_parcel;

    bool ready = false;


    // IMPORTANT: remember that the first load actually builds the modules, so no more
    //            uploading once first loaded

    void upload(yama::type_info&& info) {
        our_parcel->mf.add_type(std::forward<yama::type_info>(info));
    }

    void upload_f();
    void upload_g();
    void upload_h();
    void upload_SomeStruct();


protected:
    void SetUp() override final {
        globals = Globals{};

        dbg = std::make_shared<yama::stderr_debug>();
        dm = std::make_shared<yama::domain>(dbg);
        ctx = std::make_shared<yama::context>(yama::res(dm), dbg);

        our_parcel = std::make_shared<testing_parcel>();

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

void ContextTests::upload_f() {
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred
        globals.snapshot_0 = CallStateSnapshot::make(ctx);

        if (ctx.push_arg(1).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 6,
        },
    };
    upload(std::move(f_info));
}

void ContextTests::upload_g() {
    const yama::const_table_info g_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:f"_str, yama::make_callsig_info({ 0 }, 0));
    auto g_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_fn(ctx.consts().type(1).value()).bad()) return; // f
        if (ctx.push_arg(1).bad()) return; // argument
        if (ctx.call(2, yama::newtop).bad()) return; // call f
        if (ctx.ret(0).bad()) return; // return result of call f
        };
    yama::type_info g_info{
        .unqualified_name = "g"_str,
        .consts = g_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = g_call_fn,
            .max_locals = 4,
        },
    };
    upload(std::move(g_info));
}

void ContextTests::upload_h() {
    const auto h_bcode =
        yama::bc::code()
        // block #1
        .add_put_const(yama::newtop, 1)
        .add_put_arg(yama::newtop, 1)
        .add_call(2, yama::newtop, true)
        .add_ret(0);
    std::cerr << h_bcode.fmt_disassembly() << "\n";
    const yama::const_table_info h_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:f"_str, yama::make_callsig_info({ 0 }, 0));
    yama::type_info h_info{
        .unqualified_name = "h"_str,
        .consts = h_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::bcode_call_fn,
            .max_locals = 2,
            .bcode = h_bcode,
        },
    };
    upload(std::move(h_info));
}

void ContextTests::upload_SomeStruct() {
    yama::type_info SomeStruct_info{
        .unqualified_name = "SomeStruct"_str,
        .consts = yama::const_table_info{},
        .info = yama::struct_info{},
    };
    upload(std::move(SomeStruct_info));
}


TEST_F(ContextTests, GetDomain) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->get_domain(), dm);
}

TEST_F(ContextTests, DM) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(&(ctx->dm()), dm.get());
}

TEST_F(ContextTests, Load) {
    ASSERT_TRUE(ready);

    // just quick-n'-dirty use this to get type f
    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));

    // expect load to be equiv to forward to domain

    EXPECT_EQ(ctx->load("yama::None"_str), dm->load("yama::None"_str));
    EXPECT_EQ(ctx->load("yama::Int"_str), dm->load("yama::Int"_str));
    EXPECT_EQ(ctx->load("abc::f"_str), dm->load("abc::f"_str));

    EXPECT_EQ(ctx->load("abc::missing"_str), dm->load("abc::missing"_str)); // test equiv failure #1

    // ensure above failure test actually failed

    EXPECT_FALSE(ctx->load("abc::missing"_str));
}

TEST_F(ContextTests, QuickAccessLoadMethods) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->none_type(), dm->none_type());
    EXPECT_EQ(ctx->int_type(), dm->int_type());
    EXPECT_EQ(ctx->uint_type(), dm->uint_type());
    EXPECT_EQ(ctx->float_type(), dm->float_type());
    EXPECT_EQ(ctx->bool_type(), dm->bool_type());
    EXPECT_EQ(ctx->char_type(), dm->char_type());
    EXPECT_EQ(ctx->type_type(), dm->type_type());
}


// LOW-LEVEL COMMAND INTERFACE TESTS

// TODO: add tests for '#_ref' methods later

TEST_F(ContextTests, ObjectRef_AsMethods) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->new_int(-100).as_int(), -100);
    EXPECT_EQ(ctx->new_uint(199).as_uint(), 199);
    EXPECT_DOUBLE_EQ(ctx->new_float(3.14159).as_float(), 3.14159);
    EXPECT_EQ(ctx->new_bool(true).as_bool(), true);
    EXPECT_EQ(ctx->new_char(U'魂').as_char(), U'魂');
    EXPECT_EQ(ctx->new_type(ctx->float_type()).as_type(), ctx->float_type());
}

TEST_F(ContextTests, ObjectRef_Equality_None) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_none();
    const yama::object_ref b = ctx->new_none();

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);

    EXPECT_EQ(b, b);
}

TEST_F(ContextTests, ObjectRef_Equality_Int) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_int(-41);
    const yama::object_ref b = ctx->new_int(-41);
    const auto c = ctx->new_int(106);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_UInt) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_uint(41);
    const yama::object_ref b = ctx->new_uint(41);
    const auto c = ctx->new_uint(106);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Float) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_float(-41.19);
    const yama::object_ref b = ctx->new_float(-41.19);
    const auto c = ctx->new_float(106.141);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Bool) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_bool(true);
    const yama::object_ref b = ctx->new_bool(true);
    const auto c = ctx->new_bool(false);

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Char) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_char(U'a');
    const yama::object_ref b = ctx->new_char(U'a');
    const auto c = ctx->new_char(U'魂');

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_TypeValue) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_type(ctx->bool_type());
    const yama::object_ref b = ctx->new_type(ctx->bool_type());
    const auto c = ctx->new_type(ctx->int_type());

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);

    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);

    EXPECT_EQ(c, c);
}

TEST_F(ContextTests, ObjectRef_Equality_Functions) {
    ASSERT_TRUE(ready);

    // remember that each fn type used w/ new_fn creates a object_ref
    // of a DIFFERENT TYPE, so the *correct* place to test inequality
    // between differing function typed object_ref is below in the
    // ObjectRef_Equality_InequalityIfTypesDiffer test

    // just quick-n'-dirty use this to get a type f
    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = ctx->load("abc:f"_str).value();

    const yama::object_ref a = ctx->new_fn(f).value();
    const yama::object_ref b = ctx->new_fn(f).value();

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);

    EXPECT_EQ(b, b);
}

TEST_F(ContextTests, ObjectRef_Equality_InequalityIfTypesDiffer) {
    ASSERT_TRUE(ready);

    const auto _Int = ctx->new_int(0);
    const auto _UInt = ctx->new_uint(0);
    const auto _Float = ctx->new_float(0.0);
    const auto _Bool = ctx->new_bool(false);
    const auto _Char = ctx->new_char(U'\0');
    const auto _Type = ctx->new_type(ctx->float_type());

    upload_f();
    upload_g();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:g"_str));
    const yama::type f = ctx->load("abc:f"_str).value();
    const yama::type g = ctx->load("abc:g"_str).value();

    const yama::object_ref _fn_f = ctx->new_fn(f).value();
    const yama::object_ref _fn_g = ctx->new_fn(g).value();

    EXPECT_NE(_Int, _UInt);
    EXPECT_NE(_Int, _Float);
    EXPECT_NE(_Int, _Bool);
    EXPECT_NE(_Int, _Char);
    EXPECT_NE(_Int, _Type);
    EXPECT_NE(_Int, _fn_f);
    EXPECT_NE(_Int, _fn_g);

    EXPECT_NE(_UInt, _Float);
    EXPECT_NE(_UInt, _Bool);
    EXPECT_NE(_UInt, _Char);
    EXPECT_NE(_UInt, _Type);
    EXPECT_NE(_UInt, _fn_f);
    EXPECT_NE(_UInt, _fn_g);

    EXPECT_NE(_Float, _Bool);
    EXPECT_NE(_Float, _Char);
    EXPECT_NE(_Float, _Type);
    EXPECT_NE(_Float, _fn_f);
    EXPECT_NE(_Float, _fn_g);

    EXPECT_NE(_Bool, _Char);
    EXPECT_NE(_Bool, _Type);
    EXPECT_NE(_Bool, _fn_f);
    EXPECT_NE(_Bool, _fn_g);

    EXPECT_NE(_Char, _Type);
    EXPECT_NE(_Char, _fn_f);
    EXPECT_NE(_Char, _fn_g);

    EXPECT_NE(_Type, _fn_f);
    EXPECT_NE(_Type, _fn_g);

    EXPECT_NE(_fn_f, _fn_g);
}

TEST_F(ContextTests, InitialState_UserCall) {
    ASSERT_TRUE(ready);

    EXPECT_FALSE(ctx->panicking());
    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);
    EXPECT_EQ(ctx->max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->args(), 0);
    EXPECT_EQ(ctx->locals(), 0);
    EXPECT_EQ(ctx->max_locals(), yama::user_max_locals);

    EXPECT_EQ(ctx->arg(0), std::nullopt); // out-of-bounds
    EXPECT_EQ(ctx->local(0), std::nullopt); // out-of-bounds
}

TEST_F(ContextTests, InitialState_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 13,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // f
    ASSERT_TRUE(ctx->call_nr(1).good());

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
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
        }
        EXPECT_TRUE(ss.locals.empty());

        EXPECT_TRUE(ss.out_of_bounds_arg_is_as_expected);
        EXPECT_TRUE(ss.out_of_bounds_local_is_as_expected);
    }
}

TEST_F(ContextTests, NewNone) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_none();

    EXPECT_EQ(a.t, dm->none_type());

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewInt) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_int(-41);

    EXPECT_EQ(a.t, dm->int_type());
    EXPECT_EQ(a.as_int(), -41);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewUInt) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_uint(106);

    EXPECT_EQ(a.t, dm->uint_type());
    EXPECT_EQ(a.as_uint(), 106);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewFloat) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_float(10.6);

    EXPECT_EQ(a.t, dm->float_type());
    EXPECT_EQ(a.as_float(), 10.6);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewBool) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_bool(true);

    EXPECT_EQ(a.t, dm->bool_type());
    EXPECT_EQ(a.as_bool(), true);

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewChar) {
    ASSERT_TRUE(ready);

    const yama::object_ref a = ctx->new_char(U'魂');

    EXPECT_EQ(a.t, dm->char_type());
    EXPECT_EQ(a.as_char(), U'魂');

    YAMA_LOG(dbg, yama::general_c, "{}", a);
}

TEST_F(ContextTests, NewType) {
    ASSERT_TRUE(ready);

    // just quick-n'-dirty use this to get type f
    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    const yama::object_ref a = ctx->new_type(ctx->bool_type());
    const yama::object_ref b = ctx->new_type(f);

    EXPECT_EQ(a.t, dm->type_type());
    EXPECT_EQ(a.as_type(), ctx->bool_type());

    EXPECT_EQ(b.t, dm->type_type());
    EXPECT_EQ(b.as_type(), f);

    YAMA_LOG(dbg, yama::general_c, "{}, {}", a, b);
}

TEST_F(ContextTests, NewFn) {
    ASSERT_TRUE(ready);

    // just quick-n'-dirty use this to get type f
    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    const auto a = ctx->new_fn(f);

    ASSERT_TRUE(a);
    EXPECT_EQ(a->t, f);
}

TEST_F(ContextTests, NewFn_FailDueToArgNotAFunctionType) {
    ASSERT_TRUE(ready);

    EXPECT_FALSE(ctx->new_fn(dm->int_type())); // Int is not a function type
}

TEST_F(ContextTests, Consts) {
    ASSERT_TRUE(ready);

    // NOTE: consts is tested via testing call since our 'snapshot' type captures its behaviour
    // NOTE: this empty test is here to detail the above note
}

TEST_F(ContextTests, Arg_UserCall) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->arg(0), std::nullopt); // no args
}

TEST_F(ContextTests, Arg_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(31).good()); // argument #1
    ASSERT_TRUE(ctx->push_float(1.33).good()); // argument #2
    ASSERT_TRUE(ctx->call_nr(3).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        // ss.arg# is sampled via arg, so below tests its usage
        if (ss.args.size() == 3) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(31));
            EXPECT_EQ(ss.args[2], ctx->new_float(1.33));
        }
        EXPECT_TRUE(ss.out_of_bounds_arg_is_as_expected);
    }
}

TEST_F(ContextTests, Local_UserCall) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->push_int(31).good());
    ASSERT_TRUE(ctx->push_none().good());
    ASSERT_TRUE(ctx->push_none().good());
    ASSERT_TRUE(ctx->push_char(U'y').good());

    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(31)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_none()));
    EXPECT_EQ(ctx->local(2), std::make_optional(ctx->new_none()));
    EXPECT_EQ(ctx->local(3), std::make_optional(ctx->new_char(U'y')));
    EXPECT_EQ(ctx->local(4), std::nullopt); // out-of-bounds
}

TEST_F(ContextTests, Local_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_bool(true).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.push_int(3).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.put_none(0).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // f
    ASSERT_TRUE(ctx->push_int(31).good()); // 31
    ASSERT_TRUE(ctx->call_nr(2).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 3);
        if (ss.locals.size() == 3) {
            EXPECT_EQ(ss.locals[0], ctx->new_bool(true));
            EXPECT_EQ(ss.locals[1], ctx->new_none());
            EXPECT_EQ(ss.locals[2], ctx->new_int(3));
        }
        EXPECT_TRUE(ss.out_of_bounds_local_is_as_expected);
    }
}

TEST_F(ContextTests, Panic) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        ctx.panic();
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        ctx.panic(); // should fail quietly
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);
    EXPECT_FALSE(ctx->panicking());

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
    EXPECT_FALSE(ctx->panicking()); // panic already finished

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);
    EXPECT_EQ(ctx->max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->locals(), 0);
    EXPECT_EQ(ctx->max_locals(), yama::user_max_locals);

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

TEST_F(ContextTests, Panic_UserCall) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);
    EXPECT_FALSE(ctx->panicking());

    // don't need to test panic failing quietly, as another panic
    // call *should* succeed here, as the initial panic will have 
    // completed by time the first panic returned

    ctx->panic();

    EXPECT_EQ(ctx->panics(), 1);
    EXPECT_FALSE(ctx->panicking()); // panic already finished

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);
    EXPECT_EQ(ctx->max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->locals(), 0);
    EXPECT_EQ(ctx->max_locals(), yama::user_max_locals);
}

TEST_F(ContextTests, Panic_MultiLevelCallStack) {
    ASSERT_TRUE(ready);

    // outer call
    const yama::const_table_info fa_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:fb"_str, yama::make_callsig_info({}, 0));
    auto fa_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_fn(yama::newtop, ctx.consts().type(1).value()).bad()) return;
        if (ctx.call_nr(1).bad()) return;
        // prepare return obj (which won't be reached due to panic)
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        globals.even_reached_0 = true;
        };
    yama::type_info fa_info{
        .unqualified_name = "fa"_str,
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
        .add_primitive_type("yama:None"_str);
    auto fb_call_fn =
        [](yama::context& ctx) {
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        ctx.panic();
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        ctx.panic(); // should fail quietly
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        };
    yama::type_info fb_info{
        .unqualified_name = "fb"_str,
        .consts = fb_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = fb_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(fa_info));
    upload(yama::type_info(fb_info));
    ASSERT_TRUE(dm->load("abc:fa"_str));
    ASSERT_TRUE(dm->load("abc:fb"_str));
    const yama::type fa = dm->load("abc:fa"_str).value();
    const yama::type fb = dm->load("abc:fb"_str).value();

    EXPECT_EQ(ctx->panics(), 0);
    EXPECT_FALSE(ctx->panicking());

    ASSERT_TRUE(ctx->push_fn(fa).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
    EXPECT_FALSE(ctx->panicking()); // panic already finished

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);
    EXPECT_EQ(ctx->max_call_frames(), yama::max_call_frames);
    EXPECT_EQ(ctx->locals(), 0);
    EXPECT_EQ(ctx->max_locals(), yama::user_max_locals);

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

TEST_F(ContextTests, Pop_UserCall) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->push_int(1).good());
    ASSERT_TRUE(ctx->push_int(2).good());
    ASSERT_TRUE(ctx->push_int(3).good());

    ASSERT_EQ(ctx->locals(), 3);

    ASSERT_TRUE(ctx->pop(2).good());

    ASSERT_EQ(ctx->locals(), 1);

    ASSERT_TRUE(ctx->pop(10).good()); // should stop prematurely

    ASSERT_EQ(ctx->locals(), 0);
}

TEST_F(ContextTests, Pop_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_int(1).bad()) return;
        if (ctx.push_int(2).bad()) return;
        if (ctx.push_int(3).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.pop(2).bad()) return;
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        if (ctx.pop(10).bad()) return; // should stop prematurely
        globals.snapshot_2 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());

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

TEST_F(ContextTests, Put_UserCall) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put(yama::newtop, ctx->new_int(-14)).good());

    EXPECT_EQ(ctx->locals(), 1);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(-14)));

    // overwrite at index
    ASSERT_TRUE(ctx->put(0, ctx->new_int(3)).good());

    EXPECT_EQ(ctx->locals(), 1);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(3)));
}

TEST_F(ContextTests, Put_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put(yama::newtop, ctx.new_int(-14)).bad()) return;
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        // overwrite at index
        if (ctx.put(0, ctx.new_int(3)).bad()) return;
        globals.snapshot_1 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->put_fn(yama::newtop, f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 1);
        if (ss.locals.size() == 1) {
            EXPECT_EQ(ss.locals[0], ctx->new_int(-14));
        }
    }
    EXPECT_TRUE(globals.snapshot_1);
    if (globals.snapshot_1) {
        const auto& ss = *globals.snapshot_1;
        EXPECT_EQ(ss.locals.size(), 1);
        if (ss.locals.size() == 1) {
            EXPECT_EQ(ss.locals[0], ctx->new_int(3));
        }
    }
}

TEST_F(ContextTests, Put_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->put(70, ctx->new_none()).bad()); // 70 is out-of-bounds! (and not newtop)

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Put_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    // saturate the stack
    for (size_t i = 0; i < yama::user_max_locals; i++) {
        ASSERT_TRUE(ctx->push_none().good()) << "i == " << i << "\n";
    }

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->put(yama::newtop, ctx->new_none()).bad()); // overflow!

    EXPECT_EQ(ctx->panics(), 1);
}

// NOTE: for the Put# tests below, for each put_# method, we
//       presume that they wrap calls to put, and thus need-not
//       be tested in regards to things like x out-of-bounds
//
//       notice this only applies to the put_# methods wrapping
//       compositions w/ new_# method calls, and does NOT include
//       put_arg, which is tested *fully*

TEST_F(ContextTests, PutNone) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_none(yama::newtop).good()); // via newtop
    ASSERT_TRUE(ctx->push_int(33).good()); // to be overwritten
    ASSERT_TRUE(ctx->put_none(1).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_none()));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_none()));
}

TEST_F(ContextTests, PutInt) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_int(yama::newtop, -14).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_int(1, 2).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(-14)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_int(2)));
}

TEST_F(ContextTests, PutUInt) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_uint(yama::newtop, 14).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_uint(1, 2).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_uint(14)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_uint(2)));
}

TEST_F(ContextTests, PutFloat) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_float(yama::newtop, 14.3).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_float(1, 2.315).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_float(14.3)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_float(2.315)));
}

TEST_F(ContextTests, PutBool) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_bool(yama::newtop, true).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_bool(1, false).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_bool(true)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_bool(false)));
}

TEST_F(ContextTests, PutChar) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_char(yama::newtop, 'y').good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_char(1, '3').good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_char('y')));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_char('3')));
}

TEST_F(ContextTests, PutType) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->put_type(yama::newtop, ctx->char_type()).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_type(1, ctx->int_type()).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_type(ctx->char_type())));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_type(ctx->int_type())));
}

TEST_F(ContextTests, PutFn) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->put_fn(yama::newtop, f).good()); // via newtop
    ASSERT_TRUE(ctx->push_none().good()); // to be overwritten
    ASSERT_TRUE(ctx->put_fn(1, f).good()); // via index

    EXPECT_EQ(ctx->locals(), 2);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_fn(f)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_fn(f)));
}

TEST_F(ContextTests, PutFn_PanicIfFIsNotCallable) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->put_fn(yama::newtop, ctx->bool_type()).bad()); // Bool is not callable!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutConst) {
    ASSERT_TRUE(ready);

    static_assert(
        []() constexpr -> bool {
            // NOTE: extend this each time we add a new loadable object constant
            static_assert(yama::const_types == 8);
            return
                yama::is_object_const(yama::int_const) &&
                yama::is_object_const(yama::uint_const) &&
                yama::is_object_const(yama::float_const) &&
                yama::is_object_const(yama::bool_const) &&
                yama::is_object_const(yama::char_const) &&
                !yama::is_object_const(yama::primitive_type_const) &&
                yama::is_object_const(yama::function_type_const) &&
                !yama::is_object_const(yama::struct_type_const);
        }());
    constexpr size_t nonobject_constant_types = 2;
    constexpr size_t loadable_object_constant_types = yama::const_types - nonobject_constant_types;
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        for (size_t i = 0; i < loadable_object_constant_types; i++) {
            if (ctx.push_none().bad()) return;
        }
        // I wanna have the x and c args differ so the impl can't so easily get away w/
        // confusing the two (as actually happened to be on initial impl of put_const)
        static_assert(loadable_object_constant_types == 6);
        if (ctx.put_const(0, 1).bad()) return; // via index
        if (ctx.put_const(1, 2).bad()) return; // via index
        if (ctx.put_const(2, 3).bad()) return; // via index
        if (ctx.put_const(3, 4).bad()) return; // via index
        if (ctx.put_const(4, 5).bad()) return; // via index
        if (ctx.put_const(5, 6).bad()) return; // via index
        static_assert(loadable_object_constant_types == 6);
        if (ctx.put_const(yama::newtop, 1).bad()) return; // via newtop
        if (ctx.put_const(yama::newtop, 2).bad()) return; // via newtop
        if (ctx.put_const(yama::newtop, 3).bad()) return; // via newtop
        if (ctx.put_const(yama::newtop, 4).bad()) return; // via newtop
        if (ctx.put_const(yama::newtop, 5).bad()) return; // via newtop
        if (ctx.put_const(yama::newtop, 6).bad()) return; // via newtop
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.put_none(0).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = yama::const_types * 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), loadable_object_constant_types * 2);
        if (ss.locals.size() == loadable_object_constant_types * 2) {
            static_assert(loadable_object_constant_types == 6);
            
            EXPECT_EQ(ss.locals[0], std::make_optional(ctx->new_int(-4)));
            EXPECT_EQ(ss.locals[1], std::make_optional(ctx->new_uint(301)));
            EXPECT_EQ(ss.locals[2], std::make_optional(ctx->new_float(3.14159)));
            EXPECT_EQ(ss.locals[3], std::make_optional(ctx->new_bool(true)));
            EXPECT_EQ(ss.locals[4], std::make_optional(ctx->new_char(U'y')));
            EXPECT_EQ(ss.locals[5], std::make_optional(ctx->new_fn(f)));
            
            EXPECT_EQ(ss.locals[6], std::make_optional(ctx->new_int(-4)));
            EXPECT_EQ(ss.locals[7], std::make_optional(ctx->new_uint(301)));
            EXPECT_EQ(ss.locals[8], std::make_optional(ctx->new_float(3.14159)));
            EXPECT_EQ(ss.locals[9], std::make_optional(ctx->new_bool(true)));
            EXPECT_EQ(ss.locals[10], std::make_optional(ctx->new_char(U'y')));
            EXPECT_EQ(ss.locals[11], std::make_optional(ctx->new_fn(f)));
        }
    }
}

TEST_F(ContextTests, PutConst_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    // the user call frame has no constants, so it's not really possible to test
    // this w/out having the constant be out-of-bounds

    ASSERT_TRUE(ctx->put_const(yama::newtop, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutConst_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_const(0, 0).bad()) return; // local register index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.put_none(yama::newtop).bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutConst_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_none().bad()) return; // saturate the stack
        if (ctx.put_const(yama::newtop, 0).bad()) return; // overflow!
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutConst_PanicIfCIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_const(yama::newtop, 70).bad()) return; // constant index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutConst_PanicIfCIsNotIndexOfAnObjectConstant) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_const(yama::newtop, 0).bad()) return; // constant at index 0 is NOT an object constant
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutTypeConst) {
    ASSERT_TRUE(ready);

    static_assert(
        []() constexpr -> bool {
            static_assert(yama::const_types == 8); // reminder
            return
                !yama::is_type_const(yama::int_const) &&
                !yama::is_type_const(yama::uint_const) &&
                !yama::is_type_const(yama::float_const) &&
                !yama::is_type_const(yama::bool_const) &&
                !yama::is_type_const(yama::char_const) &&
                yama::is_type_const(yama::primitive_type_const) &&
                yama::is_type_const(yama::function_type_const) &&
                yama::is_type_const(yama::struct_type_const);
        }());
    constexpr size_t type_constant_types = 2; // <- don't forget to update this!
    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Int"_str)
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        for (size_t i = 0; i < type_constant_types; i++) {
            if (ctx.push_none().bad()) return;
        }
        // I wanna have the x and c args differ so the impl can't so easily get away w/
        // confusing the two
        static_assert(type_constant_types == 2);
        if (ctx.put_type_const(0, 1).bad()) return; // via index
        if (ctx.put_type_const(1, 2).bad()) return; // via index
        static_assert(type_constant_types == 2);
        if (ctx.put_type_const(yama::newtop, 1).bad()) return; // via newtop
        if (ctx.put_type_const(yama::newtop, 2).bad()) return; // via newtop
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.put_none(0).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = type_constant_types * 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), type_constant_types * 2);
        if (ss.locals.size() == type_constant_types * 2) {
            static_assert(type_constant_types == 2);
            
            EXPECT_EQ(ss.locals[0], std::make_optional(ctx->new_type(ctx->int_type())));
            EXPECT_EQ(ss.locals[1], std::make_optional(ctx->new_type(f)));
            
            EXPECT_EQ(ss.locals[2], std::make_optional(ctx->new_type(ctx->int_type())));
            EXPECT_EQ(ss.locals[3], std::make_optional(ctx->new_type(f)));
        }
    }
}

TEST_F(ContextTests, PutTypeConst_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    // the user call frame has no constants, so it's not really possible to test
    // this w/out having the constant be out-of-bounds

    ASSERT_TRUE(ctx->put_type_const(yama::newtop, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutTypeConst_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_type_const(0, 0).bad()) return; // local register index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.put_none(yama::newtop).bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutTypeConst_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_none().bad()) return; // saturate the stack
        if (ctx.put_type_const(yama::newtop, 0).bad()) return; // overflow!
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutTypeConst_PanicIfCIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_type_const(yama::newtop, 70).bad()) return; // constant index out-of-bounds
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutTypeConst_PanicIfCIsNotIndexOfATypeConstant) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_int(10);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_type_const(yama::newtop, 1).bad()) return; // constant at index 1 is NOT a type constant
        globals.even_reached_0 = true; // shouldn't reach due to panic
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, PutArg) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_arg(yama::newtop, 0).bad()) return; // via newtop (callobj)
        if (ctx.push_none().bad()) return;
        if (ctx.put_arg(1, 1).bad()) return; // via index (argument)
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(2).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->call_nr(2).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 2);
        if (ss.locals.size() == 2) {
            EXPECT_EQ(ss.locals[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.locals[1], ctx->new_int(-14));
        }
    }
}

TEST_F(ContextTests, PutArg_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    // the user call frame has no arguments, so it's not really possible to test
    // this w/out having the arg param be out-of-bounds

    ASSERT_TRUE(ctx->put_arg(yama::newtop, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutArg_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_arg(70, 0).bad()) return; // 70 is out-of-bounds
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->call_nr(2).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutArg_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_none().bad()) return; // saturate the stack
        if (ctx.put_arg(yama::newtop, 0).bad()) return; // overflow!
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->call_nr(2).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, PutArg_PanicIfArgIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.put_arg(yama::newtop, 70).bad()) return; // 70 is out-of-bounds
        if (ctx.push_none().bad()) return;
        if (ctx.ret(1).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 1),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(-14).good()); // argument
    ASSERT_TRUE(ctx->call_nr(2).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Copy_UserCall) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->push_int(-14).good());
    ASSERT_TRUE(ctx->push_none().good());

    ASSERT_TRUE(ctx->copy(0, 1).good()); // via index
    ASSERT_TRUE(ctx->copy(0, yama::newtop).good()); // via newtop

    EXPECT_EQ(ctx->locals(), 3);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(-14)));
    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_int(-14)));
    EXPECT_EQ(ctx->local(2), std::make_optional(ctx->new_int(-14)));
}

TEST_F(ContextTests, Copy_NonUserCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        if (ctx.push_int(-14).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.copy(0, 1).bad()) return; // via index
        if (ctx.copy(0, yama::newtop).bad()) return; // via newtop
        globals.snapshot_0 = CallStateSnapshot::make(ctx);
        if (ctx.push_none().bad()) return;
        if (ctx.ret(3).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 4,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());

    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.locals.size(), 3);
        if (ss.locals.size() == 3) {
            EXPECT_EQ(ss.locals[0], std::make_optional(ctx->new_int(-14)));
            EXPECT_EQ(ss.locals[1], std::make_optional(ctx->new_int(-14)));
            EXPECT_EQ(ss.locals[2], std::make_optional(ctx->new_int(-14)));
        }
    }
}

TEST_F(ContextTests, Copy_OverwritesStateOfDest) {
    ASSERT_TRUE(ready);

    // copy is to overwrite uint 301 w/ a int -4

    ASSERT_TRUE(ctx->push_int(-4).good());
    ASSERT_TRUE(ctx->push_uint(301).good()); // old obj state
    
    ASSERT_TRUE(ctx->copy(0, 1).good());

    EXPECT_EQ(ctx->local(1), std::make_optional(ctx->new_int(-4)));
}

TEST_F(ContextTests, Copy_PanicIfSrcOutOfBounds) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->copy(70, yama::newtop).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Copy_PanicIfDestIsOutOfBounds) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_int(3).good());

    ASSERT_TRUE(ctx->copy(0, 70).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Copy_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    // saturate the stack
    for (size_t i = 0; i < yama::user_max_locals; i++) {
        ASSERT_TRUE(ctx->push_none().good()) << "i == " << i << "\n";
    }

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->copy(0).bad()); // overflow!

    EXPECT_EQ(ctx->panics(), 1);
}

static_assert(yama::kinds == 3); // reminder
static_assert(yama::ptypes == 7); // reminder

namespace default_init_helpers {
    // have tests for each ptype, plus others as needed, such as for
    // struct types, fn types, etc.
    constexpr size_t unique_cases = yama::ptypes + 2;
    constexpr size_t total_cases = unique_cases * 2;

    yama::const_table_info mk_testfn_consts() {
        return
            yama::const_table_info()
            .add_primitive_type("yama:None"_str)
            .add_primitive_type("yama:Int"_str)
            .add_primitive_type("yama:UInt"_str)
            .add_primitive_type("yama:Float"_str)
            .add_primitive_type("yama:Bool"_str)
            .add_primitive_type("yama:Char"_str)
            .add_primitive_type("yama:Type"_str)
            .add_function_type("self:f"_str, yama::make_callsig_info({ 1 }, 1))
            .add_struct_type("self:SomeStruct"_str);
    }
    
    yama::type_info mk_testfn(yama::call_fn call_fn, yama::const_table_info consts = mk_testfn_consts()) {
        return yama::type_info{
            .unqualified_name = "testfn"_str,
            .consts = std::move(consts),
            .info = yama::function_info{
                .callsig = yama::make_callsig_info({}, 0),
                .call_fn = call_fn,
                .max_locals = default_init_helpers::total_cases,
            },
        };
    }

    // the const_t overload has its testfn code directly in the unit test, w/
    // it using the const table itself instead of a case_types array

    void default_init_tester_for_type_overload(yama::context& ctx, yama::type f, yama::type SomeStruct) {
        std::array<yama::type, unique_cases> case_types{
            ctx.none_type(),
            ctx.int_type(),
            ctx.uint_type(),
            ctx.float_type(),
            ctx.bool_type(),
            ctx.char_type(),
            ctx.type_type(),
            f, // fn type
            SomeStruct, // struct type
        };

        // via newtop
        for (const auto& t : case_types) {
            ASSERT_TRUE(ctx.default_init(yama::newtop, t).good()) << "t == " << t;
        }

        // push dummy Int 10 objects for us to overwrite
        for (size_t i = 0; i < unique_cases; i++) {
            // we use Int 10 instead of None objects so default init of none
            // will be properly differentiatable
            ASSERT_TRUE(ctx.push_int(10).good());
        }

        // via index
        size_t index = unique_cases; // start after last 'via newtop' obj
        for (const auto& t : case_types) {
            ASSERT_TRUE(ctx.default_init(index++, t).good()) << "t == " << t << ", index == " << index - 1;
        }

        // so we don't miss anything
        ASSERT_EQ(ctx.locals(), total_cases);

        // check state

        // prepare + check non-empty
        std::array<std::optional<yama::object_ref>, total_cases> objs{};
        for (size_t i = 0; i < total_cases; i++) {
            objs[i] = ctx.local(i);
            ASSERT_TRUE(objs[i].has_value());
        }

        // check types (this presumes non-empty)
        for (size_t i = 0; i < unique_cases; i++) {
            // modulo to account for having two unique_cases length halfs
            ASSERT_EQ(objs[i]->t, case_types[i % unique_cases]);
        }

        // check values (this presumes correct types)
        for (size_t i = 0; i < total_cases; i += unique_cases) {
            // skip None (ie. i + 0)
            ASSERT_EQ(objs[i + 1]->as_int(), 0);
            ASSERT_EQ(objs[i + 2]->as_uint(), 0u);
            ASSERT_EQ(objs[i + 3]->as_float(), 0.0);
            ASSERT_EQ(objs[i + 4]->as_bool(), false);
            ASSERT_EQ(objs[i + 5]->as_char(), '\0');
            ASSERT_EQ(objs[i + 6]->as_type(), ctx.none_type());
            // skip f (ie. i + 7)
            // skip SomeStruct (ie. i + 8)
        }
    }
    void default_init_tester_for_const_overload(yama::context& ctx) {
        // via newtop
        for (yama::const_t c = 0; c < unique_cases; c++) {
            ASSERT_TRUE(ctx.default_init(yama::newtop, c).good()) << "c == " << c;
        }

        // push dummy Int 10 objects for us to overwrite
        for (size_t i = 0; i < unique_cases; i++) {
            // we use Int 10 instead of None objects so default init of none
            // will be properly differentiatable
            ASSERT_TRUE(ctx.push_int(10).good());
        }

        // via index
        size_t index = unique_cases; // start after last 'via newtop' obj
        for (yama::const_t c = 0; c < unique_cases; c++) {
            ASSERT_TRUE(ctx.default_init(index++, c).good()) << "c == " << c << ", index == " << index - 1;
        }

        // so we don't miss anything
        ASSERT_EQ(ctx.locals(), total_cases);

        // check state

        // prepare + check non-empty
        std::array<std::optional<yama::object_ref>, total_cases> objs{};
        for (size_t i = 0; i < total_cases; i++) {
            objs[i] = ctx.local(i);
            ASSERT_TRUE(objs[i].has_value());
        }

        // check types (this presumes non-empty)
        for (size_t i = 0; i < unique_cases; i++) {
            // modulo to account for having two unique_cases length halfs
            ASSERT_EQ(objs[i]->t, ctx.consts().type(i % unique_cases).value());
        }

        // check values (this presumes correct types)
        for (size_t i = 0; i < total_cases; i += unique_cases) {
            // skip None (ie. i + 0)
            ASSERT_EQ(objs[i + 1]->as_int(), 0);
            ASSERT_EQ(objs[i + 2]->as_uint(), 0u);
            ASSERT_EQ(objs[i + 3]->as_float(), 0.0);
            ASSERT_EQ(objs[i + 4]->as_bool(), false);
            ASSERT_EQ(objs[i + 5]->as_char(), '\0');
            ASSERT_EQ(objs[i + 6]->as_type(), ctx.none_type());
            // skip f (ie. i + 7)
            // skip SomeStruct (ie. i + 8)
        }
    }
}

TEST_F(ContextTests, DefaultInit_TypeOverload_UserCall) {
    ASSERT_TRUE(ready);

    upload_f();
    upload_SomeStruct();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:SomeStruct"_str));
    const yama::type f = dm->load("abc:f"_str).value();
    const yama::type SomeStruct = dm->load("abc:SomeStruct"_str).value();
    default_init_helpers::default_init_tester_for_type_overload(*ctx, f, SomeStruct);
}

TEST_F(ContextTests, DefaultInit_TypeOverload_NonUserCall) {
    ASSERT_TRUE(ready);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        const yama::type f = ctx.consts().type(7).value();
        const yama::type SomeStruct = ctx.consts().type(8).value();
        default_init_helpers::default_init_tester_for_type_overload(ctx, f, SomeStruct);
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());
}

TEST_F(ContextTests, DefaultInit_TypeOverload_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->default_init(70, ctx->none_type()).bad()); // 70 is out-of-bounds!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_TypeOverload_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    // saturate the stack
    for (size_t i = 0; i < yama::user_max_locals; i++) {
        ASSERT_TRUE(ctx->push_none().good()) << "i == " << i << "\n";
    }

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->default_init(yama::newtop, ctx->none_type()).bad()); // overflow!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_ConstOverload_NonUserCall) {
    ASSERT_TRUE(ready);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        default_init_helpers::default_init_tester_for_const_overload(ctx);
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).good());
}

TEST_F(ContextTests, DefaultInit_ConstOverload_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ready);

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->default_init(yama::newtop, 0).bad()); // illegal in user call frame!

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_ConstOverload_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        // panic! 70 is out-of-bounds!
        if (ctx.default_init(70, 0).bad()) return;

        FAIL() << "default_init didn't panic!";

        // *proper* exit
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad()); // should panic

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_ConstOverload_PanicIfPushingOverflows) {
    ASSERT_TRUE(ready);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        // saturate the stack
        for (size_t i = 0; i < ctx.max_locals(); i++) {
            ASSERT_TRUE(ctx.push_none().good()) << "i == " << i << "\n";
        }

        // panic! overflow!
        if (ctx.default_init(yama::newtop, 0).bad()) return;

        FAIL() << "default_init didn't panic!";

        // *proper* exit
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad()); // should panic

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_ConstOverload_PanicIfCIsOutOfBounds) {
    ASSERT_TRUE(ready);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        // panic! 70 is out-of-bounds!
        if (ctx.default_init(yama::newtop, 70).bad()) return;

        FAIL() << "default_init didn't panic!";

        // *proper* exit
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad()); // should panic

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, DefaultInit_ConstOverload_PanicIfCIsNotIndexOfATypeConstant) {
    ASSERT_TRUE(ready);

    auto testfn_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_float(3.14159);

    auto testfn_call_fn =
        [](yama::context& ctx) {
        // panic! Float (3.14159) is not type constant!
        if (ctx.default_init(yama::newtop, 1).bad()) return;

        FAIL() << "default_init didn't panic!";

        // *proper* exit
        if (ctx.pop(size_t(-1)).bad()) return;
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };

    upload_f();
    upload_SomeStruct();
    upload(default_init_helpers::mk_testfn(testfn_call_fn, testfn_consts));
    ASSERT_TRUE(dm->load("abc:testfn"_str));
    const yama::type testfn = dm->load("abc:testfn"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(testfn).good()); // call object
    ASSERT_TRUE(ctx->call_nr(1).bad()); // should panic

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Call_ViaIndex) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(2, 0).good()); // via index

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_EQ(ctx->locals(), 1);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, Call_ViaNewtop) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(2, yama::newtop).good()); // via newtop

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_EQ(ctx->locals(), 1);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(3)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, Call_MultiLevelCallStack) {
    ASSERT_TRUE(ready);

    upload_f();
    upload_g();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:g"_str));
    const yama::type f = dm->load("abc:f"_str).value();
    const yama::type g = dm->load("abc:g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(g).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(2, 0).good());

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_EQ(ctx->locals(), 1);
    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(3)));

    // call to g should have called f

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, Call_OverwritesStateOfRet) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    // call is to overwrite uint 301 w/ a int -4
    ASSERT_TRUE(ctx->push_uint(301).good()); // result (ie. overwrite old obj state)
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(-4).good()); // argument

    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->call(2, 0).good());

    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(-4)));
}

TEST_F(ContextTests, Call_BCodeExec) {
    ASSERT_TRUE(ready);

    upload_f();
    upload_h();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:h"_str));
    const yama::type f = dm->load("abc:f"_str).value();
    const yama::type h = dm->load("abc:h"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result goes here
    ASSERT_TRUE(ctx->push_fn(h).good()); // h
    ASSERT_TRUE(ctx->push_int(-4).good()); // -4
    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->call(2, 0).good());

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_int(-4)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 3);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(-4));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, Call_PanicIfArgsProvidesNoCallObj) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;

        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // <- would be callobj (but call won't specify having it)

    ASSERT_TRUE(ctx->call(0, yama::newtop).bad()); // no callobj -> panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, Call_PanicIfArgsIndexRangeIsOutOfBounds) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(70, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, Call_PanicIfRetWillBeOutOfBoundsAfterTheCall) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(2, 0).bad()); // R(0) is no longer valid after call

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, Call_PanicIfCallObjIsNotCallableType) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_float(13.02).good()); // call object, but float can't be!

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call(1, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Call_PanicIfUnexpectedArgCount_TooManyArgs) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument #1
    ASSERT_TRUE(ctx->push_int(3).good()); // argument #2

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(3, 0).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, Call_PanicIfUnexpectedArgCount_TooFewArgs) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(1, 0).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, Call_PanicIfCallStackOverflow) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and panic
        if (ctx.push_fn(ctx.consts().type(1).value()).bad()) return; // call object
        if (ctx.call(1, yama::newtop).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_none().good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call(1, 0).bad());

    EXPECT_EQ(ctx->panics(), 1);
    EXPECT_EQ(globals.call_depth, yama::max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, Call_PanicIfNoReturnValueObjectProvided) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        // return w/out ever calling ret
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_none().good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call(1, 0).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(globals.even_reached_0); // unlike others, this panic doesn't prevent call behaviour
}

TEST_F(ContextTests, CallNR) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call_nr(2).good());

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 2);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, CallNR_MultiLevelCallStack) {
    ASSERT_TRUE(ready);

    upload_f();
    upload_g();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:g"_str));
    const yama::type f = dm->load("abc:f"_str).value();
    const yama::type g = dm->load("abc:g"_str).value();

    // g will indirectly call f, and this tests that f was called

    ASSERT_TRUE(ctx->push_fn(g).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument
    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call_nr(2).good());

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    // call to g should have called f

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 3); // <- since f call was nested in g call
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(3));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, CallNR_BCodeExec) {
    ASSERT_TRUE(ready);

    upload_f();
    upload_h();
    ASSERT_TRUE(dm->load("abc:f"_str));
    ASSERT_TRUE(dm->load("abc:h"_str));
    const yama::type f = dm->load("abc:f"_str).value();
    const yama::type h = dm->load("abc:h"_str).value();

    ASSERT_TRUE(ctx->push_fn(h).good()); // h
    ASSERT_TRUE(ctx->push_int(-4).good()); // -4
    globals.int_arg_value_called_with = -4;
    ASSERT_TRUE(ctx->call_nr(2).good());

    EXPECT_TRUE(ctx->is_user());
    EXPECT_EQ(ctx->call_frames(), 1);

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
    EXPECT_TRUE(globals.snapshot_0);
    if (globals.snapshot_0) {
        const auto& ss = *globals.snapshot_0;
        EXPECT_EQ(ss.consts.size(), 1);
        EXPECT_TRUE(ss.consts.type(0));
        if (ss.consts.type(0)) {
            EXPECT_EQ(ss.consts.type(0).value(), dm->int_type());
        }

        EXPECT_FALSE(ss.is_user);
        EXPECT_EQ(ss.call_frames, 3);
        EXPECT_EQ(ss.max_call_frames, yama::max_call_frames);
        EXPECT_EQ(ss.max_locals, 6);

        EXPECT_EQ(ss.args.size(), 2);
        if (ss.args.size() == 2) {
            EXPECT_EQ(ss.args[0], ctx->new_fn(f).value());
            EXPECT_EQ(ss.args[1], ctx->new_int(-4));
        }

        EXPECT_TRUE(ss.locals.empty());
    }
}

TEST_F(ContextTests, CallNR_PanicIfArgsProvidesNoCallObj) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;

        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 2,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(f).good()); // <- would be callobj (but call_nr won't specify having it)

    ASSERT_TRUE(ctx->call_nr(0).bad()); // no callobj -> panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, CallNR_PanicIfArgsIndexRangeIsOutOfBounds) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call_nr(70).bad());

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, CallNR_PanicIfCallObjIsNotCallableType) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->push_float(13.02).good()); // call object, but float can't be!

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, CallNR_PanicIfUnexpectedArgCount_TooManyArgs) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object
    ASSERT_TRUE(ctx->push_int(3).good()); // argument #1
    ASSERT_TRUE(ctx->push_int(3).good()); // argument #2

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call_nr(3).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, CallNR_PanicIfUnexpectedArgCount_TooFewArgs) {
    ASSERT_TRUE(ready);

    upload_f();
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call_nr(1).bad()); // but f expects 1 arg

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_FALSE(globals.even_reached_0);
}

TEST_F(ContextTests, CallNR_PanicIfCallStackOverflow) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_function_type("self:f"_str, yama::make_callsig_info({}, 0));
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.call_depth++; // count number of calls until call stack overflow
        // infinitely recurse until we overflow the call stack and panic
        if (ctx.push_fn(ctx.consts().type(1).value()).bad()) return; // call object
        if (ctx.call(1, yama::newtop).bad()) return;
        if (ctx.ret(2).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call_nr(1).bad());

    EXPECT_EQ(ctx->panics(), 1);
    EXPECT_EQ(globals.call_depth, yama::max_call_frames - 1); // gotta -1 cuz of user call frame
}

TEST_F(ContextTests, CallNR_PanicIfNoReturnValueObjectProvided) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        // return w/out ever calling ret
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call_nr(1).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(globals.even_reached_0); // unlike others, this panic doesn't prevent call behaviour
}

// NOTE: most usage of ret will be tested as part of testing call

TEST_F(ContextTests, Ret_AllowsReturningWrongTypedObject) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true; // acknowledge that call behaviour actually occurred

        // tell ret to return a UInt, even though f should return a Int,
        // but the VM will allow for returning the wrong typed object
        
        // checking return type of return value object is beyond the scope 
        // of this level of abstraction, and so for safety this is allowed
        // as almost a kind of *feature* of the low-level command interface

        if (ctx.push_uint(301).bad()) return;
        if (ctx.ret(0).bad()) return;
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 3,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_int(0).good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    globals.int_arg_value_called_with = 3;
    ASSERT_TRUE(ctx->call(1, 0).good());

    EXPECT_EQ(ctx->local(0), std::make_optional(ctx->new_uint(301)));

    EXPECT_TRUE(globals.even_reached_0); // acknowledge f call behaviour occurred
}

TEST_F(ContextTests, Ret_PanicIfInUserCallFrame) {
    ASSERT_TRUE(ready);

    ASSERT_TRUE(ctx->is_user());

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->ret(0).bad());

    EXPECT_EQ(ctx->panics(), 1);
}

TEST_F(ContextTests, Ret_PanicIfXIsOutOfBounds) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        if (ctx.ret(70).bad()) return; // 70 is out-of-bounds
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_none().good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call(1, 0).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

TEST_F(ContextTests, Ret_PanicIfCalledMultipleTimesInOneCall) {
    ASSERT_TRUE(ready);

    const yama::const_table_info f_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);
    auto f_call_fn =
        [](yama::context& ctx) {
        globals.even_reached_0 = true;
        if (ctx.ret(0).bad()) return; // okay
        if (ctx.ret(0).bad()) return; // illegal
        };
    yama::type_info f_info{
        .unqualified_name = "f"_str,
        .consts = f_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn = f_call_fn,
            .max_locals = 1,
        },
    };
    upload(yama::type_info(f_info));
    ASSERT_TRUE(dm->load("abc:f"_str));
    const yama::type f = dm->load("abc:f"_str).value();

    ASSERT_TRUE(ctx->push_none().good()); // result
    ASSERT_TRUE(ctx->push_fn(f).good()); // call object

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->call(1, 0).bad()); // call behaviour internals should cause panic

    EXPECT_EQ(ctx->panics(), 1);

    EXPECT_TRUE(globals.even_reached_0);
}

