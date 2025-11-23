

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <format>
#include <span>
#include <string>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "ParcelInfo.h"
#include "YmParcel.h"


namespace _ym {
    using Const = ym::Variant<
        YmInt,
        YmUInt,
        YmFloat,
        YmBool,
        YmRune
    >;
    static_assert(Const::size == YmConstType_Num);

    inline YmConstType constTypeOf(const Const& x) noexcept {
        return YmConstType(x.index());
    }
}


struct YmItem final {
public:
    // TODO: Right now YmItem is generated anew for each context, unlike YmParcel which has
    //       context-independent ParcelData objects they wrap.

    const YmGID gid;
    const std::string fullname;
    const ym::Safe<const _ym::ItemInfo> info;


    inline YmItem(ym::Safe<YmCtx> ctx, ym::Safe<YmParcel> parcel, ym::Safe<const _ym::ItemInfo> info) :
        gid(ymGID(parcel->pid(), info->lid)),
        fullname(std::format("{}:{}", parcel->path(), info->localName)),
        info(info) {
    }


    inline YmKind kind() const noexcept { return info->kind; }
    std::string_view path() const noexcept;
    std::string_view localName() const noexcept;

    std::span<const _ym::Const> consts() const noexcept;
    template<YmConstType ConstType>
    inline const _ym::Const::Alt<ConstType>& constAs(YmConst index) const noexcept {
        ymAssert(index < consts().size());
        ymAssert(consts()[index].index() == ConstType);
        return consts()[index].as<ConstType>();
    }

    void resolveConsts();


private:
    // TODO: Later revise to make our _consts inline w/ the memory block of YmItem itself via HAStruct.

    std::vector<_ym::Const> _consts;
};

