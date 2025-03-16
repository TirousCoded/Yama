

#pragma once


#include <optional>

#include "res.h"
#include "kind.h"
#include "ptype.h"
#include "type_info.h"

// TODO: revert back to using frwd decls below if this causes cyclical include issues
#include "callsig.h"
#include "const_table.h"

#include "../internals/type_instance.h"


namespace yama {


    class callsig;
    class const_table;

    namespace internal {
        class type_instance;
        type_mem get_type_mem(type x) noexcept;
    }


    // yama::type is a lightweight non-owning reference to a instantiated
    // Yama language type, and w/ no 'null' state

    // type DOES NOT take ownership of the memory of the type_instance
    // used to instantiate it, meaning that the query provider for yama::type
    // must GUARANTEE to not discard any secondary information which it isn't
    // 100% certain about there not being a yama::type referencing

    // behaviour is undefined if a type object is used outside the scope
    // of the system of type information in which it belongs (ie. if it's
    // used illegally across domain boundaries)

    class type final {
    public:

        // TODO: remove this ctor from frontend

        explicit type(const internal::type_instance& instance) noexcept;

        type() = delete;
        type(const type&) = default;
        type(type&&) noexcept = default;

        ~type() noexcept = default;

        type& operator=(const type&) = default;
        type& operator=(type&&) noexcept = default;


        // complete returns if the type is 'complete', meaning that it has
        // no stubs in its constant table, and is in general ready for use

        bool complete() const noexcept;


        // fullname returns the fullname of the type

        str fullname() const noexcept;

        // kind returns the kind of type this is

        kind kind() const noexcept;

        // consts returns the constant table of the type

        const_table consts() const noexcept;


        std::optional<ptype> ptype() const noexcept;
        std::optional<callsig> callsig() const noexcept;
        std::optional<call_fn> call_fn() const noexcept;
        size_t max_locals() const noexcept; // returns 0 if the type is not callable


        // yama::type equality compares by reference

        bool operator==(const type&) const noexcept = default;


        std::string fmt() const;


    private:
        friend class internal::type_instance;

        friend internal::type_mem yama::internal::get_type_mem(type x) noexcept;


        internal::type_mem _mem;


        explicit type(internal::type_mem mem) noexcept;
    };


    // NOTE: I wanna enforce yama::type being no more than a pointer in size

    static_assert(sizeof(type) <= sizeof(void*));
    static_assert(std::is_trivially_copyable_v<type>);
}


YAMA_SETUP_FORMAT(yama::type, x.fmt());

