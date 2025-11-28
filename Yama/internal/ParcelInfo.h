

#pragma once


#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../yama/yama.h"
#include "../yama++/Variant.h"


namespace _ym {


    constexpr bool isValConstType(YmConstType t) noexcept {
        static_assert(YmConstType_Num == 6);
        return t >= YmConstType_Int && t <= YmConstType_Rune;
    }

    constexpr bool isRefConstType(YmConstType t) noexcept {
        return t == YmConstType_Ref;
    }

    struct RefConstInfo final {
        std::string sym;


        bool operator==(const RefConstInfo&) const noexcept = default;
    };

    using ConstInfo = ym::Variant<
        YmInt,
        YmUInt,
        YmFloat,
        YmBool,
        YmRune,

        RefConstInfo
    >;
    static_assert(ConstInfo::size == YmConstType_Num);

    inline YmConstType constTypeOf(const ConstInfo& x) noexcept {
        return YmConstType(x.index());
    }

    struct ItemInfo final {
        const YmLID lid;
        const std::string localName;
        const YmKind kind;
        std::vector<ConstInfo> consts; // Constant table symbols.


        std::optional<std::string_view> ownerName() const noexcept;
        std::optional<std::string_view> memberName() const noexcept;


        // Queries constant table symbol w/out adding if missing.
        YmConst queryConst(const ConstInfo& sym) const noexcept;

        // Performs adding/querying of constant table symbols.
        YmConst pullConst(ConstInfo sym);
    };

    // Encapsulates static parcel data in the absence of linkage.
    class ParcelInfo final {
    public:
        ParcelInfo() = default;


        YmWord items() const noexcept;
        ItemInfo* item(const std::string& localName) noexcept;
        const ItemInfo* item(const std::string& localName) const noexcept;
        ItemInfo* item(YmLID lid) noexcept;
        const ItemInfo* item(YmLID lid) const noexcept;


        // Fails if name conflict arises.
        // Invalidates item pointers.
        std::optional<YmLID> addItem(std::string localName, YmKind kind);

        // Performs adding/querying of constant table symbols.
        YmConst pullConst(YmLID item, ConstInfo sym);


    private:
        std::vector<ItemInfo> _items;
        std::unordered_map<std::string, YmLID> _lookup;
    };
}

