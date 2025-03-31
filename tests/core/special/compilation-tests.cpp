

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
//                w/ the exact wording of the header, just so long as the overall section test is
//                properly comprehensive
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

        void observe_misc(std::string_view msg) { seq += std::format("misc {}\n", msg); }
    };

    sidefx_t sidefx; // global so our Yama functions can reference it


    // our Yama functions for side-effects + misc

    auto consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str)
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:UInt"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Bool"_str)
        .add_primitive_type("yama:Char"_str);

    yama::type_info observeNone_info{
        .unqualified_name = "observeNone"_str,
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
        .unqualified_name = "observeInt"_str,
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
        .unqualified_name = "observeUInt"_str,
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
        .unqualified_name = "observeFloat"_str,
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
        .unqualified_name = "observeBool"_str,
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
        .unqualified_name = "observeChar"_str,
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
        .unqualified_name = "doPanic"_str,
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
        .unqualified_name = "doIncr"_str,
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
        .unqualified_name = "isEqInt"_str,
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
        .unqualified_name = "isEqChar"_str,
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
        .unqualified_name = "isNotEqInt"_str,
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
                return yama::make_res<yama::module_info>(make_fns());
            }
            // .bad is used to test 
            else if (relative_path == ".bad"_str) {
                yama::module_factory mf{};
                mf.add_function_type(
                    "f"_str,
                    yama::const_table_info{},
                    yama::make_callsig_info({}, 10'000), // <- return type invalid! so .bad is also invalid!
                    1,
                    yama::noop_call_fn);
                return mf.done();
            }
            else return std::nullopt;
        }
    };


    auto acknowledge_consts =
        yama::const_table_info()
        .add_primitive_type("yama:None"_str);

    yama::type_info acknowledge_info{
        .unqualified_name = "acknowledge"_str,
        .consts = acknowledge_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 0),
            .call_fn =
                [](yama::context& ctx) {
                    sidefx.observe_misc("ack");
                    if (ctx.push_none().bad()) return;
                    if (ctx.ret(0).bad()) return;
                },
            .max_locals = 1,
        },
    };

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
                mf.add_type(yama::type_info(acknowledge_info));
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
//      - functions are callable (obviously)
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
    // no 'yama' dep to use for implicit import dir

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
    // 'yama' dep to use for implicit import dir being *wrong*

    std::string txt = R"(

fn f() {}

)";

    our_parcel->our_src = txt;
    ASSERT_FALSE(dm->import("bad"_str).has_value());

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_env), 1);
}


// IMPORTANT: 'dir' is short for 'directive'

// import dir
//
//      - the compilation defines an 'import set' of modules which have been
//        imported such that their contents are made available
// 
//          - if two imported modules have similarly named types, no issue arises
//            from them coexisting (just so long as they're not referenced unqualified)
// 
//      - import dirs add new modules to the import set
// 
//          - the 'import path specifiers' used in import dirs specify the module
//            imported
// 
//              - import path specifiers are relative to the parcel env of the parcel
//                of the compiling module
// 
//          - compilation must be able to succeed at importing a module via an import
//            dir's import path specifier to add to the import set
// 
//              - if compilation succeeds, the modules imported during compilation
//                become part of the module import state observable by end-user
// 
//          - redundant import dirs are tolerated
// 
//          - import dirs importing the compiling module itself are tolerated
// 
//      - compilation defines an implicit import dir which imports the 'yama' module
//
//          - this module is the root of the 'yama' dep of the compiling module's
//            parcel, which is expected to map to the special parcel defining
//            builtin Yama types
//
//          - due to rules defined above in 'general', this module is guaranteed to
//            be available, and its contents should be as expected
//
//      - import dirs may exist only in the global block
//
//      - import dirs may exist only prior to the first type decl
// 
//      - import dirs may induce multi-source compilation
//
//          - import dirs within these additional translation units may induce
//            further multi-source compilation

// explicit import dir, w/ module imported from dependency

