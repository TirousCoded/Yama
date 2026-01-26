

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <format>
#include <span>
#include <string>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "ConstTableInfo.h"
#include "ParcelInfo.h"
#include "YmParcel.h"


namespace _ym {


    using Const = ym::Variant<
        YmInt,
        YmUInt,
        YmFloat,
        YmBool,
        YmRune,

        ym::Safe<YmItem>
    >;
    static_assert(Const::size == ConstTypes);

    inline ConstType constTypeOf(const Const& x) noexcept {
        return ConstType(x.index());
    }
}


struct YmItem final : public std::enable_shared_from_this<YmItem> {
public:
    using Name = std::string;


    const ym::Safe<YmParcel> parcel;
    const ym::Safe<const _ym::ItemInfo> info;
    const std::vector<ym::Safe<YmItem>> itemArgs;


    inline YmItem(
        ym::Safe<YmParcel> parcel,
        ym::Safe<const _ym::ItemInfo> info,
        std::vector<ym::Safe<YmItem>> itemArgs = {}) :
        parcel(parcel),
        info(info),
        itemArgs(std::move(itemArgs)) {
        ymAssert(!ymKind_IsMember(kind()) || this->itemArgs.empty());
        _initConstsArrayToDummyIntConsts();
        _initFullname();
    }
    inline YmItem(
        ym::Safe<YmParcel> parcel,
        ym::Safe<const _ym::ItemInfo> info,
        YmItem& owner) :
        parcel(parcel),
        info(info),
        itemArgs(std::move(itemArgs)) {
        ymAssert(!ymKind_IsMember(kind()) || this->itemArgs.empty());
        _initConstsArrayToDummyIntConsts();
        _initFullname(owner);
    }


    inline YmKind kind() const noexcept { return info->kind; }
    const std::string& path() const noexcept;
    const std::string& fullname() const noexcept;
    const std::string& localName() const noexcept;

    YmItem* owner() noexcept;
    const YmItem* owner() const noexcept;
    ym::Safe<YmItem> self() noexcept;
    ym::Safe<const YmItem> self() const noexcept;
    YmMembers members() const noexcept;
    YmItem* member(YmMemberIndex member) const noexcept;
    YmItem* member(const std::string& name) const noexcept;
    YmItemParams itemParams() const noexcept;
    YmItem* itemParam(YmItemParamIndex index) const noexcept;
    YmItem* itemParam(const std::string& name) const noexcept;
    YmItem* itemParamConstraint(YmItemParamIndex index) const noexcept;
    YmItem* itemParamConstraint(const std::string& name) const noexcept;

    YmItem* returnType() const noexcept;
    inline YmParams params() const noexcept { return YmParams(info->params.size()); }
    const YmChar* paramName(YmParamIndex param) const;
    YmItem* paramType(YmParamIndex param) const;

    YmItem* ref(YmRef reference) const noexcept;
    std::optional<YmRef> findRef(ym::Safe<YmItem> referenced) const noexcept;

    bool conforms(ym::Safe<YmItem> protocol) const noexcept;

    inline const Name& getName() const noexcept { return fullname(); }

    std::span<const _ym::Const> consts() const noexcept;
    template<_ym::ConstType I>
    inline const _ym::Const::Alt<size_t(I)>& constAs(size_t index) const noexcept {
        ymAssert(index < consts().size());
        ymAssert(_ym::constTypeOf(consts()[index]) == I);
        return consts()[index].as<size_t(I)>();
    }
    inline ym::Safe<YmItem> constAsRef(size_t index) const noexcept {
        return constAs<_ym::ConstType::Ref>(index);
    }

    void putValConst(size_t index);
    // Fails quietly if ref == nullptr.
    void putRefConst(size_t index, YmItem* ref);


private:
    std::string _fullname;

    // TODO: Later revise to make our _consts inline w/ the memory block of YmItem itself via HAStruct.

    std::vector<_ym::Const> _consts;


    void _initConstsArrayToDummyIntConsts();
    void _initFullname();
    void _initFullname(YmItem& owner);

    template<typename T>
    inline void _putValConstAs(size_t index) {
        ymAssert(size_t(index) < info->consts.size());
        ymAssert(info->consts.isVal(index));
        _consts[index] = _ym::Const::byType<T>(info->consts[index].as<T>());
    }
};

