

#pragma once


#include <optional>

#include "const_table_info.h"
#include "const_type.h"

#include "../internals/type_mem.h"


namespace yama {


    class type;


    // const_table provides a non-owning view of the constant table of a type

    class const_table final {
    public:
        const_table(const type& x) noexcept; // implicit convert

        const_table() = delete;
        const_table(const const_table&) = default;
        const_table(const_table&&) noexcept = default;
        ~const_table() noexcept = default;
        const_table& operator=(const const_table&) = default;
        const_table& operator=(const_table&&) noexcept = default;


        // TODO: update our is_stub unit tests when we add types w/ stubs in their
        //       otherwise resolved constant table (also update Get_Stub!)

        size_t size() const noexcept; // returns the size of the constant table

        // NOTE: below methods treat out-of-bounds constant indices as referring to stubs

        bool is_stub(const_t x) const noexcept; // returns if constant at x, if any, is a stub
        template<const_type C>
        inline std::optional<const_data_of_t<C>> get(const_t x) const noexcept; // returns constant at x as const_data_of_t<C>, or std::nullopt if cannot
        std::optional<const_type> const_type(const_t x) const noexcept; // returns the const_type of constant at x
        std::optional<type> type(const_t x) const noexcept; // returns type of constant at x, or std::nullopt if cannot

        bool operator==(const const_table&) const noexcept = default; // compares by reference

        std::string fmt_const(const_t x) const;
        std::string fmt_type_const(const_t x) const;
        std::string fmt(const char* tab = default_tab) const;


    private:
        friend class yama::type;


        internal::type_mem _mem;
    };


    static_assert(sizeof(const_table) <= sizeof(void*)); // guarantee no more than a pointer in size
    static_assert(std::is_trivially_copyable_v<const_table>);
}


YAMA_SETUP_FORMAT(yama::const_table, x.fmt());


template<yama::const_type C>
inline std::optional<yama::const_data_of_t<C>> yama::const_table::get(const_t x) const noexcept {
    if (x >= size())                return std::nullopt; // out-of-bounds
    if (!_mem.elems()[x].holds(C))  return std::nullopt; // wrong const_type, or is a stub
    return _mem.elems()[x].as<C>();
}

