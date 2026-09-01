

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>
#include <yama++/print.h>

#include "../../utils/utils.h"
#include "../../utils/SideFxHistory.h"


using namespace ym;


namespace {
    class Compiler : public testing::Test {
    public:
        ErrCounter err;
        Scoped<YmDm> dm;
        Scoped<YmCtx> ctx;


        Compiler() :
            dm(makeScoped<YmDm>()),
            ctx(makeScoped<YmCtx>(dm.get())) {
        }


        void bindSource(const std::string& path, const std::string& code) {
            ymDm_BindSourceCode(dm.get(), path.c_str(), code.c_str(), code.length());
        }

        YmParcel* compile(const std::string& initial) {
            return ymCtx_Import(ctx.get(), initial.c_str());
        }

        // TODO: At some point maybe look into making these 'callSig(s)' part of
        //		 the Yama C API frontend.

        static std::optional<std::string> callSigOf(YmType* t) {
            if (!t) {
                return std::nullopt;
            }
            std::string argList{}, returns{};
            bool hitFirstNamedParam = false;
            for (YmParams i = 0; i < ymType_Params(t, true); i++) {
                if (!argList.empty()) {
                    argList += ", ";
                }
                auto named = ymType_ParamCategory(t, i) == YmParamCategory_Named;
                if (named && !hitFirstNamedParam) {
                    hitFirstNamedParam = true;
                    argList += "named: ";
                }
                argList += ymType_Fullname(ymType_ParamType(t, i));
            }
            returns = ymType_Fullname(ymType_ReturnType(t));
            return std::format("fn({}) -> {}", argList, returns);
        }

        YmType* expectType(
            const std::string& fln,
            YmKind k,
            const std::optional<std::string>& callSig = std::nullopt) {
            if (auto t = ymCtx_Load(ctx.get(), fln.c_str())) {
                if (ymType_Kind(t) != k) {
                    ADD_FAILURE()
                        << "Compiler::expectType (" << fln << ", " << ymKind_Fmt(k) << ", ~): "
                        << "Kind is " << ymKind_Fmt(ymType_Kind(t)) << ", but expected " << ymKind_Fmt(k) << "!";
                    return nullptr;
                }
                if (auto actualCallSig = callSigOf(t); callSig && actualCallSig != *callSig) {
                    ADD_FAILURE()
                        << "Compiler::expectType (" << fln << ", " << ymKind_Fmt(k) << ", ~): "
                        << "Callsig is " << actualCallSig.value_or("**NO-CALLSIG**") << ", but expected " << *callSig << "!";
                    return nullptr;
                }
                return t;
            }
            else {
                ADD_FAILURE()
                    << "Compiler::expectType (" << fln << ", " << ymKind_Fmt(k) << ", ~): "
                    << fln << " didn't load!";
                return nullptr;
            }
        }
        YmType* expectStruct(const std::string& fln) {
            return expectType(fln, YmKind_Struct);
        }
        YmType* expectProtocol(const std::string& fln) {
            return expectType(fln, YmKind_Protocol);
        }
        YmType* expectFn(const std::string& fln, const std::string& callSig) {
            return expectType(fln, YmKind_Fn, callSig);
        }

        bool expectLocal(YmLocal where, const Val& expects) {
            if (auto local = ymCtx_Local(ctx.get(), where, YM_BORROW)) {
                auto v = toVal(local);
                if (!compare(v, expects)) {
                    ADD_FAILURE() << "Compiler::expectLocal (" << where << ", ~): " << fmt(v) << " != " << fmt(expects);
                    return false;
                }
                return true;
            }
            ADD_FAILURE() << "Compiler::expectLocal (" << where << ", ~): **OUT-OF-BOUNDS** != " << fmt(expects);
            return false;
        }
        bool expectSideFx(const SideFxHistory& expects) {
            return sidefx.expectEq(expects);
        }


    protected:
        void SetUp() override {
            err.setupCallbackForThisThread();
            setupSideFxParcel(dm.get());
            clearSideFx();
        }
        void TearDown() override {
            //
        }
    };
}


TEST_F(Compiler, Empty) {
    bindSource("main", R"(

// Empty.

)");

    auto p = compile("main");
    ASSERT_TRUE(p);

    EXPECT_EQ(ymParcel_Count(p), 0);
}

