

#pragma once


#include <optional>
#include <string>
#include <taul/all.h>
#include <unordered_map>
#include <vector>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "../yama++/scalar.h"
#include "../yama++/Variant.h"

#include "ConstTableInfo.h"
#include "SpecSolver.h"


namespace _ym {


    struct ParamInfo;
    struct TypeInfo;
    class ParcelInfo;


    std::optional<Spec> normalizeRefSym(const std::string& symbol, std::string_view msg, SpecSolver solver = {});
    bool checkCallable(const TypeInfo& type, std::string_view msg);
    bool checkNonMember(const TypeInfo& type, std::string_view msg);

    inline void methodReqCallBhvr(YmCtx* ctx, void* user) {}


    struct TypeParamInfo final {
        YmTypeParamIndex index;
        std::string name;
        ConstIndex constraint;
    };
    struct ParamInfo final {
        YmParamIndex index;
        std::string name;
        ConstIndex type;
    };

    struct TypeInfo final {
        const YmTypeIndex index;
        const std::string localName;
        const YmKind kind;

        // TODO: TypeInfo is rather *fat*, w/ types carrying a LOT of data, much of which many
        //       don't even need to define themselves. To this end, look into a 'descriptor' setup,
        //       like in our old impl, to reduce per-type memory consumption.

        // TODO: When we impl descriptors, be sure to also reorganize TypeInfo owner/member relationships
        //       so that member TypeInfo have DIRECT access to the type param data of their owner, as
        //       right now the lack of this ability is a MAJOR flaw of our currennt impl.

        std::optional<CallBhvrCallbackInfo> callBehaviour;
        std::optional<ConstIndex> owner;
        std::vector<std::shared_ptr<TypeParamInfo>> typeParams;
        std::unordered_map<std::string, ym::Safe<TypeParamInfo>> typeParamNameMap;
        std::vector<ConstIndex> membersByIndex;
        std::unordered_map<std::string, ConstIndex> membersByName;
        std::optional<ConstIndex> returnType;
        std::vector<ParamInfo> params;
        ConstTableInfo consts;


        bool returnTypeIsSelf() const noexcept;
        bool paramTypeIsSelf(YmParamIndex index) const noexcept;

        // TODO: Currently, TypeInfo of member types have NO KNOWLEDGE of type params, which MUST all
        //       be in the owner's TypeInfo, and are thus unavailable to member TypeInfo.

        bool isOwner() const noexcept; // Detects if owner based on name syntax.
        std::optional<std::string_view> ownerName() const noexcept;
        std::optional<std::string_view> memberName() const noexcept;
        const TypeParamInfo* queryTypeParam(YmTypeParamIndex index) const noexcept;
        const TypeParamInfo* queryTypeParam(const std::string& name) const noexcept;
        const ParamInfo* queryParam(YmParamIndex index) const noexcept;
        const ParamInfo* queryParam(const std::string& localName) const noexcept;

        YmMembers memberCount() const noexcept;
        YmTypeParams typeParamCount() const noexcept;
        bool isParameterized() const noexcept;

        std::optional<YmTypeParamIndex> addTypeParam(std::string name, std::string constraintTypeSymbol);
        std::optional<YmParamIndex> addParam(std::string name, std::string paramTypeSymbol);
        std::optional<YmRef> addRef(std::string symbol);

        void attemptSetupAsMember(ParcelInfo& parcel);
        std::string fullnameForRef() const;
    };

    // Encapsulates static parcel data in the absence of linkage.
    class ParcelInfo final {
    public:
        ParcelInfo() = default;


        bool verify() const;

        size_t types() const noexcept;
        TypeInfo* type(const std::string& localName) noexcept;
        const TypeInfo* type(const std::string& localName) const noexcept;
        TypeInfo* type(YmTypeIndex index) noexcept;
        const TypeInfo* type(YmTypeIndex index) const noexcept;

        // Fails if name conflict arises.
        // Invalidates type pointers.
        std::optional<YmTypeIndex> addType(
            std::string localName,
            YmKind kind,
            std::optional<std::string> returnTypeSymbol = std::nullopt,
            std::optional<CallBhvrCallbackInfo> callBehaviour = std::nullopt);
        // Fails if name conflict arises.
        // Invalidates type pointers.
        std::optional<YmTypeIndex> addType(
            YmTypeIndex owner,
            std::string memberName,
            YmKind kind,
            std::optional<std::string> returnTypeSymbol = std::nullopt,
            std::optional<CallBhvrCallbackInfo> callBehaviour = std::nullopt);

        std::optional<YmTypeParamIndex> addTypeParam(
            YmTypeIndex type,
            std::string name,
            std::string constraintTypeSymbol);
        std::optional<YmParamIndex> addParam(
            YmTypeIndex type,
            std::string name,
            std::string paramTypeSymbol);
        std::optional<YmRef> addRef(
            YmTypeIndex type,
            std::string symbol);


    private:
        std::vector<TypeInfo> _types;
        std::unordered_map<std::string, YmTypeIndex> _lookup;


        _ym::TypeInfo* _expectType(YmTypeIndex index, std::string_view msg);
        bool _checkNoMemberLevelNameConflict(const TypeInfo& owner, const std::string& name, std::string_view msg);
    };
}

