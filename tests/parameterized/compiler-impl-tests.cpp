

#include "compiler-impl-tests.h"

#include <yama/core/general.h>
#include <yama/core/scalars.h>
#include <yama/core/callsig_info.h>
#include <yama/core/const_table_info.h>


using namespace yama::string_literals;


GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(CompilerImplTests);


// IMPORTANT:
//      our policy is gonna be to only testing a single error per unit test, not testing
//      multiple different errors arising in one unit test
//
//      the reason is so that the impl is free to forgo work checking for other errors
//      after one is found, nor any complexity about certain errors having to be grouped
//      together w/ others
//
//      one error may correspond to multiple dsignal raises


// IMPORTANT:
//      our policy is gonna be to allow for the impl to define implementation-defined
//      limits on things like the max number of allowed local vars, temporaries, branch
//      jump distance, etc.


// IMPORTANT:
//      in order to unit test yama::compiler impls in a way that ensures we don't
//      couple our tests to impl details, we're gonna assert the following:
// 
//          1) if the code compiles
// 
//          2) if, once uploaded to a domain, the domain is then populated w/
//             the expected compiled types, w/ the expected frontend details
// 
//              * we're NOT gonna be asserting anything in regards to the vector
//                of type_info returned by compile, as I don't want our tests to
//                be coupled to this info, as we may change type_info later
// 
//              * this will also check whether or not output type_info vector
//                even survives static verification
// 
//          3) if executing compiled functions produces the expected return value
// 
//          4) if executing compiled functions produces the expected sequence
//             of observable side effects
//
//      the above are in regards to tests of successful compilation, w/ failure
//      tests simply asserting failed compilation and expected debug signals


// for checking side-effects, we'll define a datastructure for recording a sequence
// of side effect outputs, and comparing sequences, and then we'll define some
// special Yama functions which then produce these observable side-effects

struct sidefx_t final {
    std::string seq;


    // in our tests, compare by fmt() return value, as googletest will produce a
    // really nice diff for us to debug w/

    std::string fmt() const { return seq; }


    // gonna exclude observing Float side-effect details as I worry asserting those
    // details could lead to issues

    void observe_none() { seq += std::format("none n/a\n"); }
    void observe_int(yama::int_t x) { seq += std::format("int {}\n", yama::fmt_int(x)); }
    void observe_uint(yama::uint_t x) { seq += std::format("uint {}\n", yama::fmt_uint(x)); }
    void observe_float(yama::float_t) { seq += std::format("float n/a\n"); }
    void observe_bool(yama::bool_t x) { seq += std::format("bool {}\n", yama::fmt_bool(x)); }
    void observe_char(yama::char_t x) { seq += std::format("char {}\n", yama::fmt_char(x)); }
};

sidefx_t sidefx; // global so our Yama functions can reference it


// our Yama functions for side-effects + misc

const auto consts =
yama::const_table_info()
.add_primitive_type("None"_str)
.add_primitive_type("Int"_str)
.add_primitive_type("UInt"_str)
.add_primitive_type("Float"_str)
.add_primitive_type("Bool"_str)
.add_primitive_type("Char"_str);

