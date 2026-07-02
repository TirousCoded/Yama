

#pragma once


#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "../yama++/scalar.h"
#include "../yama++/Variant.h"

#include "kinds.h"
#include "ConstTableInfo.h"
#include "SpecSolver.h"


namespace _ym {


    class TypeInfo;
    class ParcelInfo;


    std::optional<Spec> normalizeRefSym(const std::string& symbol, std::string_view msg, SpecSolver solver = {});
    bool checkHasCallSig(const TypeInfo& type, std::string_view msg);
    bool checkNonMember(const TypeInfo& type, std::string_view msg);
    bool checkNonProtocolMember(const TypeInfo& type, std::string_view msg);

    void methodReqCallBhvr(YmCtx* ctx, YmType* type, void* user);
    void storedPropertyGetCallBhvr(YmCtx* ctx, YmType* type, void* user);
    void storedPropertySetCallBhvr(YmCtx* ctx, YmType* type, void* user);
    void storedVarGetCallBhvr(YmCtx* ctx, YmType* type, void* user);
    void storedVarSetCallBhvr(YmCtx* ctx, YmType* type, void* user);


    class TypeInfo final {
    public:
        struct TypeParam final {
            YmTypeParamIndex index;
            std::string name;
            ConstIndex constraintConst;
        };
        struct Member final {
            YmMemberIndex index;
            std::string name;
            ym::Safe<TypeInfo> type;
            ConstIndex typeConst;
        };
        struct Param final {
            YmParamCategory category;
            YmParamIndex index;
            std::string name;
            ConstIndex typeConst;


            inline bool isPositional() const noexcept { return category == YmParamCategory_Positional; }
            inline bool isNamed() const noexcept { return category == YmParamCategory_Named; }
        };


        uint16_t slots = 0;
        ConstTableInfo consts;
        std::vector<ConstIndex> refs;


        TypeInfo(ParcelInfo& parcel, KindEx k, const std::string& localName);


        ParcelInfo& parcel() const noexcept;
        KindEx kindEx() const noexcept;
        YmKind kind() const noexcept;
        const std::string& localName() const noexcept;

        bool isRegular() const noexcept;
        bool isIrregular() const noexcept;
        bool isPrimitive() const noexcept;
        bool isGetter() const noexcept;
        bool isSetter() const noexcept;
        bool isVarLike() const noexcept;
        bool isProtocolReq() const noexcept;

        bool hasCallSig() const noexcept;
        bool hasUserDefinedCallSig() const noexcept;
        bool isOwner() const noexcept;
        bool isMember() const noexcept;
        bool canHaveMembers() const noexcept;
        bool canHaveTypeParams() const noexcept;

        bool hasDefaultValue() const noexcept;

        // NOTE: These don't differentiate regular from irregular.
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

        std::optional<ConstIndex> varConst() const noexcept;

        TypeInfo* owner() const noexcept;
        std::optional<ConstIndex> ownerConst() const noexcept;
        const std::string& ownerName() const noexcept; // Returns local name if owner type.
        const std::string& memberName() const noexcept; // Returns empty string if owner type.

        // TODO: Currently, TypeInfo of member types have NO KNOWLEDGE of type params, which MUST all
        //       be in the owner's TypeInfo, and are thus unavailable to member TypeInfo.

        YmTypeParams typeParams() const noexcept;
        bool isParameterized() const noexcept;
        const TypeParam* typeParam(ConstIndex index) const noexcept;
        const TypeParam* typeParam(const std::string& name) const noexcept;

        YmMembers members() const noexcept;
        const Member* member(ConstIndex index) const noexcept;
        const Member* member(const std::string& name) const noexcept;

        // NOTE: Remember, since it's possible for a self param to be specified by multiple different
        //       symbols (ie. not just $Self), and which symbols do/don't CAN CHANGE BASED ON THE
        //       IMPORT ENVIRONMENT WHEN A YmType IS LOADED!!!
        //
        //       As such, it's not appropriate to check for self params at the TypeInfo level.

