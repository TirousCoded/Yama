

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/res.h>
#include <yama/core/scalars.h>
#include <yama/core/domain.h>
#include <yama/core/context.h>
#include <yama/core/callsig_info.h>
#include <yama/core/const_table_info.h>


using namespace yama::string_literals;


class CompilationTests : public testing::Test {
public:
    std::shared_ptr<yama::dsignal_debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<yama::context> ctx;

    bool ready = false; // if fixture setup correctly


    std::optional<yama::module> perform_compile(
        const std::string& txt,
        const std::string& txt_multi_a = "",
        const std::string& txt_multi_b = "",
        const std::string& txt_multi_c = "",
        const std::string& txt_multi_d = "");


protected:
    void SetUp() override final;

    void TearDown() override final {
        //
    }
};


// IMPORTANT:
//      herein we use a vary specific format when writing our unit tests, in order to
//      ensure consistency and comprehensiveness:
//          - we split up our tests into sections, each of which starts w/ a large
//            'header' comment block at the top, which names the section, and gives a
//            description of the semantics expected of the impl which the tests of the
//            section are checking for
// 
//          - each test in a section has a little preamble comment above it which describes
//            the behaviour covered by the test, followed by the unit test itself
//              * these preamble comments, and their corresponding unit tests, need not be 1-to-1
//                w/ the exact wording of the header, just so long as the overall section's tests
//                are properly comprehensive
// 
//              * a bit redundant, but I like how these preamble comments make our unit tests
//                more readable, rather than us having to stuff everything in the cramped name
//                identifier of the unit test
//
//              * I like writing our the preamble comments and header comment block fully before
//                even starting to write the unit tests themselves


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
//      limits on things like the max values for allowed local vars, temporaries, branch
//      jump distances, etc.


// IMPORTANT:
//      in order to unit test a way that ensures we don't couple our tests to impl details,
//      and is clean and comprehensive, we're gonna assert the following:
// 
//          1) if the code compiles + survives static verif
//              * compilation is performed by importing from a special module which
//                in turn pulls on the compiler impl which we inject into the domain
// 
//          2) if once imported into domain the types of the compiled module then
//             become available
//              * we're NOT gonna be asserting anything in regards to the module
//                returned by compile, as I don't want our tests to be coupled to
//                this info
// 
//          3) if executing compiled functions produces the expected return value
// 
//          4) if executing compiled functions produces the expected sequence
//             of observable side effects
//
//      the above are in regards to tests of successful compilation, w/ failure
//      tests simply asserting failed compilation and expected debug signals


// IMPORTANT:
//      the term 'extern type' refers to types which have been loaded from modules
//      imported into the compilation


// IMPORTANT:
//      Yama lang exprs are 'constant exprs' or 'constexprs' if and only if:
//          1) the expr's operation is constexpr eligible (ie. it's a pure fn)
//          2) oprand exprs parameterizing them, if any, are also constexprs
//
//      the Yama lang impl is free to precompute constexprs


// TODO: maybe mention in docs that Yama lvalues/rvalues differ quite a bit from C++ ones

// IMPORTANT:
//      Yama lang exprs each have a property called 'expr mode' which dictates the semantic
//      role of the expr
//
//      the following expr modes exist:
//
//          lvalue      an expr which exists to be assigned a value
//                          * expr must be assignable (see below)
//          rvalue      an expr which exists to compute a value
//          crvalue     a constexpr rvalue
//                          * expr must be a constexpr
// 
//      the behaviour of an expr is dictated by if it's a lvalue or rvalue:
//
//          - rvalue exprs have a 'compute' behaviour, which computes a value
//              * herein, when describing what an expr *does*, the description of its
//                behaviour upon evaluation is implicitly talking about its compute
//                behaviour
//
//          - lvalue exprs have an 'assignment' behaviour, which accepts a value and
//            (presumably) uses it to assign something
//              * whether an expr has a defined assignment behaviour dictates whether
//                it is 'assignable' or not


// IMPORTANT:
//      Yama lang 'homomorphic exprs' are exprs which, given a set of other exprs
//      which all share the same syntactic structure, exist to disambiguate these
//      exprs, defining the 'selection rules' governing which ones should be selected
//      in a given circumstance
//
//      once selected, the expr *ceases to be* the homomorphic expr, and it instead
//      will identify thereafter as the selected expr
//
//      the exprs covered by a homomorphic expr, in their unit tests, delegate semantics
//      relating to selection to their associated homomorphic expr


// IMPORTANT:
//      all exprs must have unit tests for the following:
//          - the compute behaviour of the expr
//          - if it is/isn't assignable
//              * must cover assignment behaviour
//              * multiple if conditionally assignable
//          - if it is/isn't constexpr
//              * multiple if conditionally constexpr


namespace {
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
        void observe_type(yama::type x) { seq += std::format("type {}\n", x); }

        void observe_misc(std::string_view msg) { seq += std::format("misc {}\n", msg); }
    };

    sidefx_t sidefx; // global so our Yama functions can reference it


    // our Yama functions for side-effects + misc

    void observeNone_fn(yama::context& ctx) {
        sidefx.observe_none();
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeInt_fn(yama::context& ctx) {
        sidefx.observe_int(ctx.arg(1).value().as_int());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeUInt_fn(yama::context& ctx) {
        sidefx.observe_uint(ctx.arg(1).value().as_uint());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeFloat_fn(yama::context& ctx) {
        sidefx.observe_float(ctx.arg(1).value().as_float());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeBool_fn(yama::context& ctx) {
        sidefx.observe_bool(ctx.arg(1).value().as_bool());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeChar_fn(yama::context& ctx) {
        sidefx.observe_char(ctx.arg(1).value().as_char());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void observeType_fn(yama::context& ctx) {
        sidefx.observe_type(ctx.arg(1).value().as_type());
        if (ctx.push_none().bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void doPanic_fn(yama::context& ctx) {
        ctx.panic();
    }
    void doIncr_fn(yama::context& ctx) {
        if (ctx.push_int(ctx.arg(1).value().as_int() + 1).bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void isEqInt_fn(yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_int();
        const auto b = ctx.arg(2).value().as_int();
        if (ctx.push_bool(a == b).bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void isEqChar_fn(yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_char();
        const auto b = ctx.arg(2).value().as_char();
        if (ctx.push_bool(a == b).bad()) return;
        if (ctx.ret(0).bad()) return;
    }
    void isNotEqInt_fn(yama::context& ctx) {
        const auto a = ctx.arg(1).value().as_int();
        const auto b = ctx.arg(2).value().as_int();
        if (ctx.push_bool(a != b).bad()) return;
        if (ctx.ret(0).bad()) return;
    }

    const auto consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Char"_str)
        .add_primitive_type("yama:Type"_str);

    const auto fns_module =
        yama::module_factory()
        .add_function("observeNone"_str, consts, yama::make_callsig({ 0 }, 0), 1, observeNone_fn)
        .add_function("observeInt"_str, consts, yama::make_callsig({ 1 }, 0), 1, observeInt_fn)
        .add_function("observeUInt"_str, consts, yama::make_callsig({ 2 }, 0), 1, observeUInt_fn)
        .add_function("observeFloat"_str, consts, yama::make_callsig({ 3 }, 0), 1, observeFloat_fn)
        .add_function("observeBool"_str, consts, yama::make_callsig({ 4 }, 0), 1, observeBool_fn)
        .add_function("observeChar"_str, consts, yama::make_callsig({ 5 }, 0), 1, observeChar_fn)
        .add_function("observeType"_str, consts, yama::make_callsig({ 6 }, 0), 1, observeType_fn)
        .add_function("doPanic"_str, consts, yama::make_callsig({}, 0), 0, doPanic_fn)
        .add_function("doIncr"_str, consts, yama::make_callsig({ 1 }, 1), 1, doIncr_fn)
        .add_function("isEqInt"_str, consts, yama::make_callsig({ 1, 1 }, 4), 1, isEqInt_fn)
        .add_function("isEqChar"_str, consts, yama::make_callsig({ 5, 5 }, 4), 1, isEqChar_fn)
        .add_function("isNotEqInt"_str, consts, yama::make_callsig({ 1, 1 }, 4), 1, isNotEqInt_fn)
        // need this to better test w/ structs, so we'll just quick-n'-dirty bundle this
        // w/ our helper test fns
        .add_struct("SomeStruct"_str, yama::const_table_info{})
        .done();

    class fns_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::shared_ptr<const yama::module_info> mi;


        fns_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (relative_path == ".abc"_str) {
                return yama::make_res<yama::module_info>(fns_module);
            }
            // .bad is used to test failure
            else if (relative_path == ".bad"_str) {
                yama::module_factory mf{};
                mf.add_function(
                    "f"_str,
                    yama::const_table_info{},
                    yama::make_callsig({}, 10'000), // <- return type invalid! so .bad is also invalid!
                    1,
                    yama::noop_call_fn);
                return mf.done();
            }
            else return std::nullopt;
        }
    };


    const auto acknowledge_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);

    const auto acknowledge_info = yama::make_function(
        "acknowledge"_str,
        acknowledge_consts,
        yama::make_callsig({}, 0),
        1,
        [](yama::context& ctx) {
            sidefx.observe_misc("ack");
            if (ctx.push_none().bad()) return;
            if (ctx.ret(0).bad()) return;
        });

    // test_parcel exists to establish the environment inside of which compilation is going
    // to be occurring (ie. defined via self-name and dep-names of the parcel)

    // test_parcel is also used to invoke the compiler

    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::string our_src;
        std::string our_src_multi_a;
        std::string our_src_multi_b;
        std::string our_src_multi_c;


        test_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str, "fns"_str, "fns2"_str, "other"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (relative_path == ""_str) {
                // return source code, invoking compiler
                taul::source_code src{};
                src.add_str("src"_str, yama::str(our_src));
                return yama::import_result(std::move(src));
            }
            else if (relative_path == ".multi.a") {
                // return source code, inducing multi-source compilation
                taul::source_code src{};
                src.add_str("src2"_str, yama::str(our_src_multi_a));
                return yama::import_result(std::move(src));
            }
            else if (relative_path == ".multi.b") {
                // return source code, inducing multi-source compilation
                taul::source_code src{};
                src.add_str("src3"_str, yama::str(our_src_multi_b));
                return yama::import_result(std::move(src));
            }
            else if (relative_path == ".multi.c") {
                // return source code, inducing multi-source compilation
                taul::source_code src{};
                src.add_str("src4"_str, yama::str(our_src_multi_c));
                return yama::import_result(std::move(src));
            }
            // .abc exists to test importing a module which exists in same parcel as compiling module
            else if (relative_path == ".abc"_str) {
                yama::module_factory mf{};
                mf.add(yama::type_info(acknowledge_info));
                return mf.done();
            }
            else return std::nullopt;
        }
    };


    // other_test_parcel exists to let us test multi-parcel multi-source compilation

    class other_test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::string our_src_multi_d;


        other_test_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str, "fns"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (relative_path == ".multi.d") {
                // return source code, inducing multi-parcel multi-source compilation
                taul::source_code src{};
                src.add_str("src5"_str, yama::str(our_src_multi_d));
                return yama::import_result(std::move(src));
            }
            else return std::nullopt;
        }
    };



    // this parcel is equiv to test_parcel, existing for a specific test case where the parcel itself
    // is invalid, causing compilation to fail

    // the failure will be due to this parcel lacking a 'yama' dep

    class test_invalid_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::string our_src;


        test_invalid_parcel() = default;


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, {} };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (relative_path != ""_str) return std::nullopt;
            // return source code, invoking compiler
            taul::source_code src{};
            src.add_str("src"_str, yama::str(our_src));
            return yama::import_result(std::move(src));
        }
    };


    // quick-n'-dirty global var test_parcel for us to be able to rebind our_src of for each test

    yama::res<fns_parcel> our_fns_parcel = yama::make_res<fns_parcel>();
    yama::res<fns_parcel> our_fns2_parcel = yama::make_res<fns_parcel>();
    yama::res<test_parcel> our_test_parcel = yama::make_res<test_parcel>();
    yama::res<other_test_parcel> our_other_test_parcel = yama::make_res<other_test_parcel>();
}


std::optional<yama::module> CompilationTests::perform_compile(
    const std::string& txt,
    const std::string& txt_multi_a,
    const std::string& txt_multi_b,
    const std::string& txt_multi_c,
    const std::string& txt_multi_d) {
    our_test_parcel->our_src = txt;
    our_test_parcel->our_src_multi_a = txt_multi_a;
    our_test_parcel->our_src_multi_b = txt_multi_b;
    our_test_parcel->our_src_multi_c = txt_multi_c;
    our_other_test_parcel->our_src_multi_d = txt_multi_d;
    return dm->import("a"_str);
}

void CompilationTests::SetUp() {
    dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
    dm = std::make_shared<yama::domain>(dbg);
    ctx = std::make_shared<yama::context>(yama::res(dm), dbg);

    sidefx = sidefx_t{}; // can't forget to reset this each time

    yama::install_batch ib{};
    ib
        .install("fns"_str, our_fns_parcel)
        .map_dep("fns"_str, "yama"_str, "yama"_str)
        .install("fns2"_str, our_fns2_parcel)
        .map_dep("fns2"_str, "yama"_str, "yama"_str)
        .install("a"_str, our_test_parcel)
        .map_dep("a"_str, "yama"_str, "yama"_str)
        .map_dep("a"_str, "fns"_str, "fns"_str)
        .map_dep("a"_str, "fns2"_str, "fns2"_str)
        .map_dep("a"_str, "other"_str, "other"_str)
        .install("other"_str, our_other_test_parcel)
        .map_dep("other"_str, "yama"_str, "yama"_str)
        .map_dep("other"_str, "fns"_str, "fns"_str);

    ready = dm->install(std::move(ib));
}


TEST_F(CompilationTests, Empty) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

// empty

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    ASSERT_EQ(result->size(), 0); // can't really test w/ dm if *result is empty
}

TEST_F(CompilationTests, MultipleRoundsOfCompilation) {
    ASSERT_TRUE(ready);

    // I got a nasty memory corruption issue once from my impl not being able to handle
    // multiple rounds of compilation due to failing to cleanup properly between each,
    // so I decided to create this test to cover this behaviour

    std::string txt = R"(

import fns.abc;

fn f() {
    var temp: Int = 100;
    observeInt(temp);
}

)";

    std::string txt_multi_a = R"(

import fns.abc;

fn f() {
    var temp: Int = 41;
    observeInt(temp);
}

)";

    std::string txt_multi_b = R"(

