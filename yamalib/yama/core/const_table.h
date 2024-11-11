

#pragma once


#include <optional>

#include "res.h"
#include "const_table_info.h"
#include "const_type.h"

#include "../internals/type_mem.h"


namespace yama {


    namespace internal {
        template<typename Allocator>
        class type_instance;
    }


    class const_table final {
    public:

        // IMPORTANT:
        //      notice how yama::const_table isn't concerned about how its 
        //      underlying memory is allocated/deallocated, and so is not 
        //      coupled to any particular allocator

        // ctor for init via type_instance

        template<typename Allocator>
        inline explicit const_table(const internal::type_instance<Allocator>& instance) noexcept;

        const_table() = delete;
        const_table(const const_table&) = default;
        const_table(const_table&&) noexcept = default;

        ~const_table() noexcept = default;

        const_table& operator=(const const_table&) = default;
        const_table& operator=(const_table&&) noexcept = default;


        // complete returns if the const_table is 'complete', meaning that it
        // contains no stubs, and is in general ready for use

        bool complete() const noexcept;


        // size returns the size of the constant table

        size_t size() const noexcept;


        // is_stub returns if the constant at x, if any, is a stub

        // is_stub returns true if x is out-of-bounds (ie. all out-of-bounds indices are
        // considered to index to constant stubs)

        bool is_stub(const_t x) const noexcept;


        // get returns constant at x, if any, as const_data_of_t<C>

        // get returns std::nullopt if C is not the constant type of the constant at x

        // get returns std::nullopt if the constant at x is a stub

        template<const_type C>
        inline std::optional<const_data_of_t<C>> get(const_t x) const noexcept;


        // const_type returns the const_type of the constant at x, if any

        std::optional<const_type> const_type(const_t x) const noexcept;


        // NOTE: the purpose of the below methods is to query shared characteristics,
        //       in a way that *abstracts away* the actual constant types

        std::optional<type> type(const_t x) const noexcept; // returns std::nullopt if constant is a stub


        // yama::const_table equality compares by reference

        bool operator==(const const_table&) const noexcept = default;


        std::string fmt_const(const_t x) const;
        std::string fmt_type_const(const_t x) const;
        std::string fmt(const char* tab = "    ") const;


    private:

        friend class yama::type;


        internal::type_mem _mem;


        explicit const_table(internal::type_mem mem) noexcept;
    };


    // NOTE: I wanna enforce yama::const_table being no more than a pointer in size

    static_assert(sizeof(const_table) <= sizeof(void*));
}


YAMA_SETUP_FORMAT(yama::const_table, x.fmt());


template<typename Allocator>
inline yama::const_table::const_table(const internal::type_instance<Allocator>& instance) noexcept
    : _mem(instance._mem) {}

template<yama::const_type C>
inline std::optional<yama::const_data_of_t<C>> yama::const_table::get(const_t x) const noexcept {
    if (x >= size()) { // out-of-bounds
        return std::nullopt;
    }
    if (!_mem.elems()[x].holds(C)) { // wrong const_type, or is a stub
        return std::nullopt;
    }
    return std::make_optional(_mem.elems()[x].as<C>());
}