        std::optional<ConstIndex> returnTypeConst() const noexcept;
        YmParams params() const noexcept;
        YmParams positionalParams() const noexcept;
        YmParams namedParams() const noexcept;
        const Param* param(YmParamIndex index) const noexcept;
        // O(n) time complexity.
        const Param* param(const std::string& name) const noexcept;
        const CallBhvrCallbackInfo* callBehaviour() const noexcept;

        std::optional<ConstIndex> assignerConst() const noexcept;
        std::optional<ConstIndex> initializerConst() const noexcept;

        std::optional<YmUInt16> storedPropertySlot() const noexcept;

        uint16_t nextSlot() noexcept;
        // Unwinds nextSlot incrs.
        void unwindSlots(uint16_t n = 1) noexcept;

        std::optional<YmTypeParamIndex> addTypeParam(std::string name, std::string constraintTypeSymbol);
        std::optional<YmParamIndex> addParam(std::string name, std::string paramTypeSymbol, bool skipHasCallSigCheck = false);
        void beginNamedParams();
        std::optional<YmRef> addRef(std::string symbol);

        void registerMember(const std::string& name);

        // This isn't performed up-front in _initMembership, as if addType thereafter
        // fails, that would leave the owner TypeInfo w/ a member registered which
        // never actually got added to parcel.
        void registerMembershipWithOwner();

        void setupCall(
            CallBhvrCallbackInfo callBehaviour,
            std::optional<ConstIndex> assignerConst,
            ConstIndex returnTypeConst);
        void setupVar(std::optional<ConstIndex> initializerConst);

        std::string fullnameForRef() const;


    private:
        struct _Membership final {
            std::string ownerName, memberName;
            ym::Safe<TypeInfo> owner;
            ConstIndex ownerConst;
        };
        struct _TypeParams final {
            std::vector<std::unique_ptr<const TypeParam>> typeParamsByIndex;
            std::unordered_map<std::string, ym::Safe<const TypeParam>> typeParamsByName;


            YmTypeParams count() const noexcept;
            const TypeParam* byIndex(size_t index) const noexcept;
            const TypeParam* byName(const std::string& name) const noexcept;

            bool add(const std::string& name, ConstIndex constraintConst);
        };
        struct _Members final {
            std::vector<std::unique_ptr<const Member>> membersByIndex;
            std::unordered_map<std::string, ym::Safe<const Member>> membersByName;


            YmMembers count() const noexcept;
            const Member* byIndex(size_t index) const noexcept;
            const Member* byName(const std::string& name) const noexcept;

            // Fails quietly if already registered.
            void registerMember(TypeInfo& owner, const std::string& name);
        };
        struct _Call final {
            CallBhvrCallbackInfo callBehaviour;
            std::optional<ConstIndex> assignerConst;
            ConstIndex returnTypeConst;
            std::vector<Param> params;
            YmParams positionalParamsN = 0;
            bool definingNamed = false;


            YmParams count() const noexcept;
            YmParams positionalCount() const noexcept;
            YmParams namedCount() const noexcept;
            const Param* param(YmParamIndex index) const noexcept;
            const Param* param(const std::string& name) const noexcept;

            bool addParam(const std::string& name, ConstIndex typeConst);
            void beginNamedParams() noexcept;
        };
        struct _Var final {
            std::optional<ConstIndex> initializerConst;
        };
        struct _VarAssigner final {
            ConstIndex varConst;
        };


        ParcelInfo* _parcel;
        KindEx _k;
        std::string _localName;
        std::unique_ptr<_Membership> _membership;
        std::unique_ptr<_TypeParams> _typeParams;
        std::unique_ptr<_Members> _members;
        std::unique_ptr<_Call> _call;
        std::unique_ptr<_Var> _var;
        std::unique_ptr<_VarAssigner> _varAssigner;


        void _initMembership();
        void _initTypeParams();
        void _initMembers();
        void _initVarAssigner();