import fns.abc;

fn f() {
    var temp: Int = 1_001;
    observeInt(temp);
}

)";

    // manually setup and call dm->import rather than using perform_compile

    // also, we're gonna use dm->load instead of dm->import

    our_test_parcel->our_src = txt;
    our_test_parcel->our_src_multi_a = txt_multi_a;
    our_test_parcel->our_src_multi_b = txt_multi_b;

    auto f1 = dm->load("a:f"_str);
    auto f2 = dm->load("a.multi.a:f"_str);
    auto f3 = dm->load("a.multi.b:f"_str);
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    ASSERT_TRUE(f3);

    ASSERT_EQ(f1->kind(), yama::kind::function);
    ASSERT_EQ(f2->kind(), yama::kind::function);
    ASSERT_EQ(f3->kind(), yama::kind::function);
    ASSERT_EQ(f1->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f2->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f3->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f1).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*f2).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*f3).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
    EXPECT_EQ(ctx->local(1).value(), ctx->new_none());
    EXPECT_EQ(ctx->local(2).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(100);
    expected.observe_int(41);
    expected.observe_int(1001);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

TEST_F(CompilationTests, Fail_LexicalError) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    $$$$$$ // <- lexical error
}

)";

    ASSERT_FALSE(perform_compile(txt));

    // IMPORTANT: I originally tried having seperate lexical/syntactic dsignal values,
    //            but I found that it's actually pretty hard to reliably differentiate
    //            between purely lexical and purely syntactic errors in our LL(1) system

    ASSERT_EQ(dbg->count(yama::dsignal::compile_syntax_error), 1);
}

TEST_F(CompilationTests, Fail_SyntaxError) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    fn // <- lexically sound, but syntax error
}

)";

    ASSERT_FALSE(perform_compile(txt));

    ASSERT_EQ(dbg->count(yama::dsignal::compile_syntax_error), 1);
}


static_assert(yama::ptypes == 7); // reminder
static_assert(yama::kinds == 4); // reminder

// general
// 
//      - there will exist a 'yama' module available for import (otherwise
//        compilation fails)
// 
//      - the 'yama' module is guaranteed to be the root of the 'yama' parcel
//        auto-installed by the domain compilation is being performed in
//        relation to (otherwise compilation fails)
// 
//      - 'yama' module makes builtin types available
//          - None
//          - Int
//          - UInt
//          - Float
//          - Bool
//          - Char
//          - Type
//
//      - None default initializes to the stateless None object
//      - Int default initializes to 0
//      - UInt default initializes to 0u
//      - Float default initializes to 0.0
//      - Bool default initializes to false
//      - Char default initializes to '\0'
//      - Type default initializes to yama:None type object
// 
//      - functions default initialize to their sole stateless object
//      - methods default initialize to their sole stateless object
//      - structs default initialize to their sole stateless object
//          * TODO: when we add struct properties, this'll need to change
// 
//      - None is non-callable
//      - Int is non-callable
//      - UInt is non-callable
//      - Float is non-callable
//      - Bool is non-callable
//      - Char is non-callable
//      - Type is non-callable
// 
//      - functions are callable (obviously)
//      - methods are callable (obviously)
//      - structs are non-callable
// 
//      - expression precedence order:
//          * TODO: add when needed
//
//      - when a panic occurs, the system is to act as expected (halting
//        program behaviour and unwinding the call stack)

// builtin types

TEST_F(CompilationTests, General_BuiltIns) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // implicit yama import makes expected builtins available

    var a1: None;
    var a2: Int;
    var a3: UInt;
    var a4: Float;
    var a5: Bool;
    var a6: Char;
    var a7: Type;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init None

