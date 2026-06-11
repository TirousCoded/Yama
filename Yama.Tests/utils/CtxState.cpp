

#include "CtxState.h"

#include <unordered_set>

#include <yama++/print.h>


CtxState::CtxState(ym::Safe<YmCtx> ctx) {
    callStkHeight = ymCtx_CallStackHeight(ctx);
    for (YmUInt16 i = 0; i < ymCtx_Args(ctx); i++) {
        auto ref = ym::Safe(ymCtx_Arg(ctx, i, YM_BORROW));
        args.push_back(ref);
        objRefCounts[ref] = ymObj_RefCount(ref);
    }
    for (YmLocals i = 0; i < ymCtx_Locals(ctx); i++) {
        auto ref = ym::Safe(ymCtx_Local(ctx, i, YM_BORROW));
        locals.push_back(ref);
        objRefCounts[ref] = ymObj_RefCount(ref);
    }
}

bool CtxState::expect(ym::Safe<YmCtx> ctx, bool verbose) const {
    auto fmtObj = [](YmObj* x) -> std::string {
        return
            x
            ? std::format("{} ({})", ymType_Fullname(ymObj_Type(x)), (void*)x)
            : "n/a";
        };

    CtxState actual(ctx);
    bool success = *this == actual;

    if (success && !verbose) {
        return true;
    }

    ym::println("** CtxState::expect{} **\nNote: Format is '<Actual> vs <Expected>'.",
        !success ? " Mismatch" : "");

    ym::println("Call Stk Heights: {} vs {}{}",
        actual.callStkHeight, callStkHeight,
        actual.callStkHeight != callStkHeight ? " (Mismatch)" : "");

    ym::println("Arg Counts: {} vs {}{}",
        actual.args.size(), args.size(),
        actual.args.size() != args.size() ? " (Mismatch)" : "");

    size_t argCount = std::max(actual.args.size(), args.size());

    for (YmUInt16 i = 0; i < argCount; i++) {
        auto actualObj = i < actual.args.size() ? actual.args[i].get() : nullptr;
        auto expectedObj = i < args.size() ? args[i].get() : nullptr;

        ym::println("Arg #{}: {} vs {}{}",
            i + 1,
            fmtObj(actualObj),
            fmtObj(expectedObj),
            actualObj != expectedObj ? " (Mismatch)" : "");
    }

    ym::println("Local Counts: {} vs {}{}",
        actual.locals.size(), locals.size(),
        actual.locals.size() != locals.size() ? " (Mismatch)" : "");

    size_t localCount = std::max(actual.locals.size(), locals.size());

    for (YmLocal i = 0; i < localCount; i++) {
        auto actualObj = i < actual.locals.size() ? actual.locals[i].get() : nullptr;
        auto expectedObj = i < locals.size() ? locals[i].get() : nullptr;

        ym::println("Local #{}: {} vs {}{}",
            i + 1,
            fmtObj(actualObj),
            fmtObj(expectedObj),
            actualObj != expectedObj ? " (Mismatch)" : "");
    }

    // Create set of ALL objects observed by *this and actual, so we can iterate
    // over it and ensure that ALL objects from both maps are acknowledged.
    std::unordered_set<ym::Safe<YmObj>> allObjects{};
    for (auto& [key, value] : actual.objRefCounts) {
        allObjects.insert(key);
    }
    for (auto& [key, value] : objRefCounts) {
        allObjects.insert(key);
    }

    for (auto& obj : allObjects) {
        auto actualObj = actual.objRefCounts.find(obj);
        auto expectedObj = objRefCounts.find(obj);

        auto matches =
            actualObj != actual.objRefCounts.end() &&
            expectedObj != objRefCounts.end() &&
            actualObj->first == expectedObj->first &&
            actualObj->second == expectedObj->second;

        ym::println("Object {} refs: {} vs {}{}",
            fmtObj(obj),
            actualObj != actual.objRefCounts.end() ? std::format("{}", actualObj->second) : "?",
            expectedObj != objRefCounts.end() ? std::format("{}", expectedObj->second) : "?",
            !matches ? " (Mismatch)" : "");
    }

    return success;
}

