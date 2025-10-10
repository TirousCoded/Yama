

#pragma once


#include <optional>

#include "const_table.h"
#include "const_type.h"

#include "../internals/item_mem.h"


namespace yama {


    class item_ref;


    // const_table_ref provides a non-owning view of the constant table of an item.
    class const_table_ref final {
    public:
        // Implicit conversion.
        const_table_ref(const item_ref& x) noexcept;

        const_table_ref() = delete;
        const_table_ref(const const_table_ref&) = default;
        const_table_ref(const_table_ref&&) noexcept = default;
        ~const_table_ref() noexcept = default;
        const_table_ref& operator=(const const_table_ref&) = default;
        const_table_ref& operator=(const_table_ref&&) noexcept = default;


        // Returns the size of the constant table.
        size_t size() const noexcept;

        // NOTE: Below methods treat out-of-bounds constant indices as referring to stubs.

        // Returns if constant at x, if any, is a stub.
        bool is_stub(const_t x) const noexcept;
        template<const_type C>
        // Returns constant at x as const_data_of_t<C>, or std::nullopt if cannot.
        inline std::optional<const_data_of_t<C>> get(const_t x) const noexcept;
        // Returns the const_type of constant at x.
        std::optional<const_type> const_type(const_t x) const noexcept;
        // Returns item of constant at x, or std::nullopt if cannot.
        std::optional<item_ref> item(const_t x) const noexcept;

        // Compares by reference.
        bool operator==(const const_table_ref&) const noexcept = default;

        std::string fmt_const(const_t x) const;
        std::string fmt_item_const(const_t x) const;
        std::string fmt(const char* tab = default_tab) const;


    private:
        friend class yama::item_ref;


        internal::item_mem _mem;
    };


    static_assert(sizeof(const_table_ref) <= sizeof(void*)); // Guarantee no more than a pointer in size.
    static_assert(std::is_trivially_copyable_v<const_table_ref>);
}


YAMA_SETUP_FORMAT(yama::const_table_ref, x.fmt());


template<yama::const_type C>
inline std::optional<yama::const_data_of_t<C>> yama::const_table_ref::get(const_t x) const noexcept {
    if (x >= size())                return std::nullopt; // Out-of-bounds.
    if (!_mem.elems()[x].holds(C))  return std::nullopt; // Wrong const_type.
    return _mem.elems()[x].as<C>();
}

