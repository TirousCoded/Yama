

#include <cstdint>
#include <yama/yama.h>
#include <yama++/Context.h>
#include <yama++/Domain.h>
#include <yama++/ParcelDef.h>
#include <yama++/print.h>
#include <yama++/scalar.h>


int32_t main(int32_t argc, char** argv) {
    auto dm = ym::Domain{};
    auto ctx = ym::Context(dm);

    ym::ParcelDef p_def{};
    auto identity_ind = p_def.addFn(
        "identity",
        "$T",
        { { "v", "$T" } },
        {},
        ymInertCallBhvrFn
    ).value();
    (void)p_def.addTypeParam(identity_ind, "T", "yama:Any").value();
    (void)p_def.addFn(
        "recurse",
        "yama:None",
        { { "level", "yama:UInt" } },
        { "p:recurse" },
        [](YmCtx* ctx_, void*) {
            auto ctx = ym::Context(ym::Safe<YmCtx>(ctx_), true);
            ctx.ret(ctx.newNone());
            auto n = ctx.arg(0).value().toUInt().value();
            ym::println("recurse({})\n{}", n, ctx.callStack());
            if (n > 1) {
                ctx.pushUInt(n - 1);
                ctx.calld(ctx.ref(0).value(), 1);
            }
            ctx.disown();
        }).value();
    auto printT_ind = p_def.addFn(
        "printT",
        "yama:None",
        { { "x", "$T" } },
        { "$T" },
        [](YmCtx* ctx_, void*) {
            auto ctx = ym::Context(*ctx_, true);
            auto T = ctx.ref(0).value();
            auto x = ctx.arg(0).value();
            ctx.ret(ctx.newNone());
            if (T == ctx.ldInt())           ym::println("yama:Int {}", ym::fmt(x.toInt().value()));
            else if (T == ctx.ldUInt())     ym::println("yama:UInt {}", ym::fmt(x.toUInt().value()));
            else if (T == ctx.ldFloat())    ym::println("yama:Float {}", ym::fmt(x.toFloat().value()));
            else if (T == ctx.ldBool())     ym::println("yama:Bool {}", ym::fmt(x.toBool().value()));
            else if (T == ctx.ldRune())     ym::println("yama:Rune {}", ym::fmt(x.toRune().value()));
            else if (T == ctx.ldType())     ym::println("yama:Type {}", x.toType().value());
            else                            ym::println("{} n/a", (std::string)T.fullname());
        }).value();
    (void)p_def.addTypeParam(printT_ind, "T", "yama:Any").value();

    ym::ParcelDef a_def{};
    auto A_ind = a_def.addStruct("A").value();
    (void)a_def.addMethod(A_ind, "hash", "yama:UInt", {}, {}, ymInertCallBhvrFn).value();
    auto B_ind = a_def.addStruct("B").value();
    (void)a_def.addMethod(B_ind, "hash", "yama:UInt", {}, {}, ymInertCallBhvrFn).value();
    auto Hash_ind = a_def.addProtocol("Hash").value();
    (void)a_def.addMethodReq(Hash_ind, "hash", "yama:UInt", {}).value();

    dm.bind("p", p_def);
    dm.bind("a", a_def);

    auto A = ctx.load("a:A").value();
    auto B = ctx.load("a:B").value();
    auto Hash = ctx.load("a:Hash").value();

    ym::println("{}\n{}\n{}", A, B, Hash);
    ym::println("{}", ym::converts(A, B));
    ym::println("{}", ym::converts(A, Hash));
    ym::println("{}", ym::converts(A, Hash, true));
    ym::println("{}", ym::converts(B, Hash));
    ym::println("{}", ym::converts(B, Hash, true));

    ym::println("{}", ctx.newFloat(3.14159));
    ym::println("{}", ctx.ldBool());
    ym::println("{}", Hash);
    ym::println("{}", ctx.import("p").value());

    dm.forEachParcel([](
        YmDm* dm,
        void* user,
        YmParcel* parcel,
        size_t increment,
        size_t total) {
            ym::println("-- {}", ym::Parcel(*parcel));
        });

    ctx.pushUInt(10);
    ctx.calld(ctx.load("p:recurse").value(), 1);

    ctx.pushNone();
    ctx.pushInt(-11);
    ctx.pushUInt(11);
    ctx.pushFloat(3.14159);
    ctx.pushBool(true);
    ctx.pushRune(U'y');
    ctx.pushType(ctx.ldBool());
    ctx.calld(ctx.load("p:printT[yama:Type]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:Rune]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:Bool]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:Float]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:UInt]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:Int]").value(), 1);
    ctx.calld(ctx.load("p:printT[yama:None]").value(), 1);

    (void)ctx.load("p:printT[p:printT[yama:Int]]");

    auto aa = ctx.load("p:printT[yama:Int]").value();
    auto pp = ctx.load("yama:Any").value();

    ym::println(">>> {}", ym::converts(aa, pp, true));

    return 0;
}

