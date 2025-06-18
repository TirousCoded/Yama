

#pragma once


#include <optional>

#include "kind.h"
#include "ptype.h"
#include "type_info.h"

// TODO: revert back to using frwd decls below if this causes cyclical include issues
#include "callsig.h"
#include "const_table.h"

#include "../internals/type_mem.h"


namespace yama {


    class callsig;
    class const_table;
    class type;

    namespace internal {
        class type_instance;
        type_mem get_type_mem(type x) noexcept;
        type create_type(const type_instance& x) noexcept;
    }


    // type is a lightweight non-owning reference to a instantiated Yama language type

    // behaviour is undefined if used in the context of a domain other than
    // the one the type object exists in relation to

    class type final {
    public:
        type() = delete;
        type(const type&) = default;
        type(type&&) noexcept = default;
        ~type() noexcept = default;
        type& operator=(const type&) = default;
        type& operator=(type&&) noexcept = default;


        // TODO: at some point add a 'owner_type' method which returns type which owns
        //       this type (if this type is a member type)
        //
        //       to do this, we'll need to do something like adding property to type_info
        //       which specifies a constant table entry which refs this owner type

        const type_info& info() const noexcept;
        str fullname() const noexcept; // returns the fullname of the type
        kind kind() const noexcept; // returns the kind of type this is
        const_table consts() const noexcept; // returns the constant table of the type
        std::optional<ptype> ptype() const noexcept;
        std::optional<callsig> callsig() const noexcept;
        std::optional<call_fn> call_fn() const noexcept;
        size_t max_locals() const noexcept; // returns 0 if the type is not callable

        bool operator==(const type&) const noexcept = default; // compares by reference

        std::string fmt() const;


    private:
        friend class internal::type_instance;
        friend internal::type_mem internal::get_type_mem(type x) noexcept;
        friend type internal::create_type(const internal::type_instance& x) noexcept;


        internal::type_mem _mem;


        explicit type(const internal::type_instance& instance) noexcept;
    };


    static_assert(sizeof(const_table) <= sizeof(void*)); // guarantee no more than a pointer in size
    static_assert(std::is_trivially_copyable_v<type>);
}


YAMA_SETUP_FORMAT(yama::type, x.fmt());

