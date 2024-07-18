

#pragma once


#include "../internals/type_mem.h"


namespace yama {


    // yama::links_view encapsulates a view of the link table of
    // an instantiated yama::type


    struct links_view final {
        internal::type_mem _mem; // internal, do not use


        // size returns the number of links in the table

        size_t size() const noexcept;

        // link returns the link at index in the table, if any

        // link returns std::nullopt if index is out-of-bounds

        // link returns std::nullopt if index is in-bounds, but 
        // refers to a stub

        std::optional<type> link(link_index index) const noexcept;

        std::optional<type> operator[](link_index index) const noexcept;
    };

    static_assert(std::is_trivially_copyable_v<links_view>);
}