TEST_F(CompilationTests, General_DefaultInit_None) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> None {
    var a: None; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Int

TEST_F(CompilationTests, General_DefaultInit_Int) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    var a: Int; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init UInt

TEST_F(CompilationTests, General_DefaultInit_UInt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> UInt {
    var a: UInt; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:UInt");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_uint(0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Float

TEST_F(CompilationTests, General_DefaultInit_Float) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Float {
    var a: Float; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_float(0.0));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Bool

TEST_F(CompilationTests, General_DefaultInit_Bool) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Bool {
    var a: Bool; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Bool");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_bool(false));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Char

TEST_F(CompilationTests, General_DefaultInit_Char) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Char {
    var a: Char; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Char");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_char('\0'));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init Type

TEST_F(CompilationTests, General_DefaultInit_Type) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Type {
    var a: Type; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Type");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_type(ctx->none_type()));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init function

TEST_F(CompilationTests, General_DefaultInit_Fn) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() {}

fn f() -> g {
    var a: g; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto g = dm->load("a:g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init method

TEST_F(CompilationTests, General_DefaultInit_Method) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() -> A::g {
    var a: A::g; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto A_g = dm->load("a:A::g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(A_g);
    ASSERT_TRUE(f);

    ASSERT_EQ(A_g->kind(), yama::kind::method);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(A_g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:A::g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*A_g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init struct

TEST_F(CompilationTests, General_DefaultInit_Struct) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct G {}

fn f() -> G {
    var a: G; // default inits a
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto G = dm->load("a:G"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(G);
    ASSERT_TRUE(f);

    ASSERT_EQ(G->kind(), yama::kind::struct0);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:G");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value().t, *G);

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// None is non-callable

TEST_F(CompilationTests, General_None_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn get_none() {}

fn f() {
    get_none()(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Int is non-callable

TEST_F(CompilationTests, General_Int_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    300(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// UInt is non-callable

TEST_F(CompilationTests, General_UInt_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    300u(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Float is non-callable

TEST_F(CompilationTests, General_Float_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    3.14159(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Bool is non-callable

TEST_F(CompilationTests, General_Bool_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    true(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Char is non-callable

TEST_F(CompilationTests, General_Char_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    'a'(); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// Type is non-callable

TEST_F(CompilationTests, General_Type_IsNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // gotta use an arg '10' to avoid forming default init expr

    None(10); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    // we also gotta check for 'compile_wrong_arg_count', instead of 'compile_invalid_operation'

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// functions are callable

TEST_F(CompilationTests, General_Fns_AreCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn identity_int(x: Int) -> Int {
    return x;
}

fn f() {
    observeInt(identity_int(10));
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto identity_int = dm->load("a:identity_int"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(identity_int);
    ASSERT_TRUE(f);

    ASSERT_EQ(identity_int->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(identity_int->callsig().value().fmt(), "fn(yama:Int) -> yama:Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// methods are callable

TEST_F(CompilationTests, General_Methods_AreCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct Util {}

fn Util::identity_int(x: Int) -> Int {
    return x;
}

fn f() {
    observeInt(Util::identity_int(10));
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto Util_identity_int = dm->load("a:Util::identity_int"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(Util_identity_int);
    ASSERT_TRUE(f);

    ASSERT_EQ(Util_identity_int->kind(), yama::kind::method);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(Util_identity_int->callsig().value().fmt(), "fn(yama:Int) -> yama:Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// structs are non-callable

TEST_F(CompilationTests, General_Structs_AreNonCallable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn f() {
    // gotta use an arg '10' to avoid forming default init expr

    A(10); // not callable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    // we also gotta check for 'compile_wrong_arg_count', instead of 'compile_invalid_operation'

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// expr precedence order

TEST_F(CompilationTests, General_ExprPrecedenceOrder) {
    ASSERT_TRUE(ready);

    // TODO
}

// panic behaviour

TEST_F(CompilationTests, General_PanicBehaviour) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    observeInt(0);
    doPanic(); // <- panics
    observeInt(1); // <- shouldn't reach
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// illegal to compile w/out a 'yama' module being available

TEST_F(CompilationTests, General_Fail_InvalidParcelEnv_NoYamaModuleAvailable) {
    ASSERT_TRUE(ready);

    // IMPORTANT: note how this test forgoes using our helpers like _perform_compile,
    //            and instead do everything *manually*
    
    auto our_invalid_parcel = yama::make_res<test_invalid_parcel>();
    
    yama::install_batch ib{};
    ib
        .install("bad"_str, our_invalid_parcel);
    ASSERT_TRUE(dm->install(std::move(ib)));

    // perfectly valid Yama code, but should fail compile up-front due to
    // no 'yama' dep to use for implicit import decl

    std::string txt = R"(

fn f() {}

)";

    our_invalid_parcel->our_src = txt;
    ASSERT_FALSE(dm->import("bad"_str).has_value());

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_env), 1);
}

// illegal to compile w/ a 'yama' module from a 'yama' parcel which is not
// the auto-installed yama parcel of the domain

TEST_F(CompilationTests, General_Fail_InvalidParcelEnv_YamaModuleIsNotFromYamaParcelAutoInstalledIntoDomain) {
    ASSERT_TRUE(ready);

    // IMPORTANT: note how this test forgoes using our helpers like _perform_compile,
    //            and instead do everything *manually*

    auto our_parcel = yama::make_res<test_parcel>();

    // we *incorrectly* map 'yama' dep to 'helper', rather than 'yama'

    yama::install_batch ib{};
    ib
        .install("bad"_str, our_parcel)
        .install("helper"_str, yama::make_res<fns_parcel>())
        .map_dep("bad"_str, "yama"_str, "helper"_str) // <- wrong!
        .map_dep("bad"_str, "fns"_str, "fns"_str)
        .map_dep("bad"_str, "fns2"_str, "fns2"_str)
        .map_dep("bad"_str, "other"_str, "other"_str)
        .map_dep("helper"_str, "yama"_str, "yama"_str);
    ASSERT_TRUE(dm->install(std::move(ib)));

    // perfectly valid Yama code, but should fail compile up-front due to
    // 'yama' dep to use for implicit import decl being *wrong*

    std::string txt = R"(

fn f() {}

)";

    our_parcel->our_src = txt;
    ASSERT_FALSE(dm->import("bad"_str).has_value());

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_env), 1);
}


// multi-source compilation
//
//      - below tests layout rules of multi-source compilation

// multi-source compilation where compiling modules are added to compilation by import decls
// (inducing their compilation)

TEST_F(CompilationTests, MultiSourceCompilation_DirectCompilingModuleAddition) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation

fn f() {
    aaa(); // self.multi.a:aaa
    bbb(); // self.multi.b:bbb
    ccc(); // self.multi.c:ccc
}

)";

    std::string txt_multi_a = R"(

import fns.abc;

fn aaa() {
    observeChar('a');
}

)";

    std::string txt_multi_b = R"(

import fns.abc;

fn bbb() {
    observeChar('b');
}

)";

    std::string txt_multi_c = R"(

import fns.abc;

fn ccc() {
    observeChar('c');
}

)";

    const auto result = perform_compile(txt, txt_multi_a, txt_multi_b, txt_multi_c);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// multi-source compilation where compiling modules added to compilation by import decls
// (inducing their compilation) in turn induce the compilation of yet more modules

TEST_F(CompilationTests, MultiSourceCompilation_IndirectCompilingModuleAddition) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.multi.a; // add new src to compilation

fn f() {
    aaa(); // self.multi.a:aaa
}

)";

    std::string txt_multi_a = R"(

import self.multi.b; // add new src to compilation
import fns.abc;

fn aaa() {
    observeChar('a');
    bbb(); // self.multi.b:bbb
}

)";

    std::string txt_multi_b = R"(

import self.multi.c; // add new src to compilation
import fns.abc;

fn bbb() {
    observeChar('b');
    ccc(); // self.multi.c:ccc
}

)";

    std::string txt_multi_c = R"(

import fns.abc;

fn ccc() {
    observeChar('c');
}

)";

    const auto result = perform_compile(txt, txt_multi_a, txt_multi_b, txt_multi_c);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// multi-parcel multi-source compilation

TEST_F(CompilationTests, MultiSourceCompilation_MultiParcel) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation
import other.multi.d; // add new src to compilation (from another parcel!)

fn f() {
    aaa(); // self.multi.a:aaa
    bbb(); // self.multi.b:bbb
    ccc(); // self.multi.c:ccc
    ddd(); // other.multi.d:ddd
}

)";

    std::string txt_multi_a = R"(

import fns.abc;

fn aaa() {
    observeChar('a');
}

)";

    std::string txt_multi_b = R"(

import fns.abc;

fn bbb() {
    observeChar('b');
}

)";

    std::string txt_multi_c = R"(

import fns.abc;

fn ccc() {
    observeChar('c');
}

)";

    std::string txt_multi_d = R"(

import fns.abc;

fn ddd() {
    observeChar('d');
}

)";

    const auto result = perform_compile(txt, txt_multi_a, txt_multi_b, txt_multi_c, txt_multi_d);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'a');
    expected.observe_char(U'b');
    expected.observe_char(U'c');
    expected.observe_char(U'd');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// multi-source compilation where import graph cycles between compiling modules do not cause issues

// remember that parcels aren't allowed to have dep graph cycles between them, so we need-not
// worry about the notion of multi-parcel dep graph cycles

TEST_F(CompilationTests, MultiSourceCompilation_TolerateCompilingModuleImportGraphCycles) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation

fn f() {
    aaa(); // self.multi.a:aaa
    bbb(); // self.multi.b:bbb
    ccc(); // self.multi.c:ccc
}

)";

    std::string txt_multi_a = R"(

// will form dep graph cycles w/ other compiling modules

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation
import fns.abc;

fn get_b() -> Char {
    return 'b';
}

fn aaa() {
    observeChar(get_a()); // self.multi.c:get_a
}

)";

    std::string txt_multi_b = R"(

// will form dep graph cycles w/ other compiling modules

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation
import fns.abc;

fn get_c() -> Char {
    return 'c';
}

fn bbb() {
    observeChar(get_b()); // self.multi.a:get_b
}

)";

    std::string txt_multi_c = R"(

// will form dep graph cycles w/ other compiling modules

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation
import fns.abc;

fn get_a() -> Char {
    return 'a';
}

fn ccc() {
    observeChar(get_c()); // self.multi.b:get_c
}

)";

    const auto result = perform_compile(txt, txt_multi_a, txt_multi_b, txt_multi_c);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// multi-source compilation failure (including w/ the error arising in another parcel!)

TEST_F(CompilationTests, MultiSourceCompilation_Fail_InducedCompilationFails) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.multi.a; // add new src to compilation
import self.multi.b; // add new src to compilation
import self.multi.c; // add new src to compilation
import other.multi.d; // add new src to compilation (from another parcel!)

fn f() {
    aaa(); // self.multi.a:aaa
    bbb(); // self.multi.b:bbb
    ccc(); // self.multi.c:ccc
    ddd(); // other.multi.d:ddd
}

)";

    std::string txt_multi_a = R"(

import fns.abc;

fn aaa() {
    observeChar('a');
}

)";

    std::string txt_multi_b = R"(

import fns.abc;

fn bbb() {
    observeChar('b');
}

)";

    std::string txt_multi_c = R"(

import fns.abc;

fn ccc() {
    observeChar('c');
}

)";

    std::string txt_multi_d = R"(

import fns.abc;

fn ddd() {
    unknown_fn(); // <- error! no fn named unknown_fn
}

)";

    ASSERT_FALSE(perform_compile(txt, txt_multi_a, txt_multi_b, txt_multi_c, txt_multi_d));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}


// decl & scope
// 
//      - 'decls' bind name identifiers to imports, types, parameters and local vars
//      - decls have 'scopes' which define when their binding is valid
//
//      - decl scope is dictated by what 'block' they were declared in,
//        as well as what region within said block the decl is valid for
//      - blocks may be nested within one another, forming hierarchies
//        of blocks w/ parent/child relationships
// 
//      - blocks are either either 'global' or 'local'
//      - global blocks define the top-level global scope inside of which
//        imports and types are defined
//      - local blocks define local scopes within function bodies and
//        the nested scopes within statements
//      - for any given chunk of Yama code, the global block is the root
//        of the block hierarchy
// 
//      - decls in the same block may not share names, unless otherwise specified
//          * TODO: we don't really have tests covering things like whether impl can
//                  detect name conflicts between types whos' fullnames are more nuanced
//                  than single identifiers (eg. methods)
// 
//      - within the global block are contained all types loaded from modules imported
//        via import decls
//      - within the global block, types are differentiated by parcel env 'fullname'
//          * this means that it's allowed for types defined in the Yama code to share
//            unqualified names w/ extern types (as they shouldn't conflict)
//          * unqualified name conflicts should only arise on reference
// 
//      - given a type and import decl w/ a common unqualified name, the two decls
//        are not considered to be in conflict
//          * this is okay as types and imports operate w/ different lookup procedures
// 
//      - when looking up an identifier via an unqualified name, the lookup occurs w/
//        either 'normal lookup procedure' or 'qualifier lookup procedure'
// 
//          - normal lookup procedure is the default, and is used to lookup types,
//            parameters and local vars, ignoring imports
// 
//          - qualifier lookup procedure is used to lookup imports used to qualify
//            identifier exprs, and ignores all decls except imports
// 
//      - decls may 'shadow' one another
// 
//          - decls in child blocks shadow ones in ancestoral blocks by using the same
//            name as them
// 
//          - given an extern type and a type defined in Yama code w/ the same
//            unqualified name, unqualified name access of this name will select the
//            type defined in Yama code, shadowing the extern type
// 
//          - qualified identifier exprs bypass shadowing
// 
//      - the different scopes decls have are as follows:
// 
//          - import decls exist in the global block, w/ their scope beginning/ending
//            at the start/end of this block
//              * these are in scope at all points in the Yama code
// 
//          - extern types exist in the global block, w/ their scope beginning/ending
//            at the start/end of this block
//              * these are in scope at all points in the Yama code
// 
//          - type decls exist in the global block, w/ their scope beginning/ending
//            at the start/end of this block
//              * these are in scope at all points in the Yama code, including prior
//                to the decl itself
//          
//          - parameter decls exist in the local block of their function's body,
//            w/ their scope beginning immediately after their decl (which here includes
//            the type annot bound to it, if any), and ending at the end of this block
//              * this means parameter's type annot expr cannot reference the parameter
// 
//              * TODO: maybe expand tests for this to include success & shadowing
// 
//          - local var decls exist in the local block they are declared in,
//            w/ their scope beginning immediately after their decl, and ending
//            at the end of their block
//              * this means local var's type annot expr cannot reference the local var
//              * this means local var's init expr cannot reference the local var
// 
//              * TODO: maybe expand tests for this to include success & shadowing
// 
//      - dead code (ie. code not reachable from entrypoint) is still subject to
//        error checking

// declaring types in global scope will result in new types being made available to the
// domain upon module import (which should happen immediately if import caused compile)

TEST_F(CompilationTests, DeclAndScope_DeclaredTypesBecomeAvailableToDomainUponCompiledModuleImport) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// types loaded from outside modules (ie. extern types) should be available to Yama code

TEST_F(CompilationTests, DeclAndScope_ExternTypes_MayBeUsedByYamaCode) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    // below usage of types from implicitly imported 'yama' module, alongside usage of
    // fns from imported fns.abc, cover the implicit decl of types made available to
    // Yama code from outside modules

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// types defined by Yama code should be available for use by said Yama code

TEST_F(CompilationTests, DeclAndScope_TypesDefinedInYamaCode_MayBeUsedByYamaCode) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto a = dm->load("a:a"_str);
    const auto b = dm->load("a:b"_str);
    const auto c = dm->load("a:c"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(f);

    ASSERT_EQ(a->kind(), yama::kind::function);
    ASSERT_EQ(b->kind(), yama::kind::function);
    ASSERT_EQ(c->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(a->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(b->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(c->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// types defined in Yama code may share unqualified name w/ extern type

TEST_F(CompilationTests, DeclAndScope_TypeDefinedInYamaCode_SharesUnqualifiedName_WithExternType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn Int() -> Char { // shares unqualified name w/ yama:Int
    return 'a';
}

fn observeInt() -> Char { // shares unqualified name w/ fns.abc:observeInt
    return 'b';
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto our_Int = dm->load("a:Int"_str);
    const auto our_observeInt = dm->load("a:observeInt"_str);
    ASSERT_TRUE(our_Int);
    ASSERT_TRUE(our_observeInt);

    ASSERT_EQ(our_Int->kind(), yama::kind::function);
    ASSERT_EQ(our_observeInt->kind(), yama::kind::function);
    ASSERT_EQ(our_Int->callsig().value().fmt(), "fn() -> yama:Char");
    ASSERT_EQ(our_observeInt->callsig().value().fmt(), "fn() -> yama:Char");

    ASSERT_TRUE(ctx->push_fn(*our_Int).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*our_observeInt).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_char(U'a'));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_char(U'b'));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// IMPORTANT: this test mainly covers the coexistence between *shadower* and *shadowed*
//            decls, w/ the behaviour of shadowing in regards to referencing behaviour
//            being deferred to tests for identifier exprs

// tolerance of type/parameter/local var decls having common names so
// long as they're in different blocks

TEST_F(CompilationTests, DeclAndScope_DeclShadowingBehaviour) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn a() {} // (unqualified ref to) fn 'a' will be shadowed by 'a's below, w/ this test covering coexistence mainly

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);
    
    const auto a = dm->load("a:a"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(a);
    ASSERT_TRUE(f);

    ASSERT_EQ(a->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(a->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Int) -> yama:None");

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

// IMPORTANT: like w/ shadowing, coexistence is tested here, but the particulars of referencing
//            are left to identifier expr tests

// test that given a type decl and an import decl w/ a common unqualified name, that
// the two can coexist w/out a name conflict arising

TEST_F(CompilationTests, DeclAndScope_TypeAndImportWithCommonUnqualifiedNameDoNotCauseNameConflict) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;

fn a() -> Int {
    return 3;
}

fn f() {
    a:observeInt(a()); // <- the two 'a's coexist w/out conflict
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto a = dm->load("a:a"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(a);
    ASSERT_TRUE(f);

    ASSERT_EQ(a->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(a->callsig().value().fmt(), "fn() -> yama:Int");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(3);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// test to ensure impl properly impls parameter scope not starting until after the
// decl itself, w/ this meaning that the type annot exprs of the parameter cannot
// reference itself

TEST_F(CompilationTests, DeclAndScope_ParamScopeBeginsWhenExpected_ForTypeAnnotExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

// param 'a' won't come into scope until at least 'd'

fn f(a, b, c: a, d: Int) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// test to ensure impl properly impls local var scope not starting until after the
// decl itself, w/ this meaning that the type annot exprs of the local var cannot
// reference itself

TEST_F(CompilationTests, DeclAndScope_LocalVarScopeBeginsWhenExpected_ForTypeAnnotExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: a; // <- local var 'a' hasn't come into scope yet!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// test to ensure impl properly impls local var scope not starting until after the
// decl itself, w/ this meaning that the init exprs of the local var cannot reference
// itself

TEST_F(CompilationTests, DeclAndScope_LocalVarScopeBeginsWhenExpected_ForInitExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: Int = a; // <- local var 'a' hasn't come into scope yet!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal to declare two or more imports in global scope w/ a common name

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_Import_Import) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;
import a: yama; // error! name 'a' already taken

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare import w/ name of implicit 'yama' import

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_Import_Import_OneIsImplicitYamaImport) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import yama: fns.abc; // error! name 'yama' already taken

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare two or more types in global scope w/ a common name

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_Type_Type_BothFromYamaCode) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {}

fn f(a: Int) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal for a parameter to have the same name as another parameter
// in the same parameter list

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_Param_Param) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a, a: Int) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in function body block w/ the name
// of a parameter

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_LocalVar_Param) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a: Int) {
    var a = 3.14159;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in function body block w/ the name
// of another local var in that block

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_LocalVar_LocalVar_BothInFnBodyBlock) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = 10;
    var a = 3.14159;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in a nested local block w/ the name
// of another local var in that block

TEST_F(CompilationTests, DeclAndScope_Fail_NameConflict_LocalVar_LocalVar_BothInNestedBlock) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    if (true) {
        var a = 10;
        var a = 3.14159;
    }
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// dead code is still subject to error checking

TEST_F(CompilationTests, DeclAndScope_Fail_IfDeadCodeIsInError) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    return;
    // dead code
    var a = 10;
    var a = 3.14159;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}


// type specifier
// 
//      - accepts a yama:Type crvalue to resolve something's type
//
//      - if crvalue is a fn type, this will be *ad hoc* implicit converted into a yama:Type
//      - if crvalue is a method type, this will be *ad hoc* implicit converted into a yama:Type
//          TODO: replace these w/ proper (constexpr!) implicit converts later

// type spec w/ yama:Type crvalue

TEST_F(CompilationTests, TypeSpecifier_ExprIsTypeType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int { // <- yama:Type crvalue
    return 100;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(100));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// type spec w/ fn type crvalue (ad hoc implicit convert)

TEST_F(CompilationTests, TypeSpecifier_ExprIsFnType_AdHocImplicitConvert) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> g { // <- fn type crvalue
    return g;
}

fn g() {}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    const auto g = dm->load("a:g"_str);
    ASSERT_TRUE(f);
    ASSERT_TRUE(g);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:g");
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// type spec w/ method type crvalue (ad hoc implicit convert)

TEST_F(CompilationTests, TypeSpecifier_ExprIsMethodType_AdHocImplicitConvert) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> A::g { // <- method type crvalue
    return A::g;
}

struct A {}

fn A::g() {}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    const auto A_g = dm->load("a:A::g"_str);
    ASSERT_TRUE(f);
    ASSERT_TRUE(A_g);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(A_g->kind(), yama::kind::method);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:A::g");
    ASSERT_EQ(A_g->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*A_g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal type spec due to expr not yama:Type, fn or method type

TEST_F(CompilationTests, TypeSpecifier_Fail_ExprTypeIsNotTypeType_FnType_OrMethodType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> 100 { // <- '100' is valid crvalue, but is not yama:Type nor fn/method type
    //
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_a_type), 1);
}

// illegal type spec due to expr not constexpr

TEST_F(CompilationTests, TypeSpecifier_Fail_ExprIsNonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() -> Type {
    return None;
}

fn f() -> g() { // <- valid Type value, but not constexpr
    //
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}


// IMPORTANT: multi-source compilation is tested elsewhere

// import decl
// 
//      - each translation unit has an 'import set' of modules
// 
//          - the set of 'extern types' of the translation unit is the sum of the sets of
//            types exposed by the modules in its import set, w/ these types being made
//            available to the translation unit
// 
//          - extern types may be referenced w/ (import decl name) qualification
// 
//          - extern types may be referenced w/out qualification if and only if their are
//            not multiple extern types w/ the same unqualified name
// 
//      - import decl adds module specified by 'import path specifier' to import set
// 
//          - import path specifiers are relative to the parcel env of the parcel
//            of the compiling module
// 
//      - import decl name is *optional*
// 
//          - no name means qualified type access is not available
//
//      - import decl must appear only in global block
//      - import decl must appear prior to first type decl
//          * this makes impl easier
// 
//      - multiple import decls may import the same module
// 
//          - in which case said import decls are considered simply different aliases for
//            the same module, sharing the same set of exposed extern types
// 
//      - import decls may import the compiling module itself
//
//      - import decls must specify module which is valid 
//      - import decls may induce multi-source compilation (recursively)
// 
//      - translation units implicitly contain a special implicit import decl which imports
//        the 'yama' module
//
//          - import decl has name 'yama'
//
//          - this module is the root of the 'yama' dep of the compiling module's
//            parcel, which is expected to map to the special parcel defining
//            builtin Yama types
//
//          - due to rules defined above in 'general', this module is guaranteed to
//            be available, and its contents should be as expected

// explicit import decl, unnamed, module imported from dependency

TEST_F(CompilationTests, ExplicitImportDecl_Unnamed_ModuleImportedFromDep) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    observeInt(10); // fns.abc:observeInt
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// explicit import decl, unnamed, module imported from same parcel as compiling module

TEST_F(CompilationTests, ExplicitImportDecl_Unnamed_ModuleImportedFromSameParcelAsCompilingModule) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self.abc;

fn f() {
    acknowledge(); // self.abc:acknowledge
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_misc("ack");
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// explicit import decl, named, module imported from dependency

TEST_F(CompilationTests, ExplicitImportDecl_Named_ModuleImportedFromDep) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import abc: fns.abc;

fn f() {
    abc:observeInt(10); // fns.abc:observeInt
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// explicit import decl, named, module imported from same parcel as compiling module

TEST_F(CompilationTests, ExplicitImportDecl_Named_ModuleImportedFromSameParcelAsCompilingModule) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import abc: self.abc;

fn f() {
    abc:acknowledge(); // self.abc:acknowledge
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_misc("ack");
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// extern type set contains types w/ same unqualified name

TEST_F(CompilationTests, ImportDecl_ExternTypeSetContainsTypesWithSameUnqualifiedName) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;
import fns2.abc; // <- won't conflict

fn f() {}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// implicit yama import decl

TEST_F(CompilationTests, ImportDecl_ImplicitYamaImportDecl) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() -> yama:Int { // 'yama' name is used by implicit import decl
    return 10;
}

fn f() -> Int { // Int exposed by implicit yama import
    return g(); // <- yama:Int and Int are same type
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// redundant import decls are tolerated

TEST_F(CompilationTests, ImportDecl_RedundantImportDeclsAreTolerated) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;
import fns.abc;
import fns.abc;
import fns.abc;
import fns.abc;
import fns.abc;

fn f() {
    observeInt(10); // observeInt is in fns.abc
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// redundant import decls are tolerated, for implicit yama import decl

TEST_F(CompilationTests, ImportDecl_RedundantImportDeclsAreTolerated_ForImplicitYamaImportDecl) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import yama;
import yama;
import yama;
import yama;

fn f() -> Int { // Int exposed by implicit yama import
    return 10;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// redundant import decls are tolerated, type accessed via multiple import decl name qualifiers

TEST_F(CompilationTests, ImportDecl_RedundantImportDeclsAreTolerated_TypeAccessedViaMultipleImportDeclNameQualifiers) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import abc1: fns.abc;
import abc2: fns.abc;

fn g(x: observeInt) {} // <- test can unqualify access observeInt

fn f() {
    g(abc1:observeInt); // <- abc1:observeInt is same type as observeInt
    g(abc2:observeInt); // <- abc2:observeInt is same type as observeInt
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// import decls importing the compiling module itself are tolerated

// TODO: maybe seperate out below into its own unit test

// here we also test that types accessed via qualifying w/ name of self import
// are the same types as those being defined in the compiling module itself

TEST_F(CompilationTests, ImportDecl_ImportDeclsImportingTheCompilingModuleItselfAreTolerated) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self;
import self;
import self;
import a: self; // <- give this one a name to test w/
import self;

fn h() {}

fn g(x: h) {} // <- assert that type of arg passed is same type as 'h'

fn f() -> Int {
    g(h); // <- should work
    g(a:h); // <- should also work (ie. h and a:h are same type)
    return 10;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal to import non-existent module

TEST_F(CompilationTests, ImportDecl_Fail_ImportNonExistentModule) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns; // error! fns parcel has no root module!

fn f() -> Int {
    return 10;
}

)";

    ASSERT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_import), 1);
}

// illegal to import non-existent module, w/ import failing due to invalid module

// I want to test this explicitly to ensure that the particular part of the
// impl which handles importing during compilation knows to statically verify
// imported modules

TEST_F(CompilationTests, ImportDecl_Fail_ImportInvalidModule) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.bad; // error! fns.bad was invalid!

fn f() -> Int {
    return 10;
}

)";

    ASSERT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_import), 1);
}

// illegal for import decl to exist in local scope

TEST_F(CompilationTests, ImportDecl_Fail_ImportDeclExistsInLocalScope) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    import fns.abc; // error! illegal in local scope!
}

)";

    ASSERT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_misplaced_import), 1);
}

// illegal for import decl to exist after first type decl

TEST_F(CompilationTests, ImportDecl_Fail_ImportDeclExistsAfterFirstTypeDecl) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    //...
}

import fns.abc; // error! illegal after first type decl

)";

    ASSERT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_misplaced_import), 1);
}


// variable decl
//
//      - defines non-local var if appears in global block
//          * illegal in current version of Yama
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

TEST_F(CompilationTests, VariableDecl_TypeAnnot_NoInitAssign) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    var a: Char;
    observeChar(a);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, VariableDecl_NoTypeAnnot_InitAssign) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    var a = 'y';
    observeChar(a);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, VariableDecl_TypeAnnot_InitAssign) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    var a: Char = 'y';
    observeChar(a);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, VariableDecl_Fail_If_NoTypeAnnot_NoInitAssign) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_local_var), 1);
}