TEST_F(CompilationTests, ImportDir_ExplicitImportDir_ModuleImportedFromDep) {
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

// explicit import dir, w/ module imported from same parcel as compiling module

TEST_F(CompilationTests, ImportDir_ExplicitImportDir_ModuleImportedFromSameParcelAsCompilingModule) {
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

// explicit import dir, imported modules share types w/ common unqualified name

TEST_F(CompilationTests, ImportDir_ExplicitImportDir_ImportedModulesShareTypesWithCommonUnqualifiedName) {
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

// implicit yama import dir

TEST_F(CompilationTests, ImportDir_ImplicitYamaImportDir) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

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

// redundant import dirs are tolerated

TEST_F(CompilationTests, ImportDir_RedundantImportDirsAreTolerated) {
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

// redundant import dirs are tolerated, for implicit yama import dir

TEST_F(CompilationTests, ImportDir_RedundantImportDirsAreTolerated_ForImplicitYamaImportDir) {
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

// import dirs importing the compiling module itself are tolerated

TEST_F(CompilationTests, ImportDir_ImportDirsImportingTheCompilingModuleItselfAreTolerated) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import self;
import self;
import self;
import self;
import self;

fn f() -> Int {
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

// multi-source compilation

TEST_F(CompilationTests, ImportDir_MultiSourceCompilation) {
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

// multi-source compilation where compiling modules added to compilation by import dirs
// inducing their compilation in turn induce the compilation of yet more modules

TEST_F(CompilationTests, ImportDir_MultiSourceCompilation_IndirectCompilingModuleAddition) {
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

TEST_F(CompilationTests, ImportDir_MultiSourceCompilation_MultiParcel) {
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

TEST_F(CompilationTests, ImportDir_MultiSourceCompilation_TolerateCompilingModuleImportGraphCycles) {
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

// illegal to import non-existent module

TEST_F(CompilationTests, ImportDir_Fail_ImportNonExistentModule) {
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

TEST_F(CompilationTests, ImportDir_Fail_ImportInvalidModule) {
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

// illegal for import dir to exist in local scope

TEST_F(CompilationTests, ImportDir_Fail_ImportDirExistsInLocalScope) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    import fns.abc; // error! illegal in local scope!
}

)";

    ASSERT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_misplaced_import), 1);
}

TEST_F(CompilationTests, ImportDir_Fail_ImportDirExistsAfterFirstTypeDecl) {
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

// multi-source compilation failure (including w/ the error arising in another parcel!)

TEST_F(CompilationTests, ImportDir_Fail_MultiSourceCompilationFailure) {
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
//      - 'decls' bind name identifiers to types, parameters and local vars
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
// 
//      - within the global block are contained all types loaded from modules imported
//        via import dirs
//      - within the global block, types are differentiated by parcel env 'fullname'
//          * this means that it's allowed for types defined in the Yama code to share
//            unqualified names w/ extern types (as they shouldn't conflict)
//          * unqualified name conflicts should only arise on reference
// 
//      - decls may 'shadow' one another
// 
//          - decls in child blocks may 'shadow' ones in ancestoral blocks by
//            using the same name as them
// 
//          - given an extern type and a type defined in Yama code w/ the same
//            unqualified name, unqualified name access of this name will select the
//            type defined in Yama code, shadowing the extern type
// 
//      - the different scopes decls have are as follows:
// 
//          - type decls for extern types exist in the global block, w/ their scope
//            beginning/ending at the start/end of this block
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
//            being deferred to tests for 'type specifiers' and 'identifier exprs'

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

// test to ensure impl properly impls local var scope not starting until after the
// decl itself, w/ this meaning that the init exprs of the local var cannot reference
// itself

TEST_F(CompilationTests, DeclAndScope_LocalVarScopeBeginsWhenExpected) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: Int = a; // <- local var 'a' hasn't come into scope yet!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal to declare a two or more types in global scope w/ a common name

TEST_F(CompilationTests, DeclAndScope_Fail_IfDeclTypeWithNameOfTypeAlreadyTaken_ByAnotherTypeInYamaCode) {
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

TEST_F(CompilationTests, DeclAndScope_Fail_IfTwoParamsShareName) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a, a: Int) {}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_name_conflict), 1);
}

// illegal to declare a local var in function body block w/ the name
// of a parameter

TEST_F(CompilationTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfParam) {
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

TEST_F(CompilationTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfAnotherLocalVar_BothInFnBodyBlock) {
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

TEST_F(CompilationTests, DeclAndScope_Fail_IfDeclareLocalVar_WithNameOfAnotherLocalVar_BothInNestedBlock) {
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
//      - specifies Yama types by name
// 
//      - type specifiers exist relative to some block, and their ability to reference
//        decls respects the scope and shadowing rules of decls

// specify extern type

TEST_F(CompilationTests, TypeSpecifier_SpecifyExternType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn f() {
    var a: Int = 10; // 'Int' specifies extern yama:Int
    observeInt(a); // 'observeInt' specifies extern fns.abc:observeInt
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

// specify type in Yama code

TEST_F(CompilationTests, TypeSpecifier_SpecifyTypeInYamaCode) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() {}

fn f() -> g {
    return g;
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
    EXPECT_EQ(ctx->local(0).value(), ctx->new_fn(*g).value());

    // expected side effects
    sidefx_t expected{};
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// specify type in Yama code which shadows an extern type

TEST_F(CompilationTests, TypeSpecifier_SpecifyTypeInYamaCode_WhichShadowsExternType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc;

fn observeInt(x: Char) { // shadows fns.abc:observeInt
    observeChar(x); // will observe fns.abc:observeChar behaviour
}

fn f() {
    observeInt('a'); // ref our observeInt
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
    ASSERT_EQ(observeInt->callsig().value().fmt(), "fn(yama:Char) -> yama:None");
    ASSERT_EQ(f->callsig().value().fmt(), "fn() -> yama:None");

    ASSERT_TRUE(ctx->push_fn(*f).good());
    ASSERT_TRUE(ctx->call(1, yama::newtop).good());

    // expected return value
    EXPECT_EQ(ctx->local(0).value(), ctx->new_none());

    // expected side effects
    sidefx_t expected{};
    expected.observe_char(U'a');
    EXPECT_EQ(sidefx.fmt(), expected.fmt());
}

// illegal for type spec to specify non-type

TEST_F(CompilationTests, TypeSpecifier_Fail_IfSpecifyUndeclaredName) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a: T; // error! no type named T
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal ambiguous unqualified name type specify (between two or more extern types)

TEST_F(CompilationTests, TypeSpecifier_Fail_IfAmbiguousUnqualifiedNameTypeSpecify_BetweenTwoOrMoreExternTypes) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

import fns.abc; // imports 'observeInt'
import fns2.abc; // imports 'observeInt'

fn f() {
    observeInt(10); // error! could be fns.abc:observeInt or fns2.abc:observeInt, ambiguous!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_ambiguous_name), 1);
}

// cannot (unqualified name) reference type which has been shadowed by parameter

TEST_F(CompilationTests, TypeSpecifier_Fail_IfTypeShadowedByParam) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(Int: Float) { // <- shadows Int type
    var a: Int; // error! Int refers to param, not yama:Int!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_a_type), 1);
}

// cannot (unqualified name) reference type which has been shadowed by local var

TEST_F(CompilationTests, TypeSpecifier_Fail_IfTypeShadowedByLocalVar) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var Int = 10; // shadows Int type (ironically)
    var a: Int; // error! Int refers to local var, not yama:Int!
}

)";

    EXPECT_FALSE(perform_compile(txt));

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

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonlocal_var), 1);
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
//          * illegal in MVP version of Yama
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

    EXPECT_EQ(dbg->count(yama::dsignal::compile_local_fn), 1);
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

TEST_F(CompilationTests, IfStmt_Fail_IfTypeMismatch) {
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

TEST_F(CompilationTests, IdentifierExpr_AccessValueOf_Fn) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() -> g {
    return g;
}

fn g() {} // make sure impl can handle fn not declared until AFTER first use!

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

// access value of parameter reference

TEST_F(CompilationTests, IdentifierExpr_AccessValueOf_Param) {
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

// access value of local var reference

TEST_F(CompilationTests, IdentifierExpr_AccessValueOf_LocalVar) {
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

// extern fn shadowed by fn defined in Yama code

TEST_F(CompilationTests, IdentifierExpr_FnDefinedInYamaCodeShadowsExternFn) {
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

// function shadowed by parameter

TEST_F(CompilationTests, IdentifierExpr_ParamShadowsFn) {
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

// function shadowed by local var

TEST_F(CompilationTests, IdentifierExpr_LocalVarShadowsFn) {
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

// illegal reference to non-function type

TEST_F(CompilationTests, IdentifierExpr_Fail_IfRefersToNonFnType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = Int; // 'Int' is not a fn type
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_not_an_expr), 1);
}

// illegal reference to undeclared name

TEST_F(CompilationTests, IdentifierExpr_Fail_IfRefersToUndeclaredName) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    var a = abc; // no decl named 'abc'
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_undeclared_name), 1);
}

// illegal reference to local var not yet in scope (ie. the identifier expr is
// in the same block as the local var, but the local var isn't in scope yet)

TEST_F(CompilationTests, IdentifierExpr_Fail_IfRefersToLocalVarNotYetInScope) {
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

TEST_F(CompilationTests, IdentifierExpr_Fail_IfRefersToLocalVarWhichIsOutOfScope) {
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

// identifier expr is non-assignable, if function reference

TEST_F(CompilationTests, IdentifierExpr_NonAssignable_IfFn) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn g() {}

fn f() {
    // below uses '10', as we can't really give the assigned value a
    // *correct type*, as that would imply fn decl 'g' has a notion
    // of itself having an object type, which it doesn't really...

    g = 10; // 'g' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// identifier expr is non-assignable, if parameter reference

TEST_F(CompilationTests, IdentifierExpr_NonAssignable_IfParam) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f(a: Int) {
    a = 10; // 'a' is not assignable
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_nonassignable_expr), 1);
}

// identifier expr is assignable, if local var reference

TEST_F(CompilationTests, IdentifierExpr_Assignable_IfLocalVar) {
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


// Int literal expr
//
//      - returns a Int value specified by a literal
// 
//      - non-assignable
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


// UInt literal expr
//
//      - returns a UInt value specified by a literal
// 
//      - non-assignable
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


// Float literal expr
//
//      - returns a Float value specified by a literal, including inf/nan keywords
// 
//      - non-assignable
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


// Bool literal expr
//
//      - returns a Bool value specified by a literal
// 
//      - non-assignable

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


// Char literal expr
//
//      - returns a Char value specified by a literal
// 
//      - non-assignable

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

    const auto choose = dm->load("a:choose"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(choose);
    ASSERT_TRUE(f);

    ASSERT_EQ(choose->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(choose->callsig().value().fmt(), "fn(yama:Bool, yama:Int, yama:Int) -> yama:Int");
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

// callobj/params evaluation order

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

    const auto get_callobj = dm->load("a:get_callobj"_str);
    const auto foo = dm->load("a:foo"_str);
    const auto bar = dm->load("a:bar"_str);
    const auto baz = dm->load("a:baz"_str);
    const auto g = dm->load("a:g"_str);
    const auto f = dm->load("a:f"_str);
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
    ASSERT_EQ(get_callobj->callsig().value().fmt(), "fn() -> a:g");
    ASSERT_EQ(foo->callsig().value().fmt(), "fn() -> yama:Int");
    ASSERT_EQ(bar->callsig().value().fmt(), "fn() -> yama:Int");
    ASSERT_EQ(baz->callsig().value().fmt(), "fn() -> yama:Int");
    ASSERT_EQ(g->callsig().value().fmt(), "fn(yama:Int, yama:Int, yama:Int) -> yama:Int");
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

    const auto foo = dm->load("a:foo"_str);
    const auto f = dm->load("a:f"_str);
    ASSERT_TRUE(foo);
    ASSERT_TRUE(f);

    ASSERT_EQ(foo->kind(), yama::kind::function);
    ASSERT_EQ(f->kind(), yama::kind::function);
    ASSERT_EQ(foo->callsig().value().fmt(), "fn(yama:Int) -> a:foo");
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

// illegal call due to call object is non-callable type

TEST_F(CompilationTests, CallExpr_Fail_CallObjIsNonCallableType) {
    ASSERT_TRUE(ready);

    std::string txt = R"(

fn f() {
    300(); // <- Int is non-callable!
}

)";

    EXPECT_FALSE(perform_compile(txt));

    EXPECT_EQ(dbg->count(yama::dsignal::compile_invalid_operation), 1);
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

