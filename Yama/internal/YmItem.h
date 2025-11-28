

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
        YmRune,

        ym::Safe<YmItem>
    >;
    static_assert(Const::size == YmConstType_Num);

    inline YmConstType constTypeOf(const Const& x) noexcept {
        return YmConstType(x.index());
    }
}


struct YmItem final {
public:
    using ID = YmGID;
    using Name = std::string;


    // TODO: Right now YmItem is generated anew for each context, unlike YmParcel which has
    //       context-independent ParcelData objects they wrap.

    const YmGID gid;
    const std::string fullname;
    const ym::Safe<const _ym::ItemInfo> info;


    inline YmItem(ym::Safe<YmParcel> parcel, ym::Safe<const _ym::ItemInfo> info) :
        gid(ymGID(parcel->pid, info->lid)),
        fullname(std::format("{}:{}", parcel->path, info->localName)),
        info(info) {
        _initConstsArrayToDummyIntConsts();
    }


    inline YmKind kind() const noexcept { return info->kind; }
    std::string_view path() const noexcept;
    std::string_view localName() const noexcept;

    inline const ID& getID() const noexcept { return gid; }
    inline const Name& getName() const noexcept { return fullname; }

    std::span<const _ym::Const> consts() const noexcept;
    template<YmConstType ConstType>
    inline const _ym::Const::Alt<ConstType>& constAs(YmConst index) const noexcept {
        ymAssert(index < consts().size());
        ymAssert(consts()[index].index() == ConstType);
        return consts()[index].as<ConstType>();
    }

    void putValConst(YmConst index);
    // Fails quietly if ref == nullptr.
    void putRefConst(YmConst index, YmItem* ref);


private:
    // TODO: Later revise to make our _consts inline w/ the memory block of YmItem itself via HAStruct.

    std::vector<_ym::Const> _consts;


    void _initConstsArrayToDummyIntConsts();

    template<typename T>
    inline void _putValConstAs(YmConst index) {
        ymAssert(index < info->consts.size());
        ymAssert(_ym::isValConstType(YmConstType(info->consts[index].index())));
        _consts[index] = _ym::Const::byType<T>(info->consts[index].as<T>());
    }
};

