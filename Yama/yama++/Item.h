

#pragma once


#include "Handle.h"
#include "Parcel.h"


namespace ym {


    // TODO: All of the below haven't been unit tested.

    // A handle wrapping a YmItem.
    class Item final : public Handle<YmItem> {
    public:
        inline explicit Item(Safe<YmItem> x) noexcept :
            Handle(x) {
        }


        inline Parcel parcel() const noexcept {
            return Parcel(Safe(ymItem_Parcel(*this)));
        }
        inline std::string_view fullname() const noexcept {
            return ymItem_Fullname(*this);
        }
        inline YmKind kind() const noexcept {
            return ymItem_Kind(*this);
        }
        inline std::optional<Item> owner() const noexcept {
            if (auto result = ymItem_Owner(*this)) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline YmMembers members() const noexcept {
            return ymItem_Members(*this);
        }
        inline std::optional<Item> member(YmMemberIndex member) const noexcept {
            if (auto result = ymItem_MemberByIndex(*this, member)) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<Item> member(std::convertible_to<std::string_view> auto const& name) const noexcept {
            if (auto result = ymItem_MemberByName(*this, std::string_view(name).data())) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<Item> returnType() const noexcept {
            if (auto result = ymItem_ReturnType(*this)) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline YmParams params() const noexcept {
            return ymItem_Params(*this);
        }
        inline std::optional<std::string_view> paramName(YmParamIndex param) const noexcept {
            if (auto result = ymItem_ParamName(*this, param)) {
                return result;
            }
            return std::nullopt;
        }
        inline std::optional<Item> paramType(YmParamIndex param) const noexcept {
            if (auto result = ymItem_ParamType(*this, param)) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<Item> ref(YmRef reference) const noexcept {
            if (auto result = ymItem_Ref(*this, reference)) {
                return Item(Safe(result));
            }
            return std::nullopt;
        }
        inline std::optional<YmRef> findRef(std::convertible_to<Safe<YmItem>> auto const& referenced) const noexcept {
            if (auto result = ymItem_FindRef(*this, Safe<YmItem>(referenced))) {
                return result;
            }
            return std::nullopt;
        }
    };


    // TODO: converts hasn't been unit tested.

    inline bool converts(
        std::convertible_to<Safe<YmItem>> auto const& from,
        std::convertible_to<Safe<YmItem>> auto const& to,
        bool coercion = false) noexcept {
        return ymItem_Converts(Safe<YmItem>(from), Safe<YmItem>(to), coercion) == YM_TRUE;
    }
}

