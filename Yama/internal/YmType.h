

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


// TODO: One issue w/ YmType is that its use of 'YmType*' vs. 'const YmType*' is inconsistent.

struct YmType final : public std::enable_shared_from_this<YmType> {
public:
    using Name = _ym::Spec;

    struct TypeParam final {
        using Info = _ym::TypeInfo::TypeParam;


        ym::Safe<const YmType> self; // No relation to '$Self'.
        ym::Safe<const Info> info;


        inline YmTypeParamIndex index() const noexcept { return info->index; }
        inline const std::string& name() const noexcept { return info->name; }
        YmType& constraint() const noexcept;
        YmType& arg() const noexcept;
    };
    struct Member final {
        using Info = _ym::TypeInfo::Member;


        ym::Safe<const YmType> self; // No relation to '$Self'.
        ym::Safe<const Info> info;


        inline YmMemberIndex index() const noexcept { return info->index; }
        inline const std::string& name() const noexcept { return info->name; }
        YmType& type() const noexcept;
    };
    struct Param final {
        using Info = _ym::TypeInfo::Param;


        ym::Safe<const YmType> self; // No relation to '$Self'.
        ym::Safe<const Info> info;


        inline YmParamIndex index() const noexcept { return info->index; }
        inline const std::string& name() const noexcept { return info->name; }
        inline YmParamCategory category() const noexcept { return info->category; }
        YmType& type() const noexcept;
        inline bool isPositional() const noexcept { return info->isPositional(); }
        inline bool isNamed() const noexcept { return info->isNamed(); }
        bool isSelfParam() const noexcept;
    };


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
        ymAssert(!isMember() || this->typeArgs.empty());
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
        ymAssert(!isMember() || this->typeArgs.empty());
        _initConstsArrayToDummyIntConsts();
        _initFullname(owner);
    }


    inline bool sameAs(const YmType* other) const noexcept { return this == other; }
    inline bool sameAs(const YmType& other) const noexcept { return sameAs(&other); }

    YmKind kind() const noexcept;
    const _ym::Spec& path() const noexcept;
    const _ym::Spec& fullname() const noexcept;
    const std::string& localName() const noexcept;

    bool isRegular() const noexcept;
    bool isIrregular() const noexcept;
    bool isPrimitive() const noexcept;
    bool isGetter() const noexcept;
    bool isSetter() const noexcept;
    bool isVarLike() const noexcept;
    bool isProtocolReq() const noexcept;

    bool hasCallSig() const noexcept;
    bool isOwner() const noexcept;
    bool isMember() const noexcept;
    bool canHaveMembers() const noexcept;

    bool hasDefaultValue() const noexcept;

    static_assert(YmKind_Num == 8);
    bool isStruct() const noexcept;
    bool isProtocol() const noexcept;
    bool isFn() const noexcept;
    bool isVar() const noexcept;
    bool isVarAssigner() const noexcept;
    bool isMethod() const noexcept;
    bool isProperty() const noexcept;
    bool isPropertyAssigner() const noexcept;

    static_assert(YmKind_Num == 8);
    bool isRegularStruct() const noexcept;
    bool isRegularProtocol() const noexcept;
    bool isRegularFn() const noexcept;
    bool isRegularVar() const noexcept;
    bool isRegularVarAssigner() const noexcept;
    bool isRegularMethod() const noexcept;
    bool isRegularProperty() const noexcept;
    bool isRegularPropertyAssigner() const noexcept;

    bool isNone() const noexcept;
    bool isInt() const noexcept;
    bool isUInt() const noexcept;
    bool isFloat() const noexcept;
    bool isBool() const noexcept;
    bool isRune() const noexcept;
    bool isType() const noexcept;

    bool isMethodReq() const noexcept;

    bool isStoredVarGet() const noexcept;
    bool isStoredVarSet() const noexcept;
    bool isStoredPropertyGet() const noexcept;
    bool isStoredPropertySet() const noexcept;

    bool isCallable() const noexcept;
    bool isTypeMethod() const noexcept;
    bool isObjMethod() const noexcept;

    std::optional<std::string> callsuff() const;
    std::optional<_ym::Spec> callsig() const;

    // TODO: We should look into how we may need to update callsigs and callsuffs
    //       to account for named params properly.

    bool checkCallSuff(std::string_view callsuff) const;
    // Succeeds by default if callsuff is empty.
    bool checkCallSuff(std::optional<std::string_view> callsuff) const;

    YmType* var() noexcept;
    const YmType* var() const noexcept;

    YmType* owner() noexcept;
    const YmType* owner() const noexcept;
    ym::Safe<YmType> self() noexcept;
    ym::Safe<const YmType> self() const noexcept;

    YmTypeParams typeParams() const noexcept;
    std::optional<TypeParam> typeParam(YmTypeParamIndex index) const noexcept;
    std::optional<TypeParam> typeParam(const std::string& name) const noexcept;

    YmMembers members() const noexcept;
    std::optional<Member> member(YmMemberIndex index) const noexcept;
    std::optional<Member> member(const std::string& name) const noexcept;

    YmType* returnType() const noexcept;
    YmParams params() const noexcept;
    YmParams positionalParams() const noexcept;
    YmParams namedParams() const noexcept;
    std::optional<Param> param(YmParamIndex index) const noexcept;
    std::optional<Param> param(const std::string& name) const noexcept;
    bool hasSelfParam() const noexcept;

    YmType* assigner() const noexcept;
    YmType* initializer() const noexcept;

    YmType* ref(YmRef reference) const noexcept;
    bool depends(ym::Safe<YmType> other) const noexcept;

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
    inline YmType* constAsRef(std::optional<size_t> index) const noexcept {
        return
            index
            ? constAsRef(*index).get()
            : nullptr;
    }

    void putValConst(size_t index);
    // Fails quietly if ref == nullptr.
    void putRefConst(size_t index, YmType* ref);

    // Call this after fully populating the constant table of the type.
    // Populates _refs w/ nullptr(s) for indices who's corresponding constant is not a ref (ie. such
    // as when its still a dummy int const due to a resolve failure.)
    void buildRefs();


private:
    _ym::Spec _fullname;

    // TODO: Later revise to make our _consts inline w/ the memory block of YmType itself via HAStruct.

    std::vector<_ym::Const> _consts;

    // TODO: I REALLY dislike how we have two seperate vectors for consts: the main one, and then this
    //       second one for user specified refs. When we do the above w/ _consts, try to fuse _refs
    //       into the same inline memory block too.

    std::vector<YmType*> _refs;


    void _initConstsArrayToDummyIntConsts();
    void _initFullname();
    void _initFullname(YmType& owner);

    // Discerns type args vector to use, forwarding to owner for member types.
    const decltype(typeArgs)& _getTypeArgs() const noexcept;

    template<typename T>
    inline void _putValConstAs(size_t index) {
        ymAssert(size_t(index) < info->consts.size());
        ymAssert(info->consts.isVal(index));
        _consts[index] = _ym::Const::byType<T>(info->consts[index].as<T>());
    }

    template<typename T>
    inline std::optional<T> _mkHelper(const T::Info* info) const noexcept {
        if (info) {
            return T{ .self = *this, .info = *info };
        }
        return std::nullopt;
    }
};

