

#pragma once


#include <vector>

#include "../yama/yama.h"
#include "ArgPackInfo.h"
#include "obj-ref-helpers.h"


namespace _ym {


    // Encapsulate call stack, global obj stack, and info about local obj stacks + args.
    // Call procedure is also encapsulated, as while things like get/set of vars/properties
    // is left to YmCtx, call procedure is more nicely handled in this class.
    class StkState final {
    public:
        StkState(YmCtx* ctx);


        YmCallStackHeight callStkHeight() const noexcept;
        std::string fmtCallStk(YmCallStackHeight skip = 0) const;

        bool isUser() const noexcept; // Returns if in user pseudo-call.
        YmUInt32 globals() const noexcept;
        YmUInt16 args() const noexcept;
        YmLocals locals() const noexcept;

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
        bool checkLocalIsInBounds(YmLocal where, std::string_view msgPrefix) const;

        void reset();
        TempRef pull(bool frontendRef) noexcept; // Returns taken ref.
        void pop(YmLocals n);
        bool put(YmLocal where, TempRef what);
        bool swap(YmLocal a, YmLocal b);
        bool call(YmType* fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo);
        bool retObj(TempRef what);


    private:
        struct _CallFrame final {
            // Fn being called (or nullptr for user call frame.)
            YmType* fn;
            _ym::ArgPackInfo<> argPack;
            // Where to put return value.
            YmLocal returnTo;
            // Where in _globalObjStk this call frame's local object stack begins (and below that are its args.)
            YmUInt32 localsOffset;
            // When protocol methods are called, they never appear on call stack, instead forwarding
            // directly to the method gotten from their ptable. This flag indicates if this call frame
            // is for one of these forwarded calls.
            bool fwdFromProto = false;
            // The bound return value object.
            _ym::InternalRef returnValue = nullptr;


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


        YmCtx* _ctx = nullptr;
        std::vector<_ym::InternalRef> _globalObjStk;
        std::vector<_CallFrame> _callStk;


        _CallFrame& _cf() noexcept;
        const _CallFrame& _cf() const noexcept;

        void _beginUserPseudoCall();
        bool _beginCall(YmType* fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo);
        bool _endCall() noexcept;
        void _dispatchCall(YmType* fn);

        // Transforms negative indices into positive absolute ones, and fails if out-of-bounds.
        std::optional<YmLocal> _absIndex(YmLocal x) const noexcept;
        std::optional<YmLocal> _absIndexForRead(YmLocal x) const noexcept;
    };
}