TEST_F(Compiler, MultipleRoundsOfCompilation) {
    // I once got a nasty memory corruption issue from my impl not being able to handle
    // multiple rounds of compilation due to failing to cleanup properly between each,
    // so I decided to create this test to cover this behaviour.

    bindSource("main1", R"(

import "sidefx";

fn f() {
    var temp: Int = 100;
    observeInt(temp);
}

)");
    bindSource("main2", R"(

import "sidefx";

fn f() {
    var temp: Int = 41;
    observeInt(temp);
}

)");
    bindSource("main3", R"(

import "sidefx";

fn f() {
    var temp: Int = 1_001;
    observeInt(temp);
}

)");

    ASSERT_TRUE(compile("main1"));
    ASSERT_TRUE(compile("main2"));
    ASSERT_TRUE(compile("main3"));
    auto f1 = expectFn("main1:f", "fn() -> yama:None");
    auto f2 = expectFn("main2:f", "fn() -> yama:None");
    auto f3 = expectFn("main3:f", "fn() -> yama:None");
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    ASSERT_TRUE(f3);

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f1, 0, "", YM_PUSH));
    ASSERT_TRUE(ymCtx_Call(ctx.get(), f2, 0, "", YM_PUSH));
    ASSERT_TRUE(ymCtx_Call(ctx.get(), f3, 0, "", YM_PUSH));
    expectLocal(-3, NoneVal{});
    expectLocal(-2, NoneVal{});
    expectLocal(-1, NoneVal{});

    SideFxHistory hist{};
    hist.observeInt(100);
    hist.observeInt(41);
    hist.observeInt(1001);
    expectSideFx(hist);
}

TEST_F(Compiler, LexicalError) {
    bindSource("main", R"(

fn f() {
    $$$$$$ // <- Lexical error.
}

)");

    // I originally tried having seperate lexical/syntactic error values,
    // but I found that it's actually pretty hard to reliably differentiate
    // between purely lexical and purely syntactic errors in our LL(1) system.

    ASSERT_FALSE(compile("main"));
    EXPECT_GE(err[YmErrCode_SyntaxError], 1);
}

TEST_F(Compiler, SyntaxError) {
    bindSource("main", R"(

fn f() {
    fn // <- Lexically sound, but syntax error.
}

)");

    ASSERT_FALSE(compile("main"));
    EXPECT_GE(err[YmErrCode_SyntaxError], 1);
}

TEST_F(Compiler, Concepts_YamaParcel) {
    // The 'yama' parcel is always implicitly imported and thus available
    // for reference in Yama code.

    bindSource("main", R"(

import "sidefx";

fn f() {
    var a: yama:Int = 31; // 'yama' can be referenced w/out explicit import.
    observeInt(a);
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    SideFxHistory hist{};
    hist.observeInt(31);
    expectSideFx(hist);
}

TEST_F(Compiler, Concepts_ExprPrecedenceOrder) {
    // TODO
}

TEST_F(Compiler, Concepts_None_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: None;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_None_DefaultValue) {
    bindSource("main", R"(

fn f() -> None {
    var a: None;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_None_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_Int_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: Int;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Int_DefaultValue) {
    bindSource("main", R"(

fn f() -> Int {
    var a: Int;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:Int");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, YmInt(0));

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Int_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_UInt_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: UInt;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_UInt_DefaultValue) {
    bindSource("main", R"(

fn f() -> UInt {
    var a: UInt;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:UInt");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, YmUInt(0));

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_UInt_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_Float_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: Float;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Float_DefaultValue) {
    bindSource("main", R"(

fn f() -> Float {
    var a: Float;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:Float");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, YmFloat(0));

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Float_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_Bool_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: Bool;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Bool_DefaultValue) {
    bindSource("main", R"(

fn f() -> Bool {
    var a: Bool;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:Bool");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, YM_FALSE);

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Bool_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_Rune_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: Rune;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Rune_DefaultValue) {
    bindSource("main", R"(

fn f() -> Rune {
    var a: Rune;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:Rune");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, YmRune('\0'));

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Rune_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_Type_FromYamaParcel) {
    bindSource("main", R"(

fn f() {
    var a: Type;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, NoneVal{});

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Type_DefaultValue) {
    bindSource("main", R"(

fn f() -> Type {
    var a: Type;
    return a;
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:Type");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, ymCtx_LdNone(ctx.get()));

    expectSideFx(SideFxHistory{});
}

TEST_F(Compiler, Concepts_Type_NonCallable) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_FnTypes_NoDefaultValue) {
    FAIL(); // TODO
}

TEST_F(Compiler, Concepts_FnTypes_Callable) {
    bindSource("main", R"(

import "sidefx";

fn g() {
    observeInt(33);
}

fn f() {
    g();
}

)");

    ASSERT_TRUE(compile("main"));
    auto f = expectFn("main:f", "fn() -> yama:None");

    ASSERT_TRUE(ymCtx_Call(ctx.get(), f, 0, "", YM_PUSH));
    expectLocal(-1, ymCtx_LdNone(ctx.get()));

    SideFxHistory hist{};
    hist.observeInt(33);
    expectSideFx(hist);
}

