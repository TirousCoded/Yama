

#pragma once


#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <taul/all.h>

#include "../yama/yama.h"
#include "../yama++/Variant.h"
#include "../yama++/scalar.h"

#include "ConstTableInfo.h"


namespace _ym {


    struct ParamInfo;
    struct ItemInfo;
    class ParcelInfo;


    bool checkRefSymbol(const std::string& symbol, std::string_view msg);
    bool checkCallable(const ItemInfo& item, std::string_view msg);


    struct ParamInfo final {
        YmParamIndex index;
        std::string name;
        ConstIndex type;
    };

    struct ItemInfo final {
        const YmItemIndex index;
        const std::string localName;
        const YmKind kind;

        // TODO: ItemInfo is rather *fat*, w/ items carrying a LOT of data, much of which many
        //       don't even need to define themselves. To this end, look into a 'descriptor' setup,
        //       like in our old impl, to reduce per-item memory consumption.

        std::optional<ConstIndex> owner;
        std::vector<ConstIndex> membersByIndex;
        std::unordered_map<std::string, ConstIndex> membersByName;
        std::optional<ConstIndex> returnType;
        std::vector<ParamInfo> params;
        ConstTableInfo consts;


        bool isOwner() const noexcept; // Detects if owner based on name syntax.
        std::optional<std::string_view> ownerName() const noexcept;
        std::optional<std::string_view> memberName() const noexcept;
        const ParamInfo* queryParam(YmParamIndex index) const noexcept;
        const ParamInfo* queryParam(const std::string& localName) const noexcept;

        YmMembers memberCount() const noexcept;

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

        size_t items() const noexcept;
        ItemInfo* item(const std::string& localName) noexcept;
        const ItemInfo* item(const std::string& localName) const noexcept;
        ItemInfo* item(YmItemIndex index) noexcept;
        const ItemInfo* item(YmItemIndex index) const noexcept;

        // Fails if name conflict arises.
        // Invalidates item pointers.
        std::optional<YmItemIndex> addItem(std::string localName, YmKind kind, std::optional<std::string> returnTypeSymbol = std::nullopt);
        // Fails if name conflict arises.
        // Invalidates item pointers.
        std::optional<YmItemIndex> addItem(YmItemIndex owner, std::string memberName, YmKind kind, std::optional<std::string> returnTypeSymbol = std::nullopt);

        std::optional<YmParamIndex> addParam(YmItemIndex item, std::string name, std::string paramTypeSymbol);
        std::optional<YmRef> addRef(YmItemIndex item, std::string symbol);


    private:
        std::vector<ItemInfo> _items;
        std::unordered_map<std::string, YmItemIndex> _lookup;


        _ym::ItemInfo* _expectItem(YmItemIndex index, std::string_view msg);
    };
}

