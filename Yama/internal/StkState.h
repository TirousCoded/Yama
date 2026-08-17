

#pragma once


#include <vector>

#include "../yama/yama.h"
#include "ArgPackInfo.h"
#include "obj-ref-helpers.h"


namespace _ym {


    // Call Frame
    struct CF final {
        // Fn being called (or nullptr for user CF.)
        YmType* fn;
        // Where to put return value (in caller's CF.)
        YmLocal returnTo;
        // Global index where CF's local object stack begins.
        // Args are located immediately prior to localsOffset.
        YmUInt32 localsOffset;
        CallArgPackInfo argPack;

        // When protocol methods are called, they never appear on call stack, instead forwarding
        // directly to the method gotten from their ptable.
        // This flag indicates if this CF is for one of these forwarded calls.
        bool fwdFromProto = false;
        // The bound return value object.
        InternalRef returnVal = nullptr;


        inline YmParams args() const noexcept { return argPack.args(); }
        inline YmParams positionalArgs() const noexcept { return argPack.positionalArgs(); }
        inline YmParams namedArgs() const noexcept { return argPack.namedArgs(); }
        inline YmParams dummies() const noexcept { return argPack.dummies(); }

        inline YmUInt32 localOffset(YmLocal where) const noexcept { return localsOffset + where; }
        inline std::optional<YmUInt32> localOffset(std::optional<YmLocal> where) const noexcept {
            if (where) {
                return localOffset(*where);
            }
            return std::nullopt;
        }
        inline std::optional<YmUInt32> argOffset(YmUInt16 which) const noexcept {
            ymAssert(which == YmUInt8(which));
            if (auto offset = argPack.argOffset(YmUInt8(which))) {
                return localsOffset - args() + *offset;
            }
            return std::nullopt;
        }
    };

    // Encapsulate call stack, global obj stack, and info about local obj stacks + args.
    class StkState final {
    public:
        StkState();


        YmCallStackHeight callStkHeight() const noexcept;
        std::string fmtCallStk(YmCallStackHeight skip = 0) const;

        CF& cf() noexcept;
        const CF& cf() const noexcept;

        // Deduces localsOffset.
        void pushCF(YmType* fn, YmLocal returnTo, CallArgPackInfo argPack);
        // Pops locals before pulling CF.
        std::optional<CF> pullCF();
        void popAllCFs();

        bool isUser() const noexcept; // Returns if in user pseudo-call.
        YmUInt32 globals() const noexcept;
        YmUInt16 args() const noexcept;
        YmLocals locals() const noexcept;

        // Transforms negative YmLocal into positive (ie. absolute) ones, failing if out-of-bounds.
        std::optional<YmLocal> absLocal(YmLocal x) const noexcept;
        // Fails if YM_PUSH or YM_DISCARD.
        std::optional<YmLocal> absLocalForRead(YmLocal x) const noexcept;
        
        bool writeIsInBounds(YmLocal where) const noexcept;
        // Fails if YM_PUSH or YM_DISCARD.
        bool readIsInBounds(YmLocal where) const noexcept;

        // Global obj indices ignore boundaries between call frames.
        TempRef global(YmUInt32 index); // Returns borrowed ref.
        TempRef global(std::optional<YmUInt32> index); // Returns borrowed ref.
        // Returns taken ref to local at index, stealing it, and likewise leaving its
        // stack entry w/ an empty InternalRef (so be careful using this method.)
        TempRef stealGlobal(YmUInt32 index, bool frontendRef);
        TempRef stealGlobal(std::optional<YmUInt32> index, bool frontendRef);
        void setGlobal(YmUInt32 index, TempRef v);

        TempRef arg(YmUInt16 which); // Returns borrowed ref.
        bool setArg(YmUInt16 which, TempRef newArg);
        YmType* ref(YmRef reference);
        TempRef local(YmLocal where); // Returns borrowed ref.
        // Returns taken ref to local at where, stealing it, and likewise leaving its
        // stack entry w/ an empty InternalRef (so be careful using this method.)
        TempRef stealLocal(YmLocal where, bool frontendRef);

        void reset();
        void pop(YmLocals n);
        void popUntil(YmLocals n);
        TempRef pull(bool frontendRef) noexcept; // Returns taken ref.
        bool copy(YmLocal from, YmLocal to);
        // Fails quietly if what == nullptr.
        bool put(YmLocal where, TempRef what);
        bool swap(YmLocal a, YmLocal b);
        bool retObj(TempRef what);


    private:
        std::vector<_ym::InternalRef> _globalObjStk;
        std::vector<CF> _callStk;


        void _pushUserPseudoCall();
    };
}