yama::type_info observeNone_info{
    .fullname = "observeNone"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 0 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_none();
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info observeInt_info{
    .fullname = "observeInt"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 1 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_int(ctx.arg(1).value().as_int());
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info observeUInt_info{
    .fullname = "observeUInt"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 2 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_uint(ctx.arg(1).value().as_uint());
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info observeFloat_info{
    .fullname = "observeFloat"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 3 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_float(ctx.arg(1).value().as_float());
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info observeBool_info{
    .fullname = "observeBool"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 4 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_bool(ctx.arg(1).value().as_bool());
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info observeChar_info{
    .fullname = "observeChar"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 5 }, 0),
        .call_fn =
            [](yama::context& ctx) {
                sidefx.observe_char(ctx.arg(1).value().as_char());
                if (ctx.push_none().bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info doPanic_info{
    .fullname = "doPanic"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({}, 0),
        .call_fn =
            [](yama::context& ctx) {
                ctx.panic();
            },
        .max_locals = 0,
    },
};
yama::type_info doIncr_info{
    .fullname = "doIncr"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 1 }, 1),
        .call_fn =
            [](yama::context& ctx) {
                if (ctx.push_int(ctx.arg(1).value().as_int() + 1).bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info isEqInt_info{
    .fullname = "isEqInt"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 1, 1 }, 4),
        .call_fn =
            [](yama::context& ctx) {
                const auto a = ctx.arg(1).value().as_int();
                const auto b = ctx.arg(2).value().as_int();
                if (ctx.push_bool(a == b).bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info isEqChar_info{
    .fullname = "isEqChar"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 5, 5 }, 4),
        .call_fn =
            [](yama::context& ctx) {
                const auto a = ctx.arg(1).value().as_char();
                const auto b = ctx.arg(2).value().as_char();
                if (ctx.push_bool(a == b).bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};
yama::type_info isNotEqInt_info{
    .fullname = "isNotEqInt"_str,
    .consts = consts,
    .info = yama::function_info{
        .callsig = yama::make_callsig_info({ 1, 1 }, 4),
        .call_fn =
            [](yama::context& ctx) {
                const auto a = ctx.arg(1).value().as_int();
                const auto b = ctx.arg(2).value().as_int();
                if (ctx.push_bool(a != b).bad()) return;
                if (ctx.ret(0).bad()) return;
            },
        .max_locals = 1,
    },
};

yama::module_info make_fns() {
    return
        yama::module_factory()
        .add_type(yama::type_info(observeNone_info))
        .add_type(yama::type_info(observeInt_info))
        .add_type(yama::type_info(observeUInt_info))
        .add_type(yama::type_info(observeFloat_info))
        .add_type(yama::type_info(observeBool_info))
        .add_type(yama::type_info(observeChar_info))
        .add_type(yama::type_info(doPanic_info))
        .add_type(yama::type_info(doIncr_info))
        .add_type(yama::type_info(isEqInt_info))
        .add_type(yama::type_info(isEqChar_info))
        .add_type(yama::type_info(isNotEqInt_info))
        .done();
}


void CompilerImplTests::SetUp() {
    dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    dm = std::make_shared<yama::default_domain>(dbg);
    ctx = std::make_shared<yama::context>(yama::res(dm), dbg);

    comp = GetParam().factory(dbg);

    sidefx = sidefx_t{}; // can't forget to reset this each time

    ready = dm->upload(yama::make_res<yama::module_info>(make_fns()));
}


TEST_P(CompilerImplTests, Empty) {
    std::string txt = R"(

// empty

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(result->empty()); // can't really test w/ dm if *result is empty
}

TEST_P(CompilerImplTests, Fail_LexicalError) {
    std::string txt = R"(

fn f() {
    $$$$$$ // <- lexical error
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    ASSERT_FALSE(comp->compile(yama::res(dm), src));

    // IMPORTANT: I originally tried having seperate lexical/syntactic dsignal values,
    //            but I found that it's actually pretty hard to reliably differentiate
    //            between purely lexical and purely syntactic errors in our LL(1) system

    ASSERT_EQ(dbg->count(yama::dsignal::compile_syntax_error), 1);
}

TEST_P(CompilerImplTests, Fail_SyntaxError) {
    std::string txt = R"(

fn f() {
    fn // <- lexically sound, but syntax error
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    ASSERT_FALSE(comp->compile(yama::res(dm), src));

    ASSERT_EQ(dbg->count(yama::dsignal::compile_syntax_error), 1);
}


// general
//
//      - None default initializes to the stateless None object
//      - Int default initializes to 0
//      - UInt default initializes to 0u
//      - Float default initializes to 0.0
//      - Bool default initializes to false
//      - Char default initializes to '\0'
//
//      - functions default initialize to their sole stateless object
// 
//      - None is non-callable
//      - Int is non-callable
//      - UInt is non-callable
//      - Float is non-callable
//      - Bool is non-callable
//      - Char is non-callable
// 
//      - function are callable (obviously)
//
//      - when a panic occurs, the system is to act as expected (halting
//        program behaviour and unwinding the call stack)

// default init None

TEST_P(CompilerImplTests, General_DefaultInit_None) {
    std::string txt = R"(

fn f() -> None {
    var a: None; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Int

TEST_P(CompilerImplTests, General_DefaultInit_Int) {
    std::string txt = R"(

fn f() -> Int {
    var a: Int; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init UInt

TEST_P(CompilerImplTests, General_DefaultInit_UInt) {
    std::string txt = R"(

fn f() -> UInt {
    var a: UInt; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> UInt");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_uint(0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Float

TEST_P(CompilerImplTests, General_DefaultInit_Float) {
    std::string txt = R"(

fn f() -> Float {
    var a: Float; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(0.0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Bool

TEST_P(CompilerImplTests, General_DefaultInit_Bool) {
    std::string txt = R"(

fn f() -> Bool {
    var a: Bool; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Bool");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_bool(false));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Char

TEST_P(CompilerImplTests, General_DefaultInit_Char) {
    std::string txt = R"(

fn f() -> Char {
    var a: Char; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Char");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_char('\0'));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init function

TEST_P(CompilerImplTests, General_DefaultInit_Fn) {
    std::string txt = R"(

fn g() {}

fn f() -> g {
    var a: g; // default inits a
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// None is non-callable

TEST_P(CompilerImplTests, General_None_IsNonCallable) {
    std::string txt = R"(

fn get_none() {}

fn f() {
    get_none()(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Int is non-callable

TEST_P(CompilerImplTests, General_Int_IsNonCallable) {
    std::string txt = R"(

fn f() {
    300(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// UInt is non-callable

TEST_P(CompilerImplTests, General_UInt_IsNonCallable) {
    std::string txt = R"(

fn f() {
    300u(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Float is non-callable

TEST_P(CompilerImplTests, General_Float_IsNonCallable) {
    std::string txt = R"(

fn f() {
    3.14159(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Bool is non-callable

TEST_P(CompilerImplTests, General_Bool_IsNonCallable) {
    std::string txt = R"(

fn f() {
    true(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Char is non-callable

TEST_P(CompilerImplTests, General_Char_IsNonCallable) {
    std::string txt = R"(

fn f() {
    'a'(); // not callable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// functions are callable

TEST_P(CompilerImplTests, General_Fns_AreCallable) {
    std::string txt = R"(

fn identity_int(x: Int) -> Int {
    return x;
}

fn f() {
    observeInt(identity_int(10));
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto identity_int = dm->load("identity_int"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(identity_int);
    ASSERT_TRUE(f);

    ASSERT_EQ(identity_int->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(identity_int->callsig().value().fmt(), "fn(Int) -> Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// panic behaviour

TEST_P(CompilerImplTests, General_PanicBehaviour) {
    std::string txt = R"(

fn f() {
    observeInt(0);
    doPanic(); // <- panics
    observeInt(1); // <- shouldn't reach
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // <- should panic

    EXPECT_EQ(ctx->panics(), 1);

    // expected return value
    //EXPECT_EQ(ctx->local(0).value(), ctx->new_none()); <- irrelevant due to panic

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(0);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// decl & scope
// 
//      - 'decls' bind name identifiers to types, parameters, and local vars
//      - decls have 'scopes' which define when that binding is valid
//
//      - decl scope is dictated by what 'block' they were declared in,
//        as well as what region within said block the decl is valid for
//      - blocks may be nested within one another, forming hierarchies
//        of blocks w/ parent/child relationships
// 
//      - blocks are either either 'global' or 'local'
//      - global blocks define the top-level global scope inside of which
//        types are defined
//      - local blocks define local scopes within function bodies and
//        the nested scopes within statements
//      - for any given chunk of Yama code, the global block is the root
//        of the block hierarchy
// 
//      - decls in the same block may not share names
//      - decls in child blocks may 'shadow' ones in ancestoral blocks by
//        using the same name as them
// 
//      - within the global block, built-in Yama types (ie. None, Int, UInt,
//        Float, Bool, and Char), and other types defined prior to compilation
//        are declared implicitly
// 
//      - the different scopes decls have are as follows:
// 
//          - decls for types defined prior to compilation exist in the global
//            block, w/ their scope beginning/ending at the start/end of this block
//              * these are in scope at all points in the Yama code
// 
//          - type decls exist in the global block, w/ their scope beginning/ending
//            at the start/end of this block
//              * these are in scope at all points in the Yama code, including prior
//                to the decl itself
//          
//          - parameter decls exist in the local block of their function's body,
//            w/ their scope beginning/ending at the start/end of this block
// 
//          - local var decls exist in the local block they are declared in,
//            w/ their scope beginning immediately after their decl, and ending
//            at the end of their block
//              * this means local var's init expr cannot reference the local var
// 
//      - dead code (ie. code not reachable from entrypoint) is still subject
//        to error checking

// declaring types in global scope will result in new types being
// made available to the domain upon upload

TEST_P(CompilerImplTests, DeclAndScope_DeclaredTypesBecomeAvailableToDomainUponUpload) {
    std::string txt = R"(

fn f() {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// types defined prior to compilation should be available for use
// by Yama code, including the built-in types None, Int, UInt, Float,
// Bool, and Char

TEST_P(CompilerImplTests, DeclAndScope_TypesDefinedPriorToCompilation_MayBeUsedByYamaCode) {
    std::string txt = R"(

fn f() {
    // below tests for primitives and custom types defined prior to
    // the compilation, w/ the observe# fns covering the ladder

    var a: None;
    observeNone(a);
    
    var b: Int;
    observeInt(b);
    
    var c: UInt;
    observeUInt(c);
    
    var d: Float;
    observeFloat(d);
    
    var e: Bool;
    observeBool(e);

    var f: Char;
    observeChar(f);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_none();
    expected.observe_int(0);
    expected.observe_uint(0);
    expected.observe_float(0.0);
    expected.observe_bool(false);
    expected.observe_char(U'\0');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// types defined by Yama code should be available for use by said 
// Yama code

TEST_P(CompilerImplTests, DeclAndScope_TypesDefinedInYamaCode_MayBeUsedByYamaCode) {
    std::string txt = R"(

fn a() {
    observeChar('a');
}

fn b() {
    observeChar('b');
}

fn c() {
    observeChar('c');
}

fn f() {
    a();
    b();
    c();
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto a = dm->load("a"_str);
    const auto b = dm->load("b"_str);
    const auto c = dm->load("c"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(f);

    ASSERT_EQ(a->kind(), yama::kind::function);
    ASSERT_EQ(b->kind(), yama::kind::function);
    ASSERT_EQ(c->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(a->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(b->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(c->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'a');
    expected.observe_char(U'b');
    expected.observe_char(U'c');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// IMPORTANT: this test mainly covers the coexistence between *shadower* and *shadowed*
//            decls, w/ the behaviour of shadowing in regards to referencing behaviour
//            being deferred to tests for 'type specifiers' and 'identifier exprs'

// tolerance of type/parameter/local var decls having common names so
// long as they're in different blocks

TEST_P(CompilerImplTests, DeclAndScope_DeclShadowingBehaviour) {
    std::string txt = R"(

fn a() {} // fn a will be shadowed by 'a's below, w/ this test covering coexistence mainly

fn f(a: Int) {
    observeInt(a);
    if (true) {
        var a = 101; // in fn body
        observeInt(a);
        if (true) {
            var a = 1001; // in nested
            observeInt(a);
        }
        observeInt(a);
    }
    observeInt(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));
    
    const auto a = dm->load("a"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(a);
    ASSERT_TRUE(f);

    ASSERT_EQ(a->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(a->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Int) -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->push_int(21).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(21);
    expected.observe_int(101);
    expected.observe_int(1001);
    expected.observe_int(101);
    expected.observe_int(21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// test to ensure impl properly impls local var scope not starting until after
// the decl itself, w/ this meaning that the init exprs of the local var cannot
// reference itself

TEST_P(CompilerImplTests, DeclAndScope_LocalVarScopeBeginsWhenExpected) {
    std::string txt = R"(

fn f() {
    var a: Int = a; // <- local var 'a' hasn't come into scope yet!
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal to declare a type in global scope with the name of a type
// defined prior to compilation, including the built-in types None,
// Int, UInt, Float, Bool, and Char

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclTypeWithNameOfTypeAlreadyTaken_ByBuiltInType) {
    std::string txt = R"(

fn Int() {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclTypeWithNameOfTypeAlreadyTaken_ByNonBuiltInType) {
    std::string txt = R"(

fn observeInt() {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a two or more types in global scope w/ a common name

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclTypeWithNameOfTypeAlreadyTaken_ByAnotherTypeInYamaCode) {
    std::string txt = R"(

fn f() {}

fn f(a: Int) {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal for a parameter to have the same name as another parameter
// in the same parameter list

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfTwoParamsShareName) {
    std::string txt = R"(

fn f(a, a: Int) {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in function body block w/ the name
// of a parameter

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfParam) {
    std::string txt = R"(

fn f(a: Int) {
    var a = 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in function body block w/ the name
// of another local var in that block

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfAnotherLocalVar_BothInFnBodyBlock) {
    std::string txt = R"(

fn f() {
    var a = 10;
    var a = 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in a nested local block w/ the name
// of another local var in that block

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfAnotherLocalVar_BothInNestedBlock) {
    std::string txt = R"(

fn f() {
    if (true) {
        var a = 10;
        var a = 3.14159;
    }
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// dead code is still subject to error checking

TEST_P(CompilerImplTests, DeclAndScope_Fail_IfDeadCodeIsInError) {
    std::string txt = R"(

fn f() {
    return;
    // dead code
    var a = 10;
    var a = 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}


// type specifier
//
//      - specifies Yama types by name
// 
//      - type specifiers exist relative to some block, and their ability to reference
//        decls respects the scope and shadowing rules of decls

// specify built-in type

TEST_P(CompilerImplTests, TypeSpecifier_SpecifyBuiltInType) {
    std::string txt = R"(

fn f() {
    var a: Int = 10;
    observeInt(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// specify type in Yama code

TEST_P(CompilerImplTests, TypeSpecifier_SpecifyTypeInYamaCode) {
    std::string txt = R"(

fn g() {}

fn f() -> g {
    return g;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g).value());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal for type spec to specify non-type

TEST_P(CompilerImplTests, TypeSpecifier_Fail_IfSpecifyUndeclaredName) {
    std::string txt = R"(

fn f() {
    var a: T; // no type named T
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// cannot reference type which has been shadowed by parameter

TEST_P(CompilerImplTests, TypeSpecifier_Fail_IfTypeShadowedByParam) {
    std::string txt = R"(

fn f(Int: Float) { // <- shadows Int type
    var a: Int; // error, Int refers to param
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_a_type), 1);
}

// cannot reference type which has been shadowed by local var

TEST_P(CompilerImplTests, TypeSpecifier_Fail_IfTypeShadowedByLocalVar) {
    std::string txt = R"(

fn f() {
    var Int = 10; // shadows Int type (ironically)
    var a: Int; // error, Int refers to local var
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_a_type), 1);
}


// variable decl
//
//      - defines non-local var if appears in global block
//          * illegal in MVP version of Yama
// 
//      - defines local var if appears in local block
// 
//      - variable decls w/ just <type-annot> are legal
//          * type specified explicitly by <type-annot>
//          * default inits
//      
//      - variable decls w/ just <init-assign> are legal
//          * type implied by <init-assign> expr type
//          * inits to value of <init-assign> expr
// 
//      - variable decls w/ both <type-annot> and <init-assign> are legal
//          * type specified explicitly by <type-annot>
//          * inits to value of <init-assign> expr
//          * <type-annot> and <init-assign> MUST agree on type
// 
//      - variable decls w/out <type-annot> or <init-assign> are illegal
// 
//      - local vars are initialized upon entering scope
// 
//      - local vars are mutable

// local var w/ just <type-annot>

TEST_P(CompilerImplTests, VariableDecl_TypeAnnot_NoInitAssign) {
    std::string txt = R"(

fn f() {
    var a: Char;
    observeChar(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'\0');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// local var w/ just <init-assign>

TEST_P(CompilerImplTests, VariableDecl_NoTypeAnnot_InitAssign) {
    std::string txt = R"(

fn f() {
    var a = 'y';
    observeChar(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'y');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// local var w/ <type-annot> and <init-assign>

TEST_P(CompilerImplTests, VariableDecl_TypeAnnot_InitAssign) {
    std::string txt = R"(

fn f() {
    var a: Char = 'y';
    observeChar(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'y');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal local var w/out <type-annot> nor <init-assign>

TEST_P(CompilerImplTests, VariableDecl_Fail_If_NoTypeAnnot_NoInitAssign) {
    std::string txt = R"(

fn f() {
    var a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_local_var), 1);
}

// local var initialization and mutability

TEST_P(CompilerImplTests, VariableDecl_InitializationAndMutability) {
    std::string txt = R"(

fn f() {
    var a = 0;
    observeInt(a);
    a = 1;
    observeInt(a);
    a = 2;
    observeInt(a);
    a = 3;
    observeInt(a);
    a = 4;
    observeInt(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(4);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal to declare non-local var

TEST_P(CompilerImplTests, VariableDecl_Fail_IfNonLocalVar) {
    std::string txt = R"(

var a = 10;

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonlocal_var), 1);
}

// illegal to initialize local var w/ wrong typed expr

TEST_P(CompilerImplTests, VariableDecl_Fail_InitExprIsWrongType) {
    std::string txt = R"(

fn f() {
    var a: Int = 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// TODO: below function decl tests don't cover None return type fns w/ 'return x;'
//       return stmt, which 'x' is some None returning expr
//
//       these are covered by our return stmt tests, but function decl tests (which
//       cover the decl itself being able to handle usage of them) isn't covered yet

// function decl
//
//      - defines non-local function if appears in global block
// 
//      - defines local function if appears in local block
//          * illegal in MVP version of Yama
// 
//      - forms w/ explicit return type explicitly specify their return type
//      - forms w/out explicit return type have None as their return type implicitly
// 
//      - w/out return type None, control paths in the function reachable from the
//        entrypoint MUST either end w/ an explicit return stmt, or enter an infinite
//        loop, or otherwise be in error
// 
//      - w/ return type None, control paths in the function reachable from the
//        entrypoint which don't otherwise end w/ a return stmt, nor enter an infinite
//        loop, get ended w/ an implicit 'return;'
// 
//      - in function parameter lists, forms like '(a: Int, b: Float)' are interpreted
//        such that, in this example, parameter 'a' is type 'Int', and 'b' is type 'Float'
//      - in function parameter lists, forms like '(a, b, c: Int)' are interpreted such 
//        that, in this example, parameters 'a', 'b', and 'c' all have type 'Int'
//      - in function parameter lists, forms like '(a, b: Int, c)', in which, in this
//        example, parameter 'c' is never given an explicit type annotation, are illegal
// 
//      - functions may have no more than 24 parameters
// 
//      - function parameter types MUST be object types
//      - function return types MUST be object types
// 
//      - a function body block scope begins at the start of function execution, and
//        ends upon the end of execution
//      - upon start of function execution, the parameters of the function are declared
//        in the function body local block and initialized to the values of the arguments
//        passed to the function from the call site

// function w/ explicit return type, which is NOT None, w/ all control paths exiting
// via return stmt

TEST_P(CompilerImplTests, FunctionDecl_ExplicitReturnType_WhichIsNotNone_AllControlPathsExitViaReturnStmt) {
    std::string txt = R"(

fn f() -> Int {
    // test w/ slightly *nuanced* control flow
    if (true) {
        observeInt(1);
        return 1;
    }
    else {
        observeInt(0);
        return 0;
    }
    observeInt(2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(1));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ explicit return type, which is NOT None, but w/ no return stmt on any
// control paths, instead entering an infinite loop, exiting via panicking

TEST_P(CompilerImplTests, FunctionDecl_ExplicitReturnType_WhichIsNotNone_NoReturnStmtOnAnyControlPath_InsteadEnterInfiniteLoop_ExitViaPanic) {
    std::string txt = R"(

fn f() -> Int { // <- non-None return type, so normally would need explicit return stmt on each control path
    // test w/ slightly *nuanced* control flow
    if (true) {
        loop { // no break stmt means no need for return stmt after loop
            observeInt(1);
            doPanic();
        }
    }
    else {
        loop { // no break stmt means no need for return stmt after loop
            observeInt(0);
            doPanic();
        }
    }
    observeInt(2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    EXPECT_EQ(ctx->panics(), 0);

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).bad()); // <- should panic

    // expected return value
    //EXPECT_EQ(ctx->local(0).value(), ctx->new_int(1)); <- n/a due to panic

    EXPECT_EQ(ctx->panics(), 1);

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ explicit return type, which is None, w/ all control paths exiting
// via return stmt (of form 'return;')

TEST_P(CompilerImplTests, FunctionDecl_ExplicitReturnType_WhichIsNone_AllControlPathsExitViaReturnStmt) {
    std::string txt = R"(

fn f() -> None {
    // test w/ slightly *nuanced* control flow
    if (true) {
        observeInt(1);
        return;
    }
    else {
        observeInt(0);
        return;
    }
    observeInt(2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ explicit return type, which is None, w/ all control paths NOT exiting
// via (explicit) return stmt

TEST_P(CompilerImplTests, FunctionDecl_ExplicitReturnType_WhichIsNone_AllControlPathsExitViaImplicitReturnStmt) {
    std::string txt = R"(

fn f() -> None {
    // test w/ slightly *nuanced* control flow
    if (true) {
        observeInt(1);
    }
    else {
        observeInt(0);
    }
    observeInt(2);
    // implicit 'return;'
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    expected.observe_int(2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/out explicit return type (so has implicit return type None), w/ all control
// paths exiting via return stmt (of form 'return;')

TEST_P(CompilerImplTests, FunctionDecl_ImplicitReturnType_WhichIsNone_AllControlPathsExitViaReturnStmt) {
    std::string txt = R"(

fn f() { // <- implies return type is None
    // test w/ slightly *nuanced* control flow
    if (true) {
        observeInt(1);
        return;
    }
    else {
        observeInt(0);
        return;
    }
    observeInt(2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/out explicit return type (so has implicit return type None), w/ all control
// paths NOT exiting via (explicit) return stmt

TEST_P(CompilerImplTests, FunctionDecl_ImplicitReturnType_WhichIsNone_AllControlPathsExitViaImplicitReturnStmt) {
    std::string txt = R"(

fn f() { // <- implies return type is None
    // test w/ slightly *nuanced* control flow
    if (true) {
        observeInt(1);
    }
    else {
        observeInt(0);
    }
    observeInt(2);
    // implicit 'return;'
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(1);
    expected.observe_int(2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ param list '(a, b, c: Int, d: Float, e: Char)', which tests all the nuances
// which a valid param list needs to be able to handle, and params are actually usable

TEST_P(CompilerImplTests, FunctionDecl_ParamList) {
    std::string txt = R"(

fn f(a, b, c: Int, d: Bool, e: Char) {
    observeInt(a);
    observeInt(b);
    observeInt(c);
    observeBool(d);
    observeChar(e);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Int, Int, Int, Bool, Char) -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->push_int(11).good());
    ASSERT_TRUE(ctx->push_int(909).good());
    ASSERT_TRUE(ctx->push_int(-13).good());
    ASSERT_TRUE(ctx->push_bool(true).good());
    ASSERT_TRUE(ctx->push_char(U'y').good());
    ASSERT_TRUE(ctx->call(6, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(11);
    expected.observe_int(909);
    expected.observe_int(-13);
    expected.observe_bool(true);
    expected.observe_char(U'y');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ 24 parameters, and params are actually usable

TEST_P(CompilerImplTests, FunctionDecl_ParamList_WithMaxParams) {
    std::string txt = R"(

fn f(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23: Int) {
    observeInt(p0);
    observeInt(p1);
    observeInt(p2);
    observeInt(p3);
    observeInt(p4);
    observeInt(p5);
    observeInt(p6);
    observeInt(p7);
    observeInt(p8);
    observeInt(p9);
    observeInt(p10);
    observeInt(p11);
    observeInt(p12);
    observeInt(p13);
    observeInt(p14);
    observeInt(p15);
    observeInt(p16);
    observeInt(p17);
    observeInt(p18);
    observeInt(p19);
    observeInt(p20);
    observeInt(p21);
    observeInt(p22);
    observeInt(p23);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int, Int) -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->push_int(24).good());
    ASSERT_TRUE(ctx->push_int(23).good());
    ASSERT_TRUE(ctx->push_int(22).good());
    ASSERT_TRUE(ctx->push_int(21).good());
    ASSERT_TRUE(ctx->push_int(20).good());
    ASSERT_TRUE(ctx->push_int(19).good());
    ASSERT_TRUE(ctx->push_int(18).good());
    ASSERT_TRUE(ctx->push_int(17).good());
    ASSERT_TRUE(ctx->push_int(16).good());
    ASSERT_TRUE(ctx->push_int(15).good());
    ASSERT_TRUE(ctx->push_int(14).good());
    ASSERT_TRUE(ctx->push_int(13).good());
    ASSERT_TRUE(ctx->push_int(12).good());
    ASSERT_TRUE(ctx->push_int(11).good());
    ASSERT_TRUE(ctx->push_int(10).good());
    ASSERT_TRUE(ctx->push_int(9).good());
    ASSERT_TRUE(ctx->push_int(8).good());
    ASSERT_TRUE(ctx->push_int(7).good());
    ASSERT_TRUE(ctx->push_int(6).good());
    ASSERT_TRUE(ctx->push_int(5).good());
    ASSERT_TRUE(ctx->push_int(4).good());
    ASSERT_TRUE(ctx->push_int(3).good());
    ASSERT_TRUE(ctx->push_int(2).good());
    ASSERT_TRUE(ctx->push_int(1).good());
    ASSERT_TRUE(ctx->call(25, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(24);
    expected.observe_int(23);
    expected.observe_int(22);
    expected.observe_int(21);
    expected.observe_int(20);
    expected.observe_int(19);
    expected.observe_int(18);
    expected.observe_int(17);
    expected.observe_int(16);
    expected.observe_int(15);
    expected.observe_int(14);
    expected.observe_int(13);
    expected.observe_int(12);
    expected.observe_int(11);
    expected.observe_int(10);
    expected.observe_int(9);
    expected.observe_int(8);
    expected.observe_int(7);
    expected.observe_int(6);
    expected.observe_int(5);
    expected.observe_int(4);
    expected.observe_int(3);
    expected.observe_int(2);
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal to declare local fn

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfLocalFn) {
    std::string txt = R"(

fn f() {
    fn g() {}
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_local_fn), 1);
}

// illegal function, w/ non-None return type, w/ a control path from entrypoint which 
// does not end w/ a return stmt

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfNonNoneReturnType_AndControlPathFromEntrypointNotEndingInReturnStmt) {
    std::string txt = R"(

fn f() -> Int {
    // test w/ slightly *nuanced* control flow
    if (true) {
        return 0;
    }
    else if (true) {
        return 1;
    }
    else {
        if (true) {
            // no return stmt (still error even if this code is logically unreachable)
        }
        else {
            return 2;
        }
    }
    // no return stmt here, so above lack means this code is in error
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_no_return_stmt), 1);
}

// illegal function w/ param list '(a, b: Int, c)'

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfInvalidParamList) {
    std::string txt = R"(

fn f(a, b: Int, c) {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_param_list), 1);
}

// illegal function w/ >24 parameters

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfInvalidParamList_MoreThanTwentyFourParams) {
    std::string txt = R"(

fn f(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24: Int) {}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_param_list), 1);
}

// illegal function w/ params of non-object types

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfParamTypesAreNotObjectTypes) {
    // TODO: stub until we add non-object types
}

// illegal function w/ return type of non-object type

TEST_P(CompilerImplTests, FunctionDecl_Fail_IfReturnTypeIsNotObjectType) {
    // TODO: stub until we add non-object types
}


// assignment stmt
// 
//      - performs assignment of the expr value to an assignable expr
//      - the expr value assigned and the assignable expr must agree on type
//
//      - exprs may have a property called 'assignability' which dictates if they can be assigned to
//          * all assignable exprs must have a test covering this
//          * all non-assignable exprs must have a test covering this
//          * each assignable expr must specify what behaviour occurs upon assignment
//          * each assignable expr must specify what type can be assigned to them
//          * the above four will be in the sections of each expr, not here

// basic usage

TEST_P(CompilerImplTests, AssignmentStmt_BasicUsage) {
    std::string txt = R"(

fn f() {
    var a = 10;
    observeInt(a);
    a = 20;
    observeInt(a);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    expected.observe_int(20);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal assignment of non-assignable expr

TEST_P(CompilerImplTests, AssignmentStmt_Fail_IfAssignNonAssignableExpr) {
    std::string txt = R"(

fn f() {
    3 = 10;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// illegal assignment type mismatch

TEST_P(CompilerImplTests, AssignmentStmt_Fail_IfTypeMismatch) {
    std::string txt = R"(

fn f() {
    var a = 10;
    a = 'y';
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// IMPORTANT: strictly speaking, the assignment stmt syntactically operates via an
//            expr stmt, but this aspect will be handled by assignment stmt tests,
//            and not covered here

// expr stmt
//
//      - evaluates the nested expr, discarding its return value

// basic usage

TEST_P(CompilerImplTests, ExprStmt_BasicUsage) {
    std::string txt = R"(

fn f() {
    100; // legal, but totally transparent
    observeInt(10); // legal, and not transparent
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// if stmt
//
//      - evaluates a Bool returning expr, branching according to its value
// 
//      - if true, branch to a nested if-block, execute it, and then branch out of it to code 
//        proceeding the if stmt
//      - if false, and there is no else-block, branch to code proceeding the if stmt
//      - if false, and there is an else-block, branch to it, execute it, and then branch out
//        of it to code proceeding the if stmt
//      - if false, and instead of an else-block there is an if stmt in place of it, perform
//        this if stmt, branching out to code proceeding the if stmt when this nested
//        if stmt would do so
// 
//      - illegal if the expr of the if stmt isn't type Bool

// w/ no else

TEST_P(CompilerImplTests, IfStmt_NoElse) {
    std::string txt = R"(

fn f(a: Bool) {
    observeBool(a);
    if (a) {
        observeInt(-1);
    }
    observeInt(0);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Bool) -> None");

    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_bool(true).good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_bool(true);
        expected.observe_int(-1);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_bool(false).good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_bool(false);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
}

// w/ else

TEST_P(CompilerImplTests, IfStmt_Else) {
    std::string txt = R"(

fn f(a: Bool) {
    observeBool(a);
    if (a) {
        observeInt(-1);
    }
    else {
        observeInt(-2);
    }
    observeInt(0);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Bool) -> None");

    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_bool(true).good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_bool(true);
        expected.observe_int(-1);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_bool(false).good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_bool(false);
        expected.observe_int(-2);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
}

// w/ else-if chain

TEST_P(CompilerImplTests, IfStmt_ElseIfChain) {
    std::string txt = R"(

fn f(a: Char) {
    observeChar(a);
    if (isEqChar(a, 'a')) {
        observeInt(-1);
    }
    else if (isEqChar(a, 'b')) {
        observeInt(-2);
    }
    else if (isEqChar(a, 'c')) {
        observeInt(-3);
    }
    else {
        observeInt(-4);
    }
    observeInt(0);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Char) -> None");

    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_char(U'a').good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_char(U'a');
        expected.observe_int(-1);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_char(U'b').good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_char(U'b');
        expected.observe_int(-2);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_char(U'c').good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_char(U'c');
        expected.observe_int(-3);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_char(U'd').good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_char(U'd');
        expected.observe_int(-4);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
    {
        sidefx = sidefx_t{};

        ASSERT_TRUE(ctx->push_fn(*f).good());
        ASSERT_TRUE(ctx->push_char(U'&').good());
        ASSERT_TRUE(ctx->call(2, yama::newtop).good());

        // expected return value
        EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        sidefx_t expected{};
        expected.observe_char(U'&');
        expected.observe_int(-4);
        expected.observe_int(0);
        EXPECT_EQ(sidefx.fmt(), expected.fmt());
    }
}

// illegal if stmt w/ expr not type Bool

TEST_P(CompilerImplTests, IfStmt_Fail_IfTypeMismatch) {
    std::string txt = R"(

fn f() {
    if ('y') {}
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// loop stmt
//
//      - branches to a nested block, executes it, and then exits this block, restarting it
//        by branching into a new instance of it, and repeating this process, forming an infinite
//        loop, one which continues until exited by either a break or return stmt

// basic usage, exit via break

TEST_P(CompilerImplTests, LoopStmt_BasicUsage_ExitViaBreak) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        observeInt(i);
        if (isEqInt(i, 3)) {
            break; // <- exit loop
        }
        i = doIncr(i);
    }
    observeInt(-2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(-2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// basic usage, exit via return

TEST_P(CompilerImplTests, LoopStmt_BasicUsage_ExitViaReturn) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        observeInt(i);
        if (isEqInt(i, 3)) {
            return; // <- exit loop
        }
        i = doIncr(i);
    }
    observeInt(-2); // will never reach
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// break stmt
//
//      - exits the scope of the nested block of the inner-most nested loop stmt, branching
//        out of any blocks nested therein, transferring control to code immediately following
//        the loop stmt
// 
//      - illegal if there is no inner-most nested loop stmt

// basic usage

TEST_P(CompilerImplTests, BreakStmt_BasicUsage) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        observeInt(i);
        if (isEqInt(i, 3)) {
            break; // <- exit loop
        }
        i = doIncr(i);
    }
    observeInt(-2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(-2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// local var in loop body block, w/ this test existing to ensure impl
// avoids issues we were having w/ it not being able to handle this

TEST_P(CompilerImplTests, BreakStmt_LocalVarInLoopBody) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        var localvar1 = 10;
        observeInt(i);
        if (isEqInt(i, 3)) {
            var localvar2 = 'a'; // put another local var in nested if stmt body
            break; // <- exit loop
        }
        i = doIncr(i);
    }
    observeInt(-2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(-2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal use outside loop stmt

TEST_P(CompilerImplTests, BreakStmt_Fail_IfUsedOutsideLoopStmt) {
    std::string txt = R"(

fn f() {
    break;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_in_loop), 1);
}


// continue stmt
//
//      - exits the scope of the nested block of the inner-most nested loop stmt, branching
//        out of any blocks nested therein, restarting it by transferring control to a new
//        instance of this loop stmt's nested block
// 
//      - illegal if there is no inner-most nested loop stmt

// basic usage

TEST_P(CompilerImplTests, ContinueStmt_BasicUsage) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        observeInt(i);
        if (isNotEqInt(i, 3)) {
            i = doIncr(i);
            continue; // <- avoid break
        }
        break; // <- exit loop
    }
    observeInt(-2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(-2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// local var in loop body block, w/ this test existing to ensure impl
// avoids issues we were having w/ it not being able to handle this

TEST_P(CompilerImplTests, ContinueStmt_LocalVarInLoopBody) {
    std::string txt = R"(

fn f() {
    observeInt(-1);
    var i = 0;
    loop {
        var localvar1 = 10;
        observeInt(i);
        if (isNotEqInt(i, 3)) {
            var localvar2 = 'a'; // put another local var in nested if stmt body
            i = doIncr(i);
            continue; // <- avoid break
        }
        break; // <- exit loop
    }
    observeInt(-2);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-1);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(3);
    expected.observe_int(-2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal use outside loop stmt

TEST_P(CompilerImplTests, ContinueStmt_Fail_IfUsedOutsideLoopStmt) {
    std::string txt = R"(

fn f() {
    continue;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_in_loop), 1);
}


// return stmt
//
//      - returns from the currently executing function invocation, exiting the scope of
//        all local blocks thereof, and transferring control back to the caller, and passing
//        a return value to them specified by the expr of the return stmt
// 
//      - illegal if the return value expr is not the expected return type (ie. type mismatch)

// basic usage, w/ non-None return type

TEST_P(CompilerImplTests, ReturnStmt_BasicUsage_NonNoneReturnType) {
    std::string txt = R"(

fn f() -> Int {
    return 10;
    observeInt(0); // <- shouldn't be reached
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// basic usage, w/ None return type, w/ None return stmt

TEST_P(CompilerImplTests, ReturnStmt_BasicUsage_NoneReturnType_NoneReturnStmt) {
    std::string txt = R"(

fn f() {
    return;
    observeInt(0); // <- shouldn't be reached
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// basic usage, w/ None return type, w/ None return stmt

TEST_P(CompilerImplTests, ReturnStmt_BasicUsage_NoneReturnType_NonNoneReturnStmt) {
    std::string txt = R"(

fn g() {} // <- to get None object (as Yama currently lacks another way to get it)

fn f() {
    return g();
    observeInt(0); // <- shouldn't be reached
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal return stmt due to type mismatch, non-None return stmt

TEST_P(CompilerImplTests, ReturnStmt_Fail_TypeMismatch_NonNoneReturnStmt) {
    std::string txt = R"(

fn f() -> Int {
    return 3.14159;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}

// illegal return stmt due to type mismatch, None return stmt

TEST_P(CompilerImplTests, ReturnStmt_Fail_TypeMismatch_NoneReturnStmt) {
    std::string txt = R"(

fn f() -> Int {
    return;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// identifier expr
// 
//      - specifies a reference to a function, parameter, or local var, for sake of either
//        accessing a value, or (in case of local vars) assigning it
// 
//      - if referring to a function, the identifier expr corresponds to the instantiation
//        of a stateless object of the function type specified
// 
//      - non-assignable if referring to a function
//      - non-assignable if referring to a parameter
//      - assignable if referring to a local var
//          * assigned value is put into the local var
// 
//      - identifier exprs exist relative to some block, and their ability to reference decls
//        respects the scope and shadowing rules of decls
//
//      - illegal if referring to a non-function type

// access value of function reference

TEST_P(CompilerImplTests, IdentifierExpr_AccessValueOf_Fn) {
    std::string txt = R"(

fn f() -> g {
    return g;
}

fn g() {} // make sure impl can handle fn not declared until AFTER first use!

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// access value of parameter reference

TEST_P(CompilerImplTests, IdentifierExpr_AccessValueOf_Param) {
    std::string txt = R"(

fn f(a: Int) -> Int {
    observeInt(a);
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Int) -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->push_int(-21).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-21));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// access value of local var reference

TEST_P(CompilerImplTests, IdentifierExpr_AccessValueOf_LocalVar) {
    std::string txt = R"(

fn f() -> Int {
    var a = -21;
    observeInt(a);
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-21));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function shadowed by parameter

TEST_P(CompilerImplTests, IdentifierExpr_ParamShadowsFn) {
    std::string txt = R"(

fn g() {}

fn f(g: Int) {
    observeInt(g); // <- param g shadows fn g
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn(Int) -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->push_int(-21).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function shadowed by local var

TEST_P(CompilerImplTests, IdentifierExpr_LocalVarShadowsFn) {
    std::string txt = R"(

fn g() {}

fn f() {
    var g = -21;
    observeInt(g); // <- local var g shadows fn g
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// local var shadowed by another local var

TEST_P(CompilerImplTests, IdentifierExpr_LocalVarShadowsAnotherLocalVar) {
    std::string txt = R"(

fn f() {
    var a = 101;
    observeInt(a); // <- not shadowed here
    if (true) {
        var a = -21;
        observeInt(a); // <- local var a shadows other local var a
    }
    observeInt(a); // <- not shadowed here
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(101);
    expected.observe_int(-21);
    expected.observe_int(101);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal reference to non-function type

TEST_P(CompilerImplTests, IdentifierExpr_Fail_IfRefersToNonFnType) {
    std::string txt = R"(

fn f() {
    var a = Int; // 'Int' is not a fn type
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_an_expr), 1);
}

// illegal reference to undeclared name

TEST_P(CompilerImplTests, IdentifierExpr_Fail_IfRefersToUndeclaredName) {
    std::string txt = R"(

fn f() {
    var a = abc; // no decl named 'abc'
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal reference to local var not yet in scope (ie. the identifier expr is
// in the same block as the local var, but the local var isn't in scope yet)

TEST_P(CompilerImplTests, IdentifierExpr_Fail_IfRefersToLocalVarNotYetInScope) {
    std::string txt = R"(

fn f() {
    var a = abc; // 'abc' isn't in scope yet
    var abc = 10;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal reference to local var which has gone out-of-scope

TEST_P(CompilerImplTests, IdentifierExpr_Fail_IfRefersToLocalVarWhichIsOutOfScope) {
    std::string txt = R"(

fn f() {
    if (true) {
        var abc = 10;
    }
    var a = abc; // 'abc' isn't in scope anymore
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// identifier expr is non-assignable, if function reference

TEST_P(CompilerImplTests, IdentifierExpr_NonAssignable_IfFn) {
    std::string txt = R"(

fn g() {}

fn f() {
    // below uses '10', as we can't really give the assigned value a
    // *correct type*, as that would imply fn decl 'g' has a notion
    // of itself having an object type, which it doesn't really...

    g = 10; // 'g' is not assignable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// identifier expr is non-assignable, if parameter reference

TEST_P(CompilerImplTests, IdentifierExpr_NonAssignable_IfParam) {
    std::string txt = R"(

fn f(a: Int) {
    a = 10; // 'a' is not assignable
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// identifier expr is assignable, if local var reference

TEST_P(CompilerImplTests, IdentifierExpr_Assignable_IfLocalVar) {
    std::string txt = R"(

fn f() -> Int {
    var a = -21;
    a = 10; // overwrite previous value
    observeInt(a);
    return a;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// Int literal expr
//
//      - returns a Int value specified by a literal
// 
//      - non-assignable
// 
//      - illegal if Int literal overflows
//      - illegal if Int literal underflows

// basic usage

TEST_P(CompilerImplTests, IntLiteralExpr_BasicUsage) {
    std::string txt = R"(

fn f() {
    observeInt(0);
    observeInt(1);
    observeInt(10);
    observeInt(10_000);
    observeInt(10_000_000);
    observeInt(987_654_321);
    observeInt(0xab1_e03);
    observeInt(0b110_101);
    observeInt(-0);
    observeInt(-1);
    observeInt(-10);
    observeInt(-10_000);
    observeInt(-10_000_000);
    observeInt(-987_654_321);
    observeInt(-0xab1_e03);
    observeInt(-0b110_101);
    observeInt(9_223_372_036_854_775_807); // max Int value
    observeInt(000_009_223_372_036_854_775_807); // leading 0s
    observeInt(-9_223_372_036_854_775_808); // min Int value
    observeInt(-000_009_223_372_036_854_775_808); // leading 0s
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(10);
    expected.observe_int(10'000);
    expected.observe_int(10'000'000);
    expected.observe_int(987'654'321);
    expected.observe_int(0xab1e03);
    expected.observe_int(0b110101);
    expected.observe_int(-0);
    expected.observe_int(-1);
    expected.observe_int(-10);
    expected.observe_int(-10'000);
    expected.observe_int(-10'000'000);
    expected.observe_int(-987'654'321);
    expected.observe_int(-0xab1e03);
    expected.observe_int(-0b110101);
    expected.observe_int(9'223'372'036'854'775'807);
    expected.observe_int(9'223'372'036'854'775'807);
    expected.observe_int(-9'223'372'036'854'775'808i64);
    expected.observe_int(-9'223'372'036'854'775'808i64);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal if Int literal overflows

TEST_P(CompilerImplTests, IntLiteralExpr_Fail_IfNumericOverflow) {
    std::string txt = R"(

fn f() -> Int {
    return 9_223_372_036_854_775_808;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_overflow), 1);
}

// illegal if Int literal underflows

TEST_P(CompilerImplTests, IntLiteralExpr_Fail_IfNumericUnderflow) {
    std::string txt = R"(

fn f() -> Int {
    return -9_223_372_036_854_775_809;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_underflow), 1);
}

// Int literal is non-assignable

TEST_P(CompilerImplTests, IntLiteralExpr_NonAssignable) {
    std::string txt = R"(

fn f() {
    1 = 2;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}


// UInt literal expr
//
//      - returns a UInt value specified by a literal
// 
//      - non-assignable
// 
//      - illegal if UInt literal overflows

// basic usage

TEST_P(CompilerImplTests, UIntLiteralExpr_BasicUsage) {
    std::string txt = R"(

fn f() {
    observeUInt(0u);
    observeUInt(1u);
    observeUInt(10u);
    observeUInt(10_000u);
    observeUInt(10_000_000u);
    observeUInt(987_654_321u);
    observeUInt(0xab1_e03u);
    observeUInt(0b110_101u);
    observeUInt(18_446_744_073_709_551_615u); // max UInt value
    observeUInt(000_018_446_744_073_709_551_615u); // leading 0s
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_uint(0);
    expected.observe_uint(1);
    expected.observe_uint(10);
    expected.observe_uint(10'000);
    expected.observe_uint(10'000'000);
    expected.observe_uint(987'654'321);
    expected.observe_uint(0xab1e03);
    expected.observe_uint(0b110101);
    expected.observe_uint(18'446'744'073'709'551'615);
    expected.observe_uint(18'446'744'073'709'551'615);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal if UInt literal overflows

TEST_P(CompilerImplTests, UIntLiteralExpr_Fail_IfNumericOverflow) {
    std::string txt = R"(

fn f() -> Int {
    return 18_446_744_073_709_551_616u;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_overflow), 1);
}

// UInt literal is non-assignable

TEST_P(CompilerImplTests, UIntLiteralExpr_NonAssignable) {
    std::string txt = R"(

fn f() {
    1u = 2u;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}


// Float literal expr
//
//      - returns a Float value specified by a literal, including inf/nan keywords
// 
//      - non-assignable
// 
//      - out-of-bounds Float literals resolve to inf/-inf values

// basic usage

TEST_P(CompilerImplTests, FloatLiteralExpr_BasicUsage) {
    std::string txt = R"(

fn f0() -> Float { return 0.0; }
fn f1() -> Float { return 1.0; }
fn f2() -> Float { return 1_000_000.0; }
fn f3() -> Float { return .1324; }
fn f4() -> Float { return .1324e2; } // 13.24
fn f5() -> Float { return inf; }
fn f6() -> Float { return nan; }

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f0 = dm->load("f0"_str);
    const auto f1 = dm->load("f1"_str);
    const auto f2 = dm->load("f2"_str);
    const auto f3 = dm->load("f3"_str);
    const auto f4 = dm->load("f4"_str);
    const auto f5 = dm->load("f5"_str);
    const auto f6 = dm->load("f6"_str);
    ASSERT_TRUE(f0);
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    ASSERT_TRUE(f3);
    ASSERT_TRUE(f4);
    ASSERT_TRUE(f5);
    ASSERT_TRUE(f6);

    ASSERT_EQ(f0->kind(), yama::kind::function);
    ASSERT_EQ(f1->kind(), yama::kind::function);
    ASSERT_EQ(f2->kind(), yama::kind::function);
    ASSERT_EQ(f3->kind(), yama::kind::function);
    ASSERT_EQ(f4->kind(), yama::kind::function);
    ASSERT_EQ(f5->kind(), yama::kind::function);
    ASSERT_EQ(f6->kind(), yama::kind::function);
    ASSERT_EQ(f0->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f1->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f2->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f3->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f4->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f5->callsig().value().fmt(), "fn() -> Float");
    ASSERT_EQ(f6->callsig().value().fmt(), "fn() -> Float");

    // f6 is handled seperately, due to NAN values being annoying

    std::initializer_list<std::pair<yama::type, yama::float_t>> cases{
        { *f0, 0.0 },
        { *f1, 1.0 },
        { *f2, 1'000'000.0 },
        { *f3, 0.1324 },
        { *f4, 0.1324e2 },
        { *f5, yama::inf },
    };

    for (const auto& I : cases) {
        const auto& f = I.first;
        const yama::float_t v = I.second;

        ASSERT_TRUE(ctx->push_fn(f).good());
        ASSERT_TRUE(ctx->call(1, yama::newtop).good());

        // expected return value
        const auto lhs = ctx->local(0).value();
        const auto rhs = ctx->new_float(v);
        EXPECT_TRUE(lhs.t == ctx->load_float());
        if (lhs.t == ctx->load_float()) EXPECT_DOUBLE_EQ(lhs.as_float(), rhs.as_float());

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        EXPECT_EQ(sidefx.fmt(), sidefx_t{}.fmt());
    }

    {
        const auto& f = *f6;

        ASSERT_TRUE(ctx->push_fn(f).good());
        ASSERT_TRUE(ctx->call(1, yama::newtop).good());

        // expected return value
        const auto lhs = ctx->local(0).value();
        EXPECT_TRUE(lhs.t == dm->load_float());
        if (lhs.t == dm->load_float()) EXPECT_TRUE(std::isnan(lhs.as_float()));

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        EXPECT_EQ(sidefx.fmt(), sidefx_t{}.fmt());
    }
}

// inf if overflows

TEST_P(CompilerImplTests, FloatLiteralExpr_InfIfOverflows) {
    std::string txt = R"(

fn f() -> Float { return 1.7976931348723158e+308; } // should overflow to inf

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    const auto lhs = ctx->local(0).value();
    EXPECT_TRUE(lhs.t == dm->load_float());
    if (lhs.t == dm->load_float()) {
        EXPECT_TRUE(std::isinf(lhs.as_float()));
        EXPECT_FALSE(std::signbit(lhs.as_float()));
    }

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// -inf if underflows

TEST_P(CompilerImplTests, FloatLiteralExpr_NegativeInfIfUnderflows) {
    std::string txt = R"(

fn f() -> Float { return -1.7976931348723158e+308; } // should underflow to -inf

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    const auto lhs = ctx->local(0).value();
    EXPECT_TRUE(lhs.t == dm->load_float());
    if (lhs.t == dm->load_float()) {
        EXPECT_TRUE(std::isinf(lhs.as_float()));
        EXPECT_TRUE(std::signbit(lhs.as_float()));
    }

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// Float literal is non-assignable

TEST_P(CompilerImplTests, FloatLiteralExpr_NonAssignable) {
    std::string txt = R"(

fn f() {
    1.0 = 2.0;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}


// Bool literal expr
//
//      - returns a Bool value specified by a literal
// 
//      - non-assignable

// basic usage

TEST_P(CompilerImplTests, BoolLiteralExpr_BasicUsage) {
    std::string txt = R"(

fn f() {
    observeBool(true);
    observeBool(false);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_bool(true);
    expected.observe_bool(false);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// Bool literal is non-assignable

TEST_P(CompilerImplTests, BoolLiteralExpr_NonAssignable) {
    std::string txt = R"(

fn f() {
    true = false;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}


// Char literal expr
//
//      - returns a Char value specified by a literal
// 
//      - non-assignable

// basic usage

TEST_P(CompilerImplTests, CharLiteralExpr_BasicUsage) {
    // IMPORTANT: for Unicode input to work, we gotta use a UTF-8 string literal
    std::u8string txt = u8R"(

fn f() {
    observeChar('a');
    observeChar('b');
    observeChar('c');
    observeChar('1');
    observeChar('2');
    observeChar('3');
    observeChar(' ');
    observeChar('&');
    observeChar('^');
    observeChar(')');
    observeChar('$');

    // escape seqs
    observeChar('\0');
    observeChar('\a');
    observeChar('\b');
    observeChar('\f');
    observeChar('\n');
    observeChar('\r');
    observeChar('\t');
    observeChar('\v');
    observeChar('\'');
    observeChar('\"');
    observeChar('\\');
    observeChar('\x1e');
    observeChar('\u1ef4');
    observeChar('\U0001f4a9');

    // Unicode
    observeChar('Δ');
    observeChar('Γ');
    observeChar('魂');
    observeChar('💩');
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(taul::utf8_s(txt)));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    //std::cerr << std::format("{}\n", result->back());

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto f = dm->load("f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'a');
    expected.observe_char(U'b');
    expected.observe_char(U'c');
    expected.observe_char(U'1');
    expected.observe_char(U'2');
    expected.observe_char(U'3');
    expected.observe_char(U' ');
    expected.observe_char(U'&');
    expected.observe_char(U'^');
    expected.observe_char(U')');
    expected.observe_char(U'$');
    expected.observe_char(U'\0');
    expected.observe_char(U'\a');
    expected.observe_char(U'\b');
    expected.observe_char(U'\f');
    expected.observe_char(U'\n');
    expected.observe_char(U'\r');
    expected.observe_char(U'\t');
    expected.observe_char(U'\v');
    expected.observe_char(U'\'');
    expected.observe_char(U'\"');
    expected.observe_char(U'\\');
    expected.observe_char(U'\x1e');
    expected.observe_char(U'\u1ef4');
    expected.observe_char(U'\U0001f4a9');
    expected.observe_char(U'Δ');
    expected.observe_char(U'Γ');
    expected.observe_char(U'魂');
    expected.observe_char(U'💩');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal Unicode, UTF-16 surrogate

TEST_P(CompilerImplTests, CharLiteralExpr_Fail_IllegalUnicode_UTF16Surrogate) {
    std::string txt = R"(

fn f() {
    var a = '\ud8a2';
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_unicode), 1);
}

// illegal Unicode, out-of-bounds

TEST_P(CompilerImplTests, CharLiteralExpr_Fail_IllegalUnicode_OutOfBounds) {
    std::string txt = R"(

fn f() {
    var a = '\U00110000';
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_unicode), 1);
}

// Char literal is non-assignable

TEST_P(CompilerImplTests, CharLiteralExpr_NonAssignable) {
    std::string txt = R"(

fn f() {
    'a' = 'b';
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}


// call expr
//
//      - given some form 'f(a0, a1, ..., an)', where f is the call object expr, and a0, a1, 
//        and other args (ie. the '..., an' part), the call expr describes a call site, first
//        evaluating the f expr to acquire the call object, then evaluating the arg exprs,
//        going from left-to-right, then performing the call, returning its return value
// 
//      - if a call expr returns an object of a callable type, it may be nested within another
//        call expr
// 
//      - non-assignable
// 
//      - illegal if the call object expr specifies a call object of a non-callable type
// 
//      - illegal if the args passed to the call are the wrong number/types for a call to
//        the specified call object

// basic usage

TEST_P(CompilerImplTests, CallExpr_BasicUsage) {
    std::string txt = R"(

fn choose(which: Bool, a, b: Int) -> Int {
    if (which) {
        return a;
    }
    else {
        return b;
    }
}

fn f() {
    observeInt(choose(true, -3, 12));
    observeInt(choose(false, -3, 12));
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto choose = dm->load("choose"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(choose);
    ASSERT_TRUE(f);

    ASSERT_EQ(choose->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(choose->callsig().value().fmt(), "fn(Bool, Int, Int) -> Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-3);
    expected.observe_int(12);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// callobj/params evaluation order

TEST_P(CompilerImplTests, CallExpr_EvalOrder) {
    std::string txt = R"(

fn get_callobj() -> g { observeChar('c'); return g; }

fn foo() -> Int { observeInt(0); return 0; }
fn bar() -> Int { observeInt(1); return 0; }
fn baz() -> Int { observeInt(2); return 0; }

fn g(a, b, c: Int) -> Int { return 0; }

fn f() {
    get_callobj()(foo(), g(foo(), bar(), baz()), bar());
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto get_callobj = dm->load("get_callobj"_str);
    const auto foo = dm->load("foo"_str);
    const auto bar = dm->load("bar"_str);
    const auto baz = dm->load("baz"_str);
    const auto g = dm->load("g"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(get_callobj);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(bar);
    ASSERT_TRUE(baz);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(get_callobj->kind(), yama::kind::function);
    ASSERT_EQ(foo->kind(), yama::kind::function);
    ASSERT_EQ(bar->kind(), yama::kind::function);
    ASSERT_EQ(baz->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(get_callobj->callsig().value().fmt(), "fn() -> g");
    ASSERT_EQ(foo->callsig().value().fmt(), "fn() -> Int");
    ASSERT_EQ(bar->callsig().value().fmt(), "fn() -> Int");
    ASSERT_EQ(baz->callsig().value().fmt(), "fn() -> Int");
    ASSERT_EQ(g->callsig().value().fmt(), "fn(Int, Int, Int) -> Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'c');
    expected.observe_int(0);
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    expected.observe_int(1);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// call expr nesting

TEST_P(CompilerImplTests, CallExpr_Nesting) {
    std::string txt = R"(

fn foo(x: Int) -> foo { observeInt(x); return foo; }

fn f() {
    foo(10)(20)(30)(40)(50)(60);
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    const auto result = comp->compile(yama::res(dm), src);
    ASSERT_TRUE(result);

    ASSERT_TRUE(dm->upload(yama::res(result)));

    const auto foo = dm->load("foo"_str);
    const auto f = dm->load("f"_str);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(f);

    ASSERT_EQ(foo->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(foo->callsig().value().fmt(), "fn(Int) -> foo");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    expected.observe_int(20);
    expected.observe_int(30);
    expected.observe_int(40);
    expected.observe_int(50);
    expected.observe_int(60);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal call due to call object is non-callable type

TEST_P(CompilerImplTests, CallExpr_Fail_CallObjIsNonCallableType) {
    std::string txt = R"(

fn f() {
    300(); // <- Int is non-callable!
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// illegal call due to too many args

TEST_P(CompilerImplTests, CallExpr_Fail_Args_TooMany) {
    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g('a', 'b');
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to too few args

TEST_P(CompilerImplTests, CallExpr_Fail_Args_TooFew) {
    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g();
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to incorrect arg type(s) (ie. type mismatch)

TEST_P(CompilerImplTests, CallExpr_Fail_Args_TypeMismatch) {
    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g('a');
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}

// call expr is non-assignable

TEST_P(CompilerImplTests, CallExpr_NonAssignable) {
    std::string txt = R"(

fn g(a, b: Int) -> Int { return 0; }

fn f() {
    g(3, 2) = 4;
}

)";

    taul::source_code src{};
    src.add_str("src"_str, yama::str(txt));

    EXPECT_FALSE(comp->compile(yama::res(dm), src));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