// local var initialization and mutability

TEST_F(CompilationTests, VariableDecl_InitializationAndMutability) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, VariableDecl_Fail_IfNonLocalVar) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

var a = 10;

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_nonlocal_decl), 1);
}

// illegal to initialize local var w/ wrong typed expr

TEST_F(CompilationTests, VariableDecl_Fail_InitExprIsWrongType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: Int = 3.14159;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// function decl
//
//      - defines non-local function if appears in global block
// 
//      - defines local function if appears in local block
//          * illegal in current version of Yama
// 
//      - forms w/ explicit return type explicitly specify their return type
//      - forms w/out explicit return type have None as their return type implicitly
// 
//      - w/ return type None, regardless of whether return type is explicit or implicit,
//        'return;' (aka. None return stmt) may be used
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

// function w/ explicit return type, which is None, may use 'return;'

TEST_F(CompilationTests, FunctionDecl_ExplicitReturnType_WhichIsNone_MayUseNoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> None {
    return;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ implicit return type, which is None, may use 'return;'

TEST_F(CompilationTests, FunctionDecl_ImplicitReturnType_WhichIsNone_MayUseNoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    return;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// function w/ explicit return type, which is NOT None, w/ all control paths exiting
// via return stmt

TEST_F(CompilationTests, FunctionDecl_ExplicitReturnType_WhichIsNotNone_AllControlPathsExitViaReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

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

TEST_F(CompilationTests, FunctionDecl_ExplicitReturnType_WhichIsNotNone_NoReturnStmtOnAnyControlPath_InsteadEnterInfiniteLoop_ExitViaPanic) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

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

TEST_F(CompilationTests, FunctionDecl_ExplicitReturnType_WhichIsNone_AllControlPathsExitViaReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_ExplicitReturnType_WhichIsNone_AllControlPathsExitViaImplicitReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_ImplicitReturnType_WhichIsNone_AllControlPathsExitViaReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_ImplicitReturnType_WhichIsNone_AllControlPathsExitViaImplicitReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_ParamList) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f(a, b, c: Int, d: Bool, e: Char) {
    observeInt(a);
    observeInt(b);
    observeInt(c);
    observeBool(d);
    observeChar(e);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Int, yama:Int, yama:Int, yama:Bool, yama:Char) -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_ParamList_WithMaxParams) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int, yama:Int) -> yama:None");

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

TEST_F(CompilationTests, FunctionDecl_Fail_IfLocalFn) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    fn g() {}
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_local_decl), 1);
}

