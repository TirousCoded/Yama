

#pragma once


#include <format>
#include <iostream>

#include "Handle.h"
#include "Parcel.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A handle wrapping a YmType.
    class Type final : public Handle<YmType> {
    public:
        class Param final {
        public:
            Param() = delete;
            Param(const Param&) = default;
            Param(Param&&) noexcept = default;
            ~Param() noexcept = default;
            Param& operator=(const Param&) = default;
            Param& operator=(Param&&) noexcept = default;


            bool operator==(const Param&) const noexcept = default;

            constexpr const YmParamIndex& index() const noexcept {
                return _index;
            }
            inline std::string_view name() const noexcept {
                return ymType_ParamName(_item, index());
            }
            inline Type type() const noexcept {
                return Type(Safe(ymType_ParamType(_item, index())));
            }


        private:
            friend class Type;


            Safe<YmType> _item;
            YmParamIndex _index = 0;


            inline Param(Safe<YmType> item, YmParamIndex index) noexcept :
                _item(item),
                _index(index) {
            }
        };


        inline explicit Type(Safe<YmType> x) noexcept :
            Handle(x) {
        }


        inline Parcel parcel() const noexcept {
            return Parcel(Safe(ymType_Parcel(get())));
        }
        inline std::string_view fullname() const noexcept {
            return ymType_Fullname(get());
        }
        inline std::string fmt() const {
            return std::string(fullname());
        }
        inline YmKind kind() const noexcept {
            return ymType_Kind(get());
        }
        inline std::optional<Type> owner() const noexcept {
            if (auto result = ymType_Owner(get())) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }
        inline YmMembers members() const noexcept {
            return ymType_Members(get());
        }
        inline std::optional<Type> member(YmMemberIndex member) const noexcept {
            if (auto result = ymType_MemberByIndex(get(), member)) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<Type> member(std::convertible_to<std::string_view> auto const& name) const noexcept {
            if (auto result = ymType_MemberByName(get(), std::string_view(name).data())) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<Type> returnType() const noexcept {
            if (auto result = ymType_ReturnType(get())) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }
        inline YmParams params() const noexcept {
            return ymType_Params(get());
        }
        inline std::optional<Param> param(YmParamIndex index) const noexcept {
            return
                index < params()
                ? std::make_optional(Param(*this, index))
                : std::nullopt;
        }
        inline std::optional<Type> ref(YmRef reference) const noexcept {
            if (auto result = ymType_Ref(get(), reference)) {
                return Type(Safe(result));
            }
            return std::nullopt;
        }
        inline bool depends(const Type& other) const noexcept {
            return ymType_Depends(get(), other.get());
        }
    };


    // TODO: converts hasn't been unit tested.

    inline bool converts(
        std::convertible_to<Safe<YmType>> auto const& from,
        std::convertible_to<Safe<YmType>> auto const& to,
        bool coercion = false) noexcept {
        return ymType_Converts(Safe<YmType>(from), Safe<YmType>(to), coercion) == YM_TRUE;
    }
}

template<>
struct std::formatter<ym::Type> : std::formatter<std::string> {
    auto format(const ym::Type& x, format_context& ctx) const {
        return formatter<string>::format(x.fmt(), ctx);
    }
};
namespace std {
    inline std::ostream& operator<<(std::ostream& stream, const ym::Type& x) {
        return stream << x.fmt();
    }
}

