

#include <cstdint>
#include <yama/yama.h>
#include <yama++/print.h>
#include <yama++/ParcelDef.h>
#include <yama++/Domain.h>
#include <yama++/Context.h>


int32_t main(int32_t argc, char** argv) {

    auto dm = ym::Domain{};
    auto ctx = ym::Context(dm);

    ym::ParcelDef p_def{};
    auto identity_ind = p_def.addFn(
        "identity",
        "$T",
        { { "v", "$T" } },
        ymInertCallBhvrFn
    ).value();
    (void)p_def.addTypeParam(identity_ind, "T", "yama:Any").value();

    ym::ParcelDef a_def{};
    auto A_ind = a_def.addStruct("A").value();
    (void)a_def.addMethod(A_ind, "hash", "yama:UInt", {}, ymInertCallBhvrFn);
    auto B_ind = a_def.addStruct("B").value();
    (void)a_def.addMethod(B_ind, "hash", "yama:UInt", {}, ymInertCallBhvrFn);
    auto Hash_ind = a_def.addProtocol("Hash").value();
    (void)a_def.addMethodReq(Hash_ind, "hash", "yama:UInt", {});

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

    return 0;
}

