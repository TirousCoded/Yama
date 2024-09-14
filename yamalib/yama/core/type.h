

#pragma once


#include <optional>

#include "res.h"
#include "kind.h"
#include "ptype.h"
#include "type_info.h"

#include "../internals/type_mem.h"


namespace yama {


    class callsig;
    class const_table;

    namespace dm {
        template<typename Allocator>
        class type_instance;
    }

    namespace internal {
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

        // IMPORTANT:
        //      notice how yama::type isn't concerned about how its underlying 
        //      memory is allocated/deallocated, and so is not coupled to any 
        //      particular allocator

        // ctor for init via type_instance

        template<typename Allocator>
        explicit inline type(const dm::type_instance<Allocator>& instance) noexcept;

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
        size_t locals() const noexcept; // returns 0 if the type is not callable


        // yama::type equality compares by reference

        bool operator==(const type&) const noexcept = default;


        std::string fmt() const;


    private:

        template<typename Allocator>
        friend class yama::dm::type_instance;

        friend internal::type_mem yama::internal::get_type_mem(type x) noexcept;


        internal::type_mem _mem;


        explicit type(internal::type_mem mem) noexcept;
    };


    // NOTE: I wanna enforce yama::type being no more than a pointer in size

    static_assert(sizeof(type) <= sizeof(void*));
    static_assert(std::is_trivially_copyable_v<type>);
}


YAMA_SETUP_FORMAT(yama::type, x.fmt());


template<typename Allocator>
inline yama::type::type(const dm::type_instance<Allocator>& instance) noexcept
    : _mem(instance._mem) {}