        static std::string _extractOwnerName(const std::string& localName) noexcept;
        static std::string _extractMemberName(const std::string& localName) noexcept;
    };

    // Encapsulates static parcel data in the absence of linkage.
    class ParcelInfo final {
    public:
        ParcelInfo() = default;


        bool verify() const;

        size_t types() const noexcept;
        TypeInfo* type(const std::string& localName) noexcept;
        const TypeInfo* type(const std::string& localName) const noexcept;

        // Fails if name conflict arises.
        // Invalidates type pointers.
        bool addType(
            KindEx k,
            const std::string& localName,
            bool skipLocalNameLegalityCheck = false);
        // Fails if name conflict arises.
        // Invalidates type pointers.
        bool addType(
            KindEx k,
            const std::string& ownerName,
            const std::string& memberName,
            bool skipLocalNameLegalityCheck = false);
        // Fails if name conflict arises.
        // Invalidates type pointers.
        bool addType(
            KindEx k,
            const std::string& localName,
            CallBhvrCallbackInfo callBehaviour,
            std::string returnTypeSymbol,
            std::optional<std::string> assignerSymbol = std::nullopt,
            bool skipLocalNameLegalityCheck = false);
        // Fails if name conflict arises.
        // Invalidates type pointers.
        bool addVarType(
            KindEx k,
            const std::string& localName,
            CallBhvrCallbackInfo callBehaviour,
            std::string returnTypeSymbol,
            std::optional<std::string> assignerSymbol = std::nullopt,
            std::optional<std::string> initializerSymbol = std::nullopt,
            bool skipLocalNameLegalityCheck = false);
        // Fails if name conflict arises.
        // Invalidates type pointers.
        bool addType(
            KindEx k,
            const std::string& ownerName,
            const std::string& memberName,
            CallBhvrCallbackInfo callBehaviour,
            std::string returnTypeSymbol,
            std::optional<std::string> assignerSymbol = std::nullopt,
            bool skipLocalNameLegalityCheck = false);

        std::optional<YmTypeParamIndex> addTypeParam(
            std::string typeName,
            std::string name,
            std::string constraintTypeSymbol);
        std::optional<YmParamIndex> addParam(
            std::string typeName,
            std::string name,
            std::string paramTypeSymbol,
            bool skipCallSigChecks = false);
        void beginNamedParams(
            const std::string& typeName);
        std::optional<YmRef> addRef(
            std::string typeName,
            std::string symbol);


    private:
        std::vector<std::unique_ptr<TypeInfo>> _types;
        std::unordered_map<std::string, size_t> _lookup;


        _ym::TypeInfo* _expectType(const std::string& typeName, std::string_view msg);
        bool _checkNameLegality(const std::string& name, std::string_view msg);
        bool _checkNoMemberLevelNameConflict(const TypeInfo& owner, const std::string& name, std::string_view msg);
        bool _checkIsntPropertyOrAssigner(const TypeInfo& t, std::string_view msg);
        bool _checkHasCallSig(const TypeInfo& t, std::string_view msg);
        bool _checkHasUserDefinedCallSig(const TypeInfo& t, std::string_view msg);
        bool _checkCanHaveTypeParams(const TypeInfo& t, std::string_view msg);

        std::optional<_ym::TypeInfo> _makeType(
            KindEx k,
            const std::string& localName,
            bool skipLocalNameLegalityCheck);
        std::optional<_ym::TypeInfo> _makeType(
            KindEx k,
            const std::string& ownerName,
            const std::string& memberName,
            bool skipLocalNameLegalityCheck);
        bool _setupCall(
            TypeInfo& t,
            CallBhvrCallbackInfo callBehaviour,
            std::string returnTypeSymbol,
            std::optional<std::string> assignerSymbol);
        bool _setupVar(
            TypeInfo& t,
            std::optional<std::string> initializerSymbol);
        bool _registerType(TypeInfo t);
        bool _registerType(std::optional<TypeInfo> t);
    };
}

