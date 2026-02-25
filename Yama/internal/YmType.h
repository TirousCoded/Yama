

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
#include "Spec.h"
#include "YmParcel.h"


namespace _ym {


    using Const = ym::Variant<
        YmInt,
        YmUInt,
        YmFloat,
        YmBool,
        YmRune,

        ym::Safe<YmType>
    >;
    static_assert(Const::size == ConstTypes);

    inline ConstType constTypeOf(const Const& x) noexcept {
        return ConstType(x.index());
    }
}


struct YmType final : public std::enable_shared_from_this<YmType> {
public:
    using Name = _ym::Spec;


    const ym::Safe<YmParcel> parcel;
    const ym::Safe<const _ym::TypeInfo> info;
    const std::vector<ym::Safe<YmType>> typeArgs;


    inline YmType(
        ym::Safe<YmParcel> parcel,
        ym::Safe<const _ym::TypeInfo> info,
        std::vector<ym::Safe<YmType>> typeArgs = {}) :
        parcel(parcel),
        info(info),
        typeArgs(std::move(typeArgs)),
        // TODO: This 'dummy' Spec is gross.
        _fullname(_ym::Spec::pathFast("dummy")) {
        ymAssert(!ymKind_IsMember(kind()) || this->typeArgs.empty());
        _initConstsArrayToDummyIntConsts();
        _initFullname();
    }
    inline YmType(
        ym::Safe<YmParcel> parcel,
        ym::Safe<const _ym::TypeInfo> info,
        YmType& owner) :
        parcel(parcel),
        info(info),
        typeArgs(std::move(typeArgs)),
        // TODO: This 'dummy' Spec is gross.
        _fullname(_ym::Spec::pathFast("dummy")) {
        ymAssert(!ymKind_IsMember(kind()) || this->typeArgs.empty());
        _initConstsArrayToDummyIntConsts();
        _initFullname(owner);
    }


    inline YmKind kind() const noexcept { return info->kind; }
    const _ym::Spec& path() const noexcept;
    const _ym::Spec& fullname() const noexcept;
    const std::string& localName() const noexcept;

    std::optional<std::string> callsuff() const;
    std::optional<_ym::Spec> callsig() const;

    bool checkCallSuff(std::string_view callsuff) const;
    // Succeeds by default if callsuff is empty.
    bool checkCallSuff(std::optional<std::string_view> callsuff) const;

    YmType* owner() noexcept;
    const YmType* owner() const noexcept;
    ym::Safe<YmType> self() noexcept;
    ym::Safe<const YmType> self() const noexcept;
    YmMembers members() const noexcept;
    YmType* member(YmMemberIndex member) const noexcept;
    YmType* member(const std::string& name) const noexcept;
    YmTypeParams typeParams() const noexcept;
    YmType* typeParam(YmTypeParamIndex index) const noexcept;
    YmType* typeParam(const std::string& name) const noexcept;
    YmType* typeParamConstraint(YmTypeParamIndex index) const noexcept;
    YmType* typeParamConstraint(const std::string& name) const noexcept;

    YmType* returnType() const noexcept;
    inline YmParams params() const noexcept { return YmParams(info->params.size()); }
    const YmChar* paramName(YmParamIndex param) const;
    YmType* paramType(YmParamIndex param) const;

    YmType* ref(YmRef reference) const noexcept;
    std::optional<YmRef> findRef(ym::Safe<YmType> referenced) const noexcept;

    bool conforms(ym::Safe<YmType> protocol) const noexcept;

    inline const Name& getName() const noexcept { return fullname(); }

    std::span<const _ym::Const> consts() const noexcept;
    template<_ym::ConstType I>
    inline const _ym::Const::Alt<size_t(I)>& constAs(size_t index) const noexcept {
        ymAssert(index < consts().size());
        ymAssert(_ym::constTypeOf(consts()[index]) == I);
        return consts()[index].as<size_t(I)>();
    }
    inline ym::Safe<YmType> constAsRef(size_t index) const noexcept {
        return constAs<_ym::ConstType::Ref>(index);
    }

    void putValConst(size_t index);
    // Fails quietly if ref == nullptr.
    void putRefConst(size_t index, YmType* ref);


private:
    _ym::Spec _fullname;

    // TODO: Later revise to make our _consts inline w/ the memory block of YmType itself via HAStruct.

    std::vector<_ym::Const> _consts;


    void _initConstsArrayToDummyIntConsts();
    void _initFullname();
    void _initFullname(YmType& owner);

    template<typename T>
    inline void _putValConstAs(size_t index) {
        ymAssert(size_t(index) < info->consts.size());
        ymAssert(info->consts.isVal(index));
        _consts[index] = _ym::Const::byType<T>(info->consts[index].as<T>());
    }
};