// illegal function, w/ non-None return type, w/ a control path from entrypoint which 
// does not end w/ a return stmt

TEST_F(CompilationTests, FunctionDecl_Fail_IfNonNoneReturnType_AndControlPathFromEntrypointNotEndingInReturnStmt) {
    ASSERT_TRUE(ready);

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

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_no_return_stmt), 1);
}

// illegal function w/ param list '(a, b: Int, c)'

TEST_F(CompilationTests, FunctionDecl_Fail_IfInvalidParamList) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a, b: Int, c) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_param_list), 1);
}

// illegal function w/ >24 parameters

TEST_F(CompilationTests, FunctionDecl_Fail_IfInvalidParamList_MoreThanTwentyFourParams) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24: Int) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_param_list), 1);
}

// illegal function w/ params of non-object types

TEST_F(CompilationTests, FunctionDecl_Fail_IfParamTypesAreNotObjectTypes) {
    ASSERT_TRUE(ready);

    // TODO: stub until we add non-object types
}

// illegal function w/ return type of non-object type

TEST_F(CompilationTests, FunctionDecl_Fail_IfReturnTypeIsNotObjectType) {
    ASSERT_TRUE(ready);
    // TODO: what do we do w/ this test + semantic?
    // TODO: will Yama even EVER have non-object types? or is this distinction meaningless?

    // TODO: stub until we add non-object types
}


// TODO: maybe in the future refactor below tests to use some kind of parameterized
//       test suite thing, or other abstraction, to make below more properly comprehensive

// IMPORTANT: method decls are expected to be in the impl an extension of regular fns,
//            and so method decls will only test basics and differences w/ fn decls,
//            and the rest will be deferred to fn tests

// method decl
//
//      - defines non-local method if appears in global block
// 
//      - defines local method if appears in local block
//          * illegal in current version of Yama
//
//      - the owner type must exist, and in the same module as the method
//      - the owner type must be an object type
//          * TODO: but what if we want to add something equiv to like a 'static class'
//                  type kind which can have methods, but is not an *object* type?
//          * TODO: when we add things like 'constants', they're the things that aren't
//                  able to have methods, so maybe the answer is to do something like
//                  rename 'types' in modules to 'items', and then replace this current
//                  rule w/ one requiring owner type item to be a *type item*
//
//      - if the first param of the method is named 'self', and it lacks an explicit
//        type annotation directly attached to it, then this self parameter's type will
//        specially be deduced to be the owner type of the method, overriding the usual
//        semantics governing param type deduction
//          * such params are called method 'self parameters'
//          * notice how the above semantics also mean that self parameters w/ no params
//            after them w/ type annotations are still able to have a type deduced
//          * 'fn A::f(self) {}' is equiv to 'fn A::f(self: A) {}'
//          * 'fn A::f(self, x: Int) {}' is equiv to 'fn A::f(self: A, x: Int) {}'
//          * 'fn A::f(self: Int) {}' wouldn't be affected, as self is directly type
//            annotated
//          * 'fn A::f(x, self, y: Int) {}' wouldn't be affected, as self is not the
//            first param
//          * 'fn A::f() {}' wouldn't be affected, as it has no params

// basic usage

TEST_F(CompilationTests, MethodDecl_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::g(x: Int) -> Int {
    // observe side-effects of call, and its return value
    observeInt(x);
    return x;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto A_g = dm->load("a:A::g"_str);
    ASSERT_TRUE(A_g);

    ASSERT_EQ(A_g->kind(), yama::kind::method);
    ASSERT_EQ(A_g->callsig().value().fmt(), "fn(yama:Int) -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*A_g).good());
    ASSERT_TRUE(ctx->push_int(51).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(51));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(51);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// self parameters

TEST_F(CompilationTests, MethodDecl_SelfParameters) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

// these have self params
fn A::yes1(self) {}
fn A::yes2(self, x: Int) {}

// these do not have self params
fn A::no1(self: Int) {}
fn A::no2(x, self, y: Int) {}
fn A::no3() {}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto A_yes1 = dm->load("a:A::yes1"_str);
    const auto A_yes2 = dm->load("a:A::yes2"_str);
    const auto A_no1 = dm->load("a:A::no1"_str);
    const auto A_no2 = dm->load("a:A::no2"_str);
    const auto A_no3 = dm->load("a:A::no3"_str);
    ASSERT_TRUE(A_yes1);
    ASSERT_TRUE(A_yes2);
    ASSERT_TRUE(A_no1);
    ASSERT_TRUE(A_no2);
    ASSERT_TRUE(A_no3);

    // we don't care about actually calling these methods for this test, we're only
    // concerned about their callsigs

    ASSERT_EQ(A_yes1->kind(), yama::kind::method);
    ASSERT_EQ(A_yes2->kind(), yama::kind::method);
    ASSERT_EQ(A_no1->kind(), yama::kind::method);
    ASSERT_EQ(A_no2->kind(), yama::kind::method);
    ASSERT_EQ(A_no3->kind(), yama::kind::method);
    ASSERT_EQ(A_yes1->callsig().value().fmt(), "fn(a:A) -> yama:None");
    ASSERT_EQ(A_yes2->callsig().value().fmt(), "fn(a:A, yama:Int) -> yama:None");
    ASSERT_EQ(A_no1->callsig().value().fmt(), "fn(yama:Int) -> yama:None");
    ASSERT_EQ(A_no2->callsig().value().fmt(), "fn(yama:Int, yama:Int, yama:Int) -> yama:None");
    ASSERT_EQ(A_no3->callsig().value().fmt(), "fn() -> yama:None");
}

// self parameters work when referenced by identifier exprs

TEST_F(CompilationTests, MethodDecl_SelfParameters_WorkWhenReferencedByIdentifierExprs) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::yes1(self) {
    observeInt(200);
}
fn A::yes2(self, x: Int) {
    observeInt(x);
    self.yes1();
}

fn f() {
    var a = A();
    a.yes2(100);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(100);
    expected.observe_int(200);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal to declare local method

TEST_F(CompilationTests, MethodDecl_Fail_IfLocalMethod) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn f() {
    fn A::g() {}
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_local_decl), 1);
}

// illegal method of non-existent owner type

TEST_F(CompilationTests, MethodDecl_Fail_NonExistentOwnerType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn Missing::g() {} // error! no type 'Missing'

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonexistent_owner), 1);
}

// illegal method of non-object owner type

TEST_F(CompilationTests, MethodDecl_Fail_NonObjectOwnerType) {
    ASSERT_TRUE(ready);

    // TODO: stub until we add non-object types
}


// TODO: when we add properties to structs, we'll need to expand below to include tests
//       covering things like the scope of the body of the struct (see equiv for 'function
//       decl' above as a reference)

// struct decl
//
//      - defines non-local struct if appears in global block
// 
//      - defines local struct if appears in local block
//          * illegal in current version of Yama

// basic usage

TEST_F(CompilationTests, StructDecl_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct G {}

fn f(x: G) -> G {
    return x;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto G = dm->load("a:G"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(G);
    ASSERT_TRUE(f);

    ASSERT_EQ(G->kind(), yama::kind::struct0);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(a:G) -> a:G");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->default_init(yama::newtop, *G).good());
    ASSERT_TRUE(ctx->call(2, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value().t, *G);

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal to declare local struct

TEST_F(CompilationTests, StructDecl_Fail_IfLocalStruct) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    struct G {}
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_local_decl), 1);
}


// assignment stmt
// 
//      - performs assignment of the expr value to an assignable expr
// 
//      - the expr value assigned and the assignable expr must agree on type

// basic usage

TEST_F(CompilationTests, AssignmentStmt_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    var a = 10;
    observeInt(a);
    a = 20;
    observeInt(a);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, AssignmentStmt_Fail_IfAssignNonAssignableExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    3 = 10;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// illegal assignment type mismatch

TEST_F(CompilationTests, AssignmentStmt_Fail_IfTypeMismatch) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = 10;
    a = 'y';
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// IMPORTANT: strictly speaking, the assignment stmt syntactically operates via an
//            expr stmt, but this aspect will be handled by assignment stmt tests,
//            and not covered here

// expr stmt
//
//      - evaluates the nested expr, discarding its return value

// basic usage

TEST_F(CompilationTests, ExprStmt_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    100; // legal, but totally transparent
    observeInt(10); // legal, and not transparent
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, IfStmt_NoElse) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f(a: Bool) {
    observeBool(a);
    if (a) {
        observeInt(-1);
    }
    observeInt(0);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Bool) -> yama:None");

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

TEST_F(CompilationTests, IfStmt_Else) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Bool) -> yama:None");

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

TEST_F(CompilationTests, IfStmt_ElseIfChain) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Char) -> yama:None");

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

