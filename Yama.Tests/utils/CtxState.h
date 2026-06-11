

#pragma once


#include <vector>
#include <unordered_map>

#include <yama/yama.h>
#include <yama++/Safe.h>


// TODO: Look into making an 'event history' helper for tracing notable events + side effects.

// TODO: Go through our contexts.cpp unit tests and actually put this to use,
//		 as right now most unit tests that could use this don't.

// TODO: Maybe write unit tests for this helper, to make sure it works.

// Observed state of a context at a given point in time.
class CtxState final {
public:
    YmCallStackHeight callStkHeight = 0;
    std::vector<ym::Safe<YmObj>> args, locals;
    std::unordered_map<ym::Safe<YmObj>, YmRefCount> objRefCounts;


    CtxState(ym::Safe<YmCtx> ctx);

    bool operator==(const CtxState&) const noexcept = default;

    // Compare *this context state against current state of ctx.
    bool expect(ym::Safe<YmCtx> ctx, bool verbose = false) const;
};

