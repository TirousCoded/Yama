

#pragma once


#include <optional>

#include "Handle.h"
#include "Domain.h"
#include "Object.h"
#include "Parcel.h"
#include "Type.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A RAII handle wrapping a YmCtx.
    class Context final : public Handle<YmCtx> {
    public:
        // TODO: One issue w/ Context::CallStack is that it isn't RAII, meaning that it'll
        //       be invalidated if the lifetime of its YmCtx ends before its does.

        class CallStack final {
        public:
            inline CallStack(std::convertible_to<Safe<YmCtx>> auto const& ctx) :
                _ctx(ctx) {
            }

            CallStack() = delete;
            ~CallStack() noexcept = default;
            CallStack(const CallStack&) = default;
            CallStack(CallStack&&) noexcept = default;
            CallStack& operator=(const CallStack&) = default;
            CallStack& operator=(CallStack&&) noexcept = default;


            inline YmCallStackHeight height() const noexcept { return ymCtx_CallStackHeight(_ctx); }
            inline std::string fmt(YmCallStackHeight skip = 0) const {
                // TODO: Figure out how to remove this extra round of heap alloc.
                auto temp = ymCtx_FmtCallStack(_ctx, skip);
                ym::assertSafe(temp);
                std::string result(temp);
                // TODO: This cleanup won't occur if any above throws.
                std::free((void*)temp);
                return result;
            }


        private:
            ym::Safe<YmCtx> _ctx;
        };


        inline Context(std::convertible_to<Safe<YmDm>> auto const& dm) :
            Context(Safe(ymCtx_Create(Safe<YmDm>(dm))), false) {
        }
        // Increments resource's ref count if secure == true.
        inline explicit Context(Safe<YmCtx> resource, bool secure) noexcept :
            Handle(resource, secure) {
        }


        inline Domain domain() const noexcept {
            return Domain(Safe(ymCtx_Dm(get(), YM_TAKE)), false);
        }

        // path is expected to be null-terminated.
        inline std::optional<Parcel> import(
            std::convertible_to<std::string_view> auto const& path) noexcept {
            if (auto result = ymCtx_Import(get(), std::string_view(path).data())) {
                return Parcel(Safe(result));
            }
            return std::nullopt;
        }
        // fullname is expected to be null-terminated.
        inline std::optional<Type> load(
            std::convertible_to<std::string_view> auto const& fullname) noexcept {
            if (auto result = ymCtx_Load(get(), std::string_view(fullname).data())) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }

        inline Type ldNone() const noexcept { return Type(Safe(ymCtx_LdNone(get()))); }
        inline Type ldInt() const noexcept { return Type(Safe(ymCtx_LdInt(get()))); }
        inline Type ldUInt() const noexcept { return Type(Safe(ymCtx_LdUInt(get()))); }
        inline Type ldFloat() const noexcept { return Type(Safe(ymCtx_LdFloat(get()))); }
        inline Type ldBool() const noexcept { return Type(Safe(ymCtx_LdBool(get()))); }
        inline Type ldRune() const noexcept { return Type(Safe(ymCtx_LdRune(get()))); }
        inline Type ldType() const noexcept { return Type(Safe(ymCtx_LdType(get()))); }

        inline void naturalize(std::convertible_to<Safe<YmParcel>> auto const& x) noexcept {
            ymCtx_NaturalizeParcel(get(), Safe<YmParcel>(x));
        }
        inline void naturalize(std::convertible_to<Safe<YmType>> auto const& x) noexcept {
            ymCtx_NaturalizeType(get(), Safe<YmType>(x));
        }

        inline Object newNone() noexcept { return Object(Safe(ymCtx_NewNone(get())), false); }
        inline Object newInt(YmInt v) noexcept { return Object(Safe(ymCtx_NewInt(get(), v)), false); }
        inline Object newUInt(YmUInt v) noexcept { return Object(Safe(ymCtx_NewUInt(get(), v)), false); }
        inline Object newFloat(YmFloat v) noexcept { return Object(Safe(ymCtx_NewFloat(get(), v)), false); }
        inline Object newBool(YmBool v) noexcept { return Object(Safe(ymCtx_NewBool(get(), v)), false); }
        inline Object newRune(YmRune v) noexcept { return Object(Safe(ymCtx_NewRune(get(), v)), false); }
        inline Object newType(const Type& v) noexcept { return Object(Safe(ymCtx_NewType(get(), v.get())), false); }

        inline CallStack callStack() const noexcept { return CallStack(*this); }
        inline YmUInt16 args() const noexcept { return ymCtx_Args(get()); }
        inline std::optional<Object> arg(YmUInt16 which) noexcept {
            auto temp = ymCtx_Arg(get(), which, YM_TAKE);
            return
                temp
                ? std::make_optional(Object(*temp, false))
                : std::nullopt;
        }
        inline std::optional<Type> ref(YmRef reference) const noexcept {
            auto temp = ymCtx_Ref(get(), reference);
            return
                temp
                ? std::make_optional(Type(*temp))
                : std::nullopt;
        }
        inline YmLocals locals() const noexcept { return ymCtx_Locals(get()); }
        inline std::optional<Object> local(YmLocal where) noexcept {
            auto temp = ymCtx_Local(get(), where, YM_TAKE);
            return
                temp
                ? std::make_optional(Object(*temp, false))
                : std::nullopt;
        }

        inline void popN(YmLocals n) noexcept { ymCtx_PopN(get(), n); }
        inline std::optional<Object> pop() noexcept {
            auto temp = ymCtx_Pop(get());
            return
                temp
                ? std::make_optional(Object(*temp, false))
                : std::nullopt;
        }

        inline bool put(YmLocal where, const Object& what) noexcept {
            return ymCtx_Put(get(), where, what.get(), YM_BORROW) == YM_TRUE;
        }
        inline bool putNone(YmLocal where) noexcept { return ymCtx_PutNone(get(), where) == YM_TRUE; }
        inline bool putInt(YmLocal where, YmInt v) noexcept { return ymCtx_PutInt(get(), where, v) == YM_TRUE; }
        inline bool putUInt(YmLocal where, YmUInt v) noexcept { return ymCtx_PutUInt(get(), where, v) == YM_TRUE; }
        inline bool putFloat(YmLocal where, YmFloat v) noexcept { return ymCtx_PutFloat(get(), where, v) == YM_TRUE; }
        inline bool putBool(YmLocal where, bool v) noexcept { return ymCtx_PutBool(get(), where, YmBool(v)) == YM_TRUE; }
        inline bool putRune(YmLocal where, YmRune v) noexcept { return ymCtx_PutRune(get(), where, v) == YM_TRUE; }
        inline bool putType(YmLocal where, const Type& v) noexcept { return ymCtx_PutType(get(), where, v.get()) == YM_TRUE; }
        inline bool putDefault(YmLocal where, const Type& type) noexcept { return ymCtx_PutDefault(get(), where, type.get()) == YM_TRUE; }

        inline bool push(const Object& what) noexcept { return put(YM_NEWTOP, what); }
        inline bool pushNone() noexcept { return putNone(YM_NEWTOP); }
        inline bool pushInt(YmInt v) noexcept { return putInt(YM_NEWTOP, v); }
        inline bool pushUInt(YmUInt v) noexcept { return putUInt(YM_NEWTOP, v); }
        inline bool pushFloat(YmFloat v) noexcept { return putFloat(YM_NEWTOP, v); }
        inline bool pushBool(bool v) noexcept { return putBool(YM_NEWTOP, v); }
        inline bool pushRune(YmRune v) noexcept { return putRune(YM_NEWTOP, v); }
        inline bool pushType(const Type& v) noexcept { return putType(YM_NEWTOP, v); }
        inline bool pushDefault(const Type& type) noexcept { return putDefault(YM_NEWTOP, type); }

        inline bool call(const Type& fn, YmUInt16 argsN, YmLocal returnTo) noexcept {
            return ymCtx_Call(get(), fn.get(), argsN, returnTo) == YM_TRUE;
        }
        // Pushes return value.
        inline bool callp(const Type& fn, YmUInt16 argsN) noexcept { return call(fn, argsN, YM_NEWTOP); }
        // Discards return value.
        inline bool calld(const Type& fn, YmUInt16 argsN) noexcept { return call(fn, argsN, YM_DISCARD); }
        inline void ret(const Object& what) noexcept { ymCtx_Ret(get(), what.get(), YM_BORROW); }
    };
}


template<>
struct std::formatter<ym::Context::CallStack> : std::formatter<std::string> {
    auto format(const ym::Context::CallStack& x, format_context& ctx) const {
        return formatter<string>::format(x.fmt(), ctx);
    }
};
namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const ym::Context::CallStack& x) {
        return stream << x.fmt();
    }
}