TEST_F(CompilationTests, IfStmt_Fail_TypeMismatch) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    if ('y') {}
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// loop stmt
//
//      - branches to a nested block, executes it, and then exits this block, restarting it
//        by branching into a new instance of it, and repeating this process, forming an infinite
//        loop, one which continues until exited by either a break or return stmt

// basic usage, exit via break

TEST_F(CompilationTests, LoopStmt_BasicUsage_ExitViaBreak) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, LoopStmt_BasicUsage_ExitViaReturn) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, BreakStmt_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, BreakStmt_LocalVarInLoopBody) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, BreakStmt_Fail_IfUsedOutsideLoopStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    break;
}

)";

    EXPECT_FALSE(perform_compile(txt));

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

TEST_F(CompilationTests, ContinueStmt_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, ContinueStmt_LocalVarInLoopBody) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, ContinueStmt_Fail_IfUsedOutsideLoopStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    continue;
}

)";

    EXPECT_FALSE(perform_compile(txt));

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

TEST_F(CompilationTests, ReturnStmt_BasicUsage_NonNoneReturnType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() -> Int {
    return 10;
    observeInt(0); // <- shouldn't be reached
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// basic usage, w/ None return type, w/ None return stmt

TEST_F(CompilationTests, ReturnStmt_BasicUsage_NoneReturnType_NoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    return;
    observeInt(0); // <- shouldn't be reached
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// basic usage, w/ None return type, w/ None return stmt

TEST_F(CompilationTests, ReturnStmt_BasicUsage_NoneReturnType_NonNoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn g() {} // <- to get None object (as Yama currently lacks another way to get it)

fn f() {
    return g();
    observeInt(0); // <- shouldn't be reached
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto g = dm->load("a:g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal return stmt due to type mismatch, non-None return stmt

TEST_F(CompilationTests, ReturnStmt_Fail_TypeMismatch_NonNoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    return 3.14159;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}

// illegal return stmt due to type mismatch, None return stmt

TEST_F(CompilationTests, ReturnStmt_Fail_TypeMismatch_NoneReturnStmt) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    return;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}


// TODO: maybe rewrite 'identifier expr' to be a homomorphic expr

static_assert(yama::kinds == 4); // reminder

// identifier expr
// 
//      - specifies a reference to a type, parameter or local var, for sake of either
//        computing a value or assigning something
// 
//      - identifier exprs exist relative to some block, and their ability to reference
//        decls respects the scope and shadowing rules of decls
//
//      - identifier exprs w/ import decl name qualifiers lookup the import to use via
//        qualifier lookup procedure
// 
//      - compute behaviour:
// 
//          primitive type      yama:Type value
//          fn type             fn value
//          struct type         yama:Type value
//          parameter           parameter value
//          local var           local var value
// 
//      - assignability & assignment behaviour:
// 
//          primitive type      illegal                 (non-assignable)
//          fn type             illegal                 (non-assignable)
//          struct type         illegal                 (non-assignable)
//          parameter           illegal                 (non-assignable)
//          local var           assigns local var       (assignable)
// 
//      - constexpr status:
// 
//          primitive type      constexpr
//          fn type             constexpr
//          struct type         constexpr
//          parameter           non-constexpr
//          local var           non-constexpr

// IMPORTANT:
//      below 'IdentifierExpr_Compute_[KIND]Type' tests do the following:
//          - compute value of id expr w/ qualifier (check if correct)
//          - compute value of id expr w/out qualifier (check if correct)
//          - check that above two are equal

// compute value of primitive type reference

TEST_F(CompilationTests, IdentifierExpr_Compute_PrimitiveType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Type {
    return yama:Int; // w/ qualifier
}

fn g() -> Type {
    return Int; // w/out qualifier
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    const auto g = dm->load("a:g"_str);
    ASSERT_TRUE(f);
    ASSERT_TRUE(g);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Type");
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:Type");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*g).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_type(ctx->int_type()));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_type(ctx->int_type()));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// compute value of fn type reference

TEST_F(CompilationTests, IdentifierExpr_Compute_FnType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import abc: fns.abc;

fn f() -> observeInt {
    return abc:observeInt; // w/ qualifier
}

fn g() -> observeInt {
    return observeInt; // w/out qualifier
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto observeInt = dm->load("fns.abc:observeInt"_str);
    const auto f = dm->load("a:f"_str);
    const auto g = dm->load("a:g"_str);
    ASSERT_TRUE(observeInt);
    ASSERT_TRUE(f);
    ASSERT_TRUE(g);

    ASSERT_EQ(observeInt->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(observeInt->callsig().value().fmt(), "fn(yama:Int) -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> fns.abc:observeInt");
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> fns.abc:observeInt");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*g).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*observeInt));
    EXPECT_EQ(ctx->local(1).value(), ctx->new_fn(*observeInt));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// compute value of struct type reference

TEST_F(CompilationTests, IdentifierExpr_Compute_StructType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import abc: fns.abc;

fn f() -> Type {
    return abc:SomeStruct; // w/ qualifier
}

fn g() -> Type {
    return SomeStruct; // w/out qualifier
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto SomeStruct = dm->load("fns.abc:SomeStruct"_str);
    const auto f = dm->load("a:f"_str);
    const auto g = dm->load("a:g"_str);
    ASSERT_TRUE(SomeStruct);
    ASSERT_TRUE(f);
    ASSERT_TRUE(g);

    ASSERT_EQ(SomeStruct->kind(), yama::kind::struct0);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Type");
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:Type");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*g).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value().as_type(), *SomeStruct);
    EXPECT_EQ(ctx->local(1).value().as_type(), *SomeStruct);

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// compute value of parameter reference

TEST_F(CompilationTests, IdentifierExpr_Compute_Param) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f(a: Int) -> Int {
    observeInt(a);
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Int) -> yama:Int");

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

// compute value of local var reference

TEST_F(CompilationTests, IdentifierExpr_Compute_LocalVar) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() -> Int {
    var a = -21;
    observeInt(a);
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(-21));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(-21);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// TODO: since fn types resolve to objects w/ fn type instead of yama:Type, should
//       we have tests for that shadowing can properly discern the correct return
//       type? as right now we're not really testing for that

// extern type shadowed by type defined in Yama code

TEST_F(CompilationTests, IdentifierExpr_TypeDefinedInYamaCodeShadowsExternType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn observeInt() {} // shadows fns.abc:observeInt

fn f() -> observeInt { // return fn value to see if it works
    return observeInt; // <- should be our fn
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto observeInt = dm->load("a:observeInt"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(observeInt);
    ASSERT_TRUE(f);

    ASSERT_EQ(observeInt->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(observeInt->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:observeInt");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(observeInt.value()).value());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// type shadowed by parameter

TEST_F(CompilationTests, IdentifierExpr_ParamShadowsType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn g() {}

fn f(g: Int) {
    observeInt(g); // <- param g shadows fn g
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto g = dm->load("a:g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn(yama:Int) -> yama:None");

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

// type shadowed by local var

TEST_F(CompilationTests, IdentifierExpr_LocalVarShadowsType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn g() {}

fn f() {
    var g = -21;
    observeInt(g); // <- local var g shadows fn g
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto g = dm->load("a:g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, IdentifierExpr_LocalVarShadowsAnotherLocalVar) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// qualified id expr bypasses shadowing by type defined in Yama code

TEST_F(CompilationTests, IdentifierExpr_QualifiedIdExprBypassesShadowingByTypeDefinedInYamaCode) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;

fn observeInt(x: Int) {
    //
}

fn f() {
    a:observeInt(10); // bypasses shadowing
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// qualified id expr bypasses shadowing by parameter

TEST_F(CompilationTests, IdentifierExpr_QualifiedIdExprBypassesShadowingByParameter) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;

fn g(observeInt: Int) {
    a:observeInt(10); // bypasses shadowing
}

fn f() {
    g(0);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// qualified id expr bypasses shadowing by local var

TEST_F(CompilationTests, IdentifierExpr_QualifiedIdExprBypassesShadowingByLocalVar) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;

fn f() {
    var observeInt: Int = 0;
    a:observeInt(10); // bypasses shadowing
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// reference w/ qualifier which names an import which shares an unqualified name
// w/ a type in Yama code

TEST_F(CompilationTests, IdentifierExpr_QualifierNameAlsoUsedByType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import a: fns.abc;

fn a() {}

fn f() {
    a:observeInt(100); // <- id expr (ie. not a type spec)
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(100);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// reference something w/ id expr which is not declared until later in module

TEST_F(CompilationTests, IdentifierExpr_CanRefSomethingNotDeclaredUntilLaterInModule) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> g {
    return g;
}

fn g() {} // <- not declared until AFTER first reference to it

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    const auto g = dm->load("a:g"_str);
    ASSERT_TRUE(f);
    ASSERT_TRUE(g);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:g");
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal reference to undeclared name

TEST_F(CompilationTests, IdentifierExpr_Fail_RefsUndeclaredName) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = abc; // no decl named 'abc'
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal reference w/ undeclared qualifier

TEST_F(CompilationTests, IdentifierExpr_Fail_RefsWithUndeclaredQualifier) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = missing:T; // no import decl named 'missing:T'
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_qualifier), 1);
}

// illegal reference to local var not yet in scope (ie. the identifier expr is
// in the same block as the local var, but the local var isn't in scope yet)

TEST_F(CompilationTests, IdentifierExpr_Fail_RefsLocalVarNotYetInScope) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = abc; // 'abc' isn't in scope yet
    var abc = 10;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal reference to local var which has gone out-of-scope

TEST_F(CompilationTests, IdentifierExpr_Fail_RefsLocalVarWhichIsOutOfScope) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    if (true) {
        var abc = 10;
    }
    var a = abc; // 'abc' isn't in scope anymore
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// primitive type identifier expr is non-assignable

TEST_F(CompilationTests, IdentifierExpr_PrimitiveType_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    Int = Int; // 'Int' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// fn type identifier expr is non-assignable

TEST_F(CompilationTests, IdentifierExpr_FnType_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() {}

fn f() {
    g = g; // 'g' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// struct type identifier expr is non-assignable

TEST_F(CompilationTests, IdentifierExpr_StructType_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct Dummy {}

fn f() {
    Dummy = Dummy; // 'Dummy' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// parameter identifier expr is non-assignable

TEST_F(CompilationTests, IdentifierExpr_Param_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a: Int) {
    a = 10; // 'a' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// local var identifier expr is assignable

TEST_F(CompilationTests, IdentifierExpr_LocalVar_Assignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() -> Int {
    var a = -21;
    a = 10; // overwrite previous value
    observeInt(a);
    return a;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Int");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_int(10));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// primitive type identifier expr is constexpr

TEST_F(CompilationTests, IdentifierExpr_PrimitiveType_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(Int)' MUST be constexpr
    const(Int);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// fn type identifier expr is constexpr

TEST_F(CompilationTests, IdentifierExpr_FnType_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() {}

fn f() {
    // enforce 'const(g)' MUST be constexpr
    const(g);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// struct type identifier expr is constexpr

TEST_F(CompilationTests, IdentifierExpr_StructType_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct Dummy {}

fn f() {
    // enforce 'const(Dummy)' MUST be constexpr
    const(Dummy);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// parameter identifier expr is non-constexpr

TEST_F(CompilationTests, IdentifierExpr_Param_NonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a: Int) {
    const(a);
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}

// local var identifier expr is non-constexpr

TEST_F(CompilationTests, IdentifierExpr_LocalVar_NonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: Int = 0;
    const(a);
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}


// Int literal expr
//
//      - returns a Int value specified by a literal
// 
//      - non-assignable
//      - constexpr
// 
//      - illegal if Int literal overflows
//      - illegal if Int literal underflows

// basic usage

TEST_F(CompilationTests, IntLiteralExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, IntLiteralExpr_Fail_IfNumericOverflow) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    return 9_223_372_036_854_775_808;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_overflow), 1);
}

// illegal if Int literal underflows

TEST_F(CompilationTests, IntLiteralExpr_Fail_IfNumericUnderflow) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    return -9_223_372_036_854_775_809;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_underflow), 1);
}

// Int literal is non-assignable

TEST_F(CompilationTests, IntLiteralExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    1 = 2;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// Int literal is constexpr

TEST_F(CompilationTests, IntLiteralExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(10)' MUST be constexpr
    const(10);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// UInt literal expr
//
//      - returns a UInt value specified by a literal
// 
//      - non-assignable
//      - constexpr
// 
//      - illegal if UInt literal overflows

// basic usage

TEST_F(CompilationTests, UIntLiteralExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, UIntLiteralExpr_Fail_IfNumericOverflow) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Int {
    return 18_446_744_073_709_551_616u;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_numeric_overflow), 1);
}

// UInt literal is non-assignable

TEST_F(CompilationTests, UIntLiteralExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    1u = 2u;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// UInt literal is constexpr

TEST_F(CompilationTests, UIntLiteralExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(10u)' MUST be constexpr
    const(10u);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// TODO: not 100% sure how I feel about floats being constexpr, w/ me feeling that
//       later, when we add more complex operations in constexprs, that floats should
//       probably be excluded, restricted more-or-less only to literals for constants

// Float literal expr
//
//      - returns a Float value specified by a literal, including inf/nan keywords
// 
//      - non-assignable
//      - constexpr
// 
//      - out-of-bounds Float literals resolve to inf/-inf values

// basic usage

TEST_F(CompilationTests, FloatLiteralExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f0() -> Float { return 0.0; }
fn f1() -> Float { return 1.0; }
fn f2() -> Float { return 1_000_000.0; }
fn f3() -> Float { return .1324; }
fn f4() -> Float { return .1324e2; } // 13.24
fn f5() -> Float { return inf; }
fn f6() -> Float { return nan; }

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f0 = dm->load("a:f0"_str);
    const auto f1 = dm->load("a:f1"_str);
    const auto f2 = dm->load("a:f2"_str);
    const auto f3 = dm->load("a:f3"_str);
    const auto f4 = dm->load("a:f4"_str);
    const auto f5 = dm->load("a:f5"_str);
    const auto f6 = dm->load("a:f6"_str);
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
    ASSERT_EQ(f0->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f1->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f2->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f3->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f4->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f5->callsig().value().fmt(), "fn() -> yama:Float");
    ASSERT_EQ(f6->callsig().value().fmt(), "fn() -> yama:Float");

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
        EXPECT_TRUE(lhs.t == ctx->float_type());
        if (lhs.t == ctx->float_type()) EXPECT_DOUBLE_EQ(lhs.as_float(), rhs.as_float());

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
        EXPECT_TRUE(lhs.t == dm->float_type());
        if (lhs.t == dm->float_type()) EXPECT_TRUE(std::isnan(lhs.as_float()));

        ASSERT_TRUE(ctx->pop(1).good()); // cleanup

        // expected side effects
        EXPECT_EQ(sidefx.fmt(), sidefx_t{}.fmt());
    }
}

// inf if overflows

TEST_F(CompilationTests, FloatLiteralExpr_InfIfOverflows) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Float { return 1.7976931348723158e+308; } // should overflow to inf

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    const auto lhs = ctx->local(0).value();
    EXPECT_TRUE(lhs.t == dm->float_type());
    if (lhs.t == dm->float_type()) {
        EXPECT_TRUE(std::isinf(lhs.as_float()));
        EXPECT_FALSE(std::signbit(lhs.as_float()));
    }

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// -inf if underflows

TEST_F(CompilationTests, FloatLiteralExpr_NegativeInfIfUnderflows) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> Float { return -1.7976931348723158e+308; } // should underflow to -inf

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:Float");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    const auto lhs = ctx->local(0).value();
    EXPECT_TRUE(lhs.t == dm->float_type());
    if (lhs.t == dm->float_type()) {
        EXPECT_TRUE(std::isinf(lhs.as_float()));
        EXPECT_TRUE(std::signbit(lhs.as_float()));
    }

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// Float literal is non-assignable

TEST_F(CompilationTests, FloatLiteralExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    1.0 = 2.0;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// Float literal is constexpr

TEST_F(CompilationTests, FloatLiteralExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(3.14159)' MUST be constexpr
    const(3.14159);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// Bool literal expr
//
//      - returns a Bool value specified by a literal
// 
//      - non-assignable
//      - constexpr

// basic usage

TEST_F(CompilationTests, BoolLiteralExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    observeBool(true);
    observeBool(false);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

TEST_F(CompilationTests, BoolLiteralExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    true = false;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// Bool literal is constexpr

TEST_F(CompilationTests, BoolLiteralExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(true)' MUST be constexpr
    const(true);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// Char literal expr
//
//      - returns a Char value specified by a literal
// 
//      - non-assignable
//      - constexpr

// basic usage

TEST_F(CompilationTests, CharLiteralExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    // IMPORTANT: for Unicode input to work, we gotta use a UTF-8 string literal
    std::u8string txt0 = u8R"(

import fns.abc;

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
    std::string txt = taul::utf8_s(txt0);

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// illegal multi-codepoint char literal

TEST_F(CompilationTests, CharLiteralExpr_Fail_IllegalMultiCodepointCharLiteral) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = 'ab';
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_malformed_literal), 1);
}

// illegal Unicode, UTF-16 surrogate

TEST_F(CompilationTests, CharLiteralExpr_Fail_IllegalUnicode_UTF16Surrogate) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = '\ud8a2';
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_unicode), 1);
}

// illegal Unicode, out-of-bounds

TEST_F(CompilationTests, CharLiteralExpr_Fail_IllegalUnicode_OutOfBounds) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = '\U00110000';
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_illegal_unicode), 1);
}

// Char literal is non-assignable

TEST_F(CompilationTests, CharLiteralExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    'a' = 'b';
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// Char literal is constexpr

TEST_F(CompilationTests, CharLiteralExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const('y')' MUST be constexpr
    const('y');
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// call-like expr
//
//      - call-like exprs are homomorphic exprs for exprs of the form '<f>(<args>)'
//      
//      - selection rules:
// 
//          1) if <f> is a bound method expr, the expr is a 'bound method call expr'
//          2) if <f> is of a callable type, the expr is a 'call expr'
//          3) if <f> is type yama:Type, the expr is a 'default init expr'
//          4) invalid otherwise

// valid selection

TEST_F(CompilationTests, CallLikeExpr_Valid) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::g(self, x: Int) {
    observeInt(x);
}

fn f() {
    var a = A();
    a.g(4); // method call expr

    observeInt(100); // call expr

    observeChar(Char()); // default init expr (tested w/ call expr)
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(4);
    expected.observe_int(100);
    expected.observe_char(U'\0');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// invalid selection

TEST_F(CompilationTests, CallLikeExpr_Invalid) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    300(); // error!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}


// bound method call expr
//
//      - given form '<owner>.<method>(<args>)', bound method call exprs designates callsites
//        equivalent to writing 'T::<method>(<owner>, <args>)', where T is the the same
//        type as <owner>
//          * here '<owner>.<method>' is a bound method expr, which has a special semantic
//            relationship w/ bound method call expr
// 
//      - eval order:
// 
//          1) <owner> eval
//          2) <args> eval, going left-to-right
//          3) call is performed
// 
//      - non-assignable
//      - non-constexpr
// 
//      - illegal if '<owner>, <args>' has wrong number/types for a call to T::<method>

// basic usage

TEST_F(CompilationTests, BoundMethodCallExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::choose(self, which: Bool, a, b: Int) -> Int {
    if (which) {
        return a;
    }
    else {
        return b;
    }
}

fn f() {
    observeInt(A().choose(true, -3, 12));
    observeInt(A().choose(false, -3, 12));
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// evaluation order

TEST_F(CompilationTests, BoundMethodCallExpr_EvalOrder) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

// NOTE: this version differs quite a bit from the regular call expr one

import fns.abc;

fn foo() -> Int { observeInt(0); return 0; }
fn bar() -> Int { observeInt(1); return 0; }
fn baz() -> Int { observeInt(2); return 0; }

struct A {}

fn A::g(self, x, y, z: Int) {}

fn get_owner() -> A { observeChar('o'); return A(); }

fn f() {
    // Yama has well defined eval order

    get_owner().g(foo(), bar(), baz());
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'o');
    expected.observe_int(0);
    expected.observe_int(1);
    expected.observe_int(2);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal call due to too many args

TEST_F(CompilationTests, BoundMethodCallExpr_Fail_Args_TooMany) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self, a: Int) {}

fn f() {
    A().g('a', 'b');
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to too few args

TEST_F(CompilationTests, BoundMethodCallExpr_Fail_Args_TooFew) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self, a: Int) {}

fn f() {
    A().g();
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to incorrect arg type(s) (ie. type mismatch)

TEST_F(CompilationTests, BoundMethodCallExpr_Fail_Args_TypeMismatch) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self, a: Int) {}

fn f() {
    A().g('a');
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}

// bound method call expr is non-assignable

TEST_F(CompilationTests, BoundMethodCallExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self, a, b: Int) -> Int { return 0; }

fn f() {
    A().g(3, 2) = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// bound method call expr is non-constexpr

TEST_F(CompilationTests, BoundMethodCallExpr_NonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self, a, b: Int) -> Int { return 0; }

fn f() {
    const(A().g(3, 2));
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}


// call expr
// 
//      - given form '<callobj>(<args>)', call exprs designate callsites, w/ <callobj>
//        being the call object, and <args> being the args to it, it returns the result
//        of the call
// 
//      - eval order:
// 
//          1) <callobj> eval
//          2) <args> eval, going left-to-right
//          3) call is performed
// 
//      - non-assignable
//      - non-constexpr
// 
//      - illegal if <args> has wrong number/types for a call to <callobj>

// basic usage

TEST_F(CompilationTests, CallExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

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

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// evaluation order

TEST_F(CompilationTests, CallExpr_EvalOrder) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn get_callobj() -> g { observeChar('c'); return g; }

fn foo() -> Int { observeInt(0); return 0; }
fn bar() -> Int { observeInt(1); return 0; }
fn baz() -> Int { observeInt(2); return 0; }

fn g(a, b, c: Int) -> Int { return 0; }

fn f() {
    // Yama has well defined eval order

    get_callobj()(foo(), g(foo(), bar(), baz()), bar());
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// this isn't needed to formally define call expr behaviour, but is here for
// comprehensiveness

TEST_F(CompilationTests, CallExpr_Nesting) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn foo(x: Int) -> foo { observeInt(x); return foo; }

fn f() {
    foo(10)(20)(30)(40)(50)(60);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

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

// illegal call due to too many args

TEST_F(CompilationTests, CallExpr_Fail_Args_TooMany) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g('a', 'b');
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to too few args

TEST_F(CompilationTests, CallExpr_Fail_Args_TooFew) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g();
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal call due to incorrect arg type(s) (ie. type mismatch)

TEST_F(CompilationTests, CallExpr_Fail_Args_TypeMismatch) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g(a: Int) {}

fn f() {
    g('a');
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_type_mismatch), 1);
}

// call expr is non-assignable

TEST_F(CompilationTests, CallExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g(a, b: Int) -> Int { return 0; }

fn f() {
    g(3, 2) = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// call expr is non-constexpr

TEST_F(CompilationTests, CallExpr_NonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g(a, b: Int) -> Int { return 0; }

fn f() {
    const(g(3, 2));
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}


// type member access expr
//
//      - type member access exprs are homomorphic exprs for exprs of the form '<owner>::<member>'
//      
//      - selection rules:
// 
//          1) if <owner> is yama:Type crvalue, and <member> is a method of type <owner>,
//             then expr is an 'unbound method expr'
//          2) invalid otherwise

// valid selection

TEST_F(CompilationTests, TypeMemberAccessExpr_Valid) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::g() {
    observeInt(100);
}

fn f() {
    A::g(); // unbound method expr (tested w/ call expr)
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(100);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// invalid selection

TEST_F(CompilationTests, TypeMemberAccessExpr_Invalid) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    const(10)::abc; // error!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

TEST_F(CompilationTests, TypeMemberAccessExpr_Invalid_OwnerIsTypeType_ButUnknownMember) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn f() {
    A::missing; // error! A is valid Type crvalue, but no member 'missing'!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

TEST_F(CompilationTests, TypeMemberAccessExpr_Invalid_OwnerIsTypeType_ButNotCRValue) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() {
    var x = A();
    x::g; // error! x is valid Type rvalue, but is not crvalue!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}


// unbound method expr
// 
//      - given form '<type>::<method>', unbound method exprs eval to crvalues
//        of the <method> of <type> specified (where <type> is a yama:Type crvalue)
// 
//      - non-assignable
//      - constexpr

// basic usage

TEST_F(CompilationTests, UnboundMethodExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() -> A::g {
    return A::g;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto A_g = dm->load("a:A::g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(A_g);
    ASSERT_TRUE(f);

    ASSERT_EQ(A_g->kind(), yama::kind::method);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(A_g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:A::g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*A_g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// unbound method expr is non-assignable

TEST_F(CompilationTests, UnboundMethodExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() {
    A::g = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// unbound method expr is constexpr

TEST_F(CompilationTests, UnboundMethodExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() {
    // enforce 'A::g' MUST be constexpr
    const(A::g);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}


// object member access expr
//
//      - object member access exprs are homomorphic exprs for exprs of the form '<owner>.<member>'
//      
//      - selection rules:
// 
//          1) if <owner> is a rvalue, and <member> is a method of the type of object <owner>,
//             then expr is a 'bound method expr'
//          2) invalid otherwise

// valid selection

TEST_F(CompilationTests, ObjectMemberAccessExpr_Valid) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct A {}

fn A::g(self) {
    observeInt(100);
}

fn f() {
    var a = A();
    a.g(); // bound method expr (tested w/ bound method call expr)
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(100);
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// invalid selection

TEST_F(CompilationTests, ObjectMemberAccessExpr_Invalid_UnknownMember) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn f() {
    var a = A();
    a.missing; // error! unknown member 'missing'
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}


// bound method expr
//
//      - given form '<object>.<method>', bound method exprs eval to rvalues
//        of the <method> of the type of <object> specified (where <object> is
//        a rvalue)
//          * TODO: later on, when we are able to have fn objects which can store
//                  references to owner objects, we want to revise bound method
//                  exprs to return these instead
// 
//      - non-assignable
//      - non-constexpr
//
//      - illegal if <method> does not have any parameters
//      - illegal if first parameter of <method> is not same type as <owner>


// basic usage

TEST_F(CompilationTests, BoundMethodExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self) {}

fn f() -> A::g {
    var a = A();
    return a.g;
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto A_g = dm->load("a:A::g"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(A_g);
    ASSERT_TRUE(f);

    ASSERT_EQ(A_g->kind(), yama::kind::method);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(A_g->callsig().value().fmt(), "fn(a:A) -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> a:A::g");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*A_g));

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// bound method expr is non-assignable

TEST_F(CompilationTests, BoundMethodExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self) {}

fn f() {
    var a = A();
    a.g = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// bound method expr is non-constexpr

TEST_F(CompilationTests, BoundMethodExpr_NonConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(self) {}

fn f() {
    var a = A();
    const(a.g); // error! a.g is not constexpr
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}

// illegal if <method> does not have any parameters

TEST_F(CompilationTests, BoundMethodExpr_Fail_NoParams) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g() {}

fn f() {
    var a = A();
    a.g; // error! g has no first param
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}

// illegal if first parameter of <method> is not same type as <owner>

TEST_F(CompilationTests, BoundMethodExpr_Fail_FirstParamNotOwnerType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct A {}

fn A::g(invalid_self: Int) {}

fn f() {
    var a = A();
    a.g; // error! g first param is not type A, and so cannot be used as *self* param
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
}


static_assert(yama::ptypes == 7); // reminder
static_assert(yama::kinds == 4); // reminder

// default init expr
// 
//      - given form '<type>()', where <type> is a crvalue of type yama:Type,
//        default init expr evals to a default initialized object of the
//        type specified by <type>
//          * the default values of types are specified in 'general'
//
//      - non-assignable
// 
//      - constexpr status:
// 
//          primitive type      constexpr
//          fn type             n/a
//          struct type         non-constexpr
//
//      - illegal if <type> is not constexpr
//      - illegal if arg count is >=1

// basic usage

TEST_F(CompilationTests, DefaultInitExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

struct SomeStruct {}

fn get_SomeStruct() -> SomeStruct {
    return SomeStruct();
}

fn f() {
    observeNone(None());
    observeInt(Int());
    observeUInt(UInt());
    observeFloat(Float());
    observeBool(Bool());
    observeChar(Char());
    observeType(Type());
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto SomeStruct = dm->load("a:SomeStruct"_str);
    const auto get_SomeStruct = dm->load("a:get_SomeStruct"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(SomeStruct);
    ASSERT_TRUE(get_SomeStruct);
    ASSERT_TRUE(f);

    ASSERT_EQ(SomeStruct->kind(), yama::kind::struct0);
    ASSERT_EQ(get_SomeStruct->kind(), yama::kind::function);
    ASSERT_EQ(get_SomeStruct->callsig().value().fmt(), "fn() -> a:SomeStruct");
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*get_SomeStruct).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return values
    EXPECT_EQ(ctx->local(0).value().t, *SomeStruct);
    EXPECT_EQ(ctx->local(1).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_none();
    expected.observe_int(0);
    expected.observe_uint(0);
    expected.observe_float(0.0);
    expected.observe_bool(false);
    expected.observe_char(U'\0');
    expected.observe_type(ctx->none_type());
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal if x is not constexpr

TEST_F(CompilationTests, DefaultInitExpr_Fail_ExprXIsNotAConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = Int;
    var b = a(); // error! 'a' is Type, but it's not constexpr!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}

// illegal if arg count >=1

TEST_F(CompilationTests, DefaultInitExpr_Fail_ArgCountIsNonZero) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    Int(10); // error! cannot have '10'
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// default init expr is non-assignable

TEST_F(CompilationTests, DefaultInitExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    Int() = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// default init expr is constexpr, if primitive type

TEST_F(CompilationTests, DefaultInitExpr_ConstExpr_IfPrimitiveType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(<TYPE>())' MUST be constexpr
    const(None());
    const(Int());
    const(UInt());
    const(Float());
    const(Bool());
    const(Char());
    const(Type());
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// default init expr is non-constexpr, if struct type

TEST_F(CompilationTests, DefaultInitExpr_NonConstExpr_IfStructType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

struct G {}

fn f() {
    const(G());
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}


// constexpr guarantee expr
//
//      - given form 'const(<x>)', where <x> is a crvalue, the constexpr
//        guarantee expr evals <x>, returning its result
//          * this expr exists to enforce that an expr must be constexpr
// 
//      - non-assignable
//      - constexpr
//
//      - illegal if x is not constexpr
//
//      - illegal if multiple or zero args are provided for x

// basic usage

TEST_F(CompilationTests, ConstExprGuaranteeExpr_BasicUsage) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn g() {}

fn f0() {
    observeInt(const(10));
    observeUInt(const(10u));
    observeBool(const(true));
    observeChar(const('y'));
}

fn f1() -> g {
    return const(g);
}

fn f2() -> Type {
    return const(Int);
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto g = dm->load("a:g"_str);
    const auto f0 = dm->load("a:f0"_str);
    const auto f1 = dm->load("a:f1"_str);
    const auto f2 = dm->load("a:f2"_str);
    ASSERT_TRUE(g);
    ASSERT_TRUE(f0);
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);

    ASSERT_EQ(g->kind(), yama::kind::function);
    ASSERT_EQ(f0->kind(), yama::kind::function);
    ASSERT_EQ(f1->kind(), yama::kind::function);
    ASSERT_EQ(f2->kind(), yama::kind::function);
    ASSERT_EQ(g->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f0->callsig().value().fmt(), "fn() -> yama:None");
    ASSERT_EQ(f1->callsig().value().fmt(), "fn() -> a:g");
    ASSERT_EQ(f2->callsig().value().fmt(), "fn() -> yama:Type");

    ASSERT_TRUE(ctx->push_fn(*f0).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*f1).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());
    ASSERT_TRUE(ctx->push_fn(*f2).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());
    EXPECT_EQ(ctx->local(1).value(), ctx->new_fn(*g).value());
    EXPECT_EQ(ctx->local(2).value(), ctx->new_type(ctx->int_type()));

    // expected side effects
    sidefx_t expected{};
    expected.observe_int(10);
    expected.observe_uint(10);
    expected.observe_bool(true);
    expected.observe_char(U'y');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal due to non-constexpr subexpr

TEST_F(CompilationTests, ConstExprGuaranteeExpr_Fail_SubExprNotAConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = 10;
    const(a); // expr 'a' is not constexpr!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonconstexpr_expr), 1);
}

// illegal due to too many args

TEST_F(CompilationTests, ConstExprGuaranteeExpr_Fail_Args_TooMany) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    const(3, 10);
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// illegal due to too few args

TEST_F(CompilationTests, ConstExprGuaranteeExpr_Fail_Args_TooFew) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    const();
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_wrong_arg_count), 1);
}

// constexpr guarantee expr is non-assignable

TEST_F(CompilationTests, ConstExprGuaranteeExpr_NonAssignable) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // can't really test 'const(x)' w/ an assignable x as it's a crvalue,
    // not an lvalue
    const(3) = 4;
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// constexpr guarantee expr is constexpr

TEST_F(CompilationTests, ConstExprGuaranteeExpr_ConstExpr) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    // enforce 'const(10)' MUST be constexpr
    var a: Int = const(const(10));
}

)";

    const auto result = perform_compile(txt);
    ASSERT_TRUE(result);

    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(f);

    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

