

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


struct YmItem final {
public:
    using Name = std::string;


    const ym::Safe<YmParcel> parcel;
    const std::string fullname;
    const ym::Safe<const _ym::ItemInfo> info;


    inline YmItem(ym::Safe<YmParcel> parcel, ym::Safe<const _ym::ItemInfo> info) :
        parcel(parcel),
        fullname(std::format("{}:{}", parcel->path, info->localName)),
        info(info) {
        _initConstsArrayToDummyIntConsts();
    }


    inline YmKind kind() const noexcept { return info->kind; }
    std::string_view path() const noexcept;
    std::string_view localName() const noexcept;

    YmItem* owner() const noexcept;
    YmMembers members() const noexcept;
    YmItem* member(YmMemberIndex member) const noexcept;
    YmItem* member(const std::string& name) const noexcept;

    YmItem* returnType() const noexcept;
    inline YmParams params() const noexcept { return YmParams(info->params.size()); }
    const YmChar* paramName(YmParamIndex param) const;
    YmItem* paramType(YmParamIndex param) const;

    YmItem* ref(YmRef reference) const noexcept;
    std::optional<YmRef> findRef(ym::Safe<YmItem> referenced) const noexcept;

    inline const Name& getName() const noexcept { return fullname; }

    std::span<const _ym::Const> consts() const noexcept;
    template<_ym::ConstType I>
    inline const _ym::Const::Alt<size_t(I)>& constAs(size_t index) const noexcept {
        ymAssert(index < consts().size());
        ymAssert(_ym::constTypeOf(consts()[index]) == I);
        return consts()[index].as<size_t(I)>();
    }

    void putValConst(size_t index);
    // Fails quietly if ref == nullptr.
    void putRefConst(size_t index, YmItem* ref);


private:
    // TODO: Later revise to make our _consts inline w/ the memory block of YmItem itself via HAStruct.

    std::vector<_ym::Const> _consts;


    void _initConstsArrayToDummyIntConsts();

    template<typename T>
    inline void _putValConstAs(size_t index) {
        ymAssert(size_t(index) < info->consts.size());
        ymAssert(info->consts.isVal(index));
        _consts[index] = _ym::Const::byType<T>(info->consts[index].as<T>());
    }
};

