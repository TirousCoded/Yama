

#pragma once


#include "../core/general.h"
#include "../core/const_type.h"
#include "../core/const_table_ref.h"
#include "../core/module.h"

#include "type_mem.h"


namespace yama::internal {
    
    
    class type_instance;


    type_mem get_type_mem(const internal::type_instance& x) noexcept;


    // Encapsulates ownership of the memory of a loaded Yama language type.
    class type_instance final {
    public:
        friend class yama::const_table_ref;
        friend class yama::item_ref;


        // The fullname is expected to be valid according to Yama API semantics
        // regarding type fullnames, and especially that it is valid for use
        // describing a type using the other information used to instantiate
        // the type_instance.
        //
        // the 'Yama API semantics' mentioned above are beyond the scope of 
        // type_instance to define, being left to type_instance's end-users.
        type_instance(str fullname, mid_t mid, module::item info);

        // Clones a type_instance, w/ clone being under a new name.
        //
        // This ctor exists to allow for the cloning of type_instance objects
        // to allow for *incomplete* types (ie. things like generic types w/out
        // resolved params) to be used to derive more *complete* types.
        type_instance(str new_fullname, const type_instance& other);

        type_instance() = delete;
        type_instance(type_instance&&) noexcept = delete;
        ~type_instance() noexcept;
        type_instance& operator=(const type_instance&) = delete;
        type_instance& operator=(type_instance&&) noexcept = delete;


        // Constant table has no stubs.
        bool complete() const noexcept;

        const str& fullname() const noexcept;


        // Put assigns v to the constant at index x in the constant table
        // of the type_instance, overwriting any existing value.
        //
        // The change in the type_instance's state caused by put will be visible
        // to any yama::const_table_ref which exists in association w/ this type_instance.
        //
        // Behaviour is undefined if x is out-of-bounds.
        // Behaviour is undefined if C is not the constant type of the constant at x.
        template<const_type C>
        inline void put(const_t x, const const_data_of_t<C>& v);


    private:
        friend yama::internal::type_mem yama::internal::get_type_mem(const internal::type_instance& x) noexcept;


        // Released via RAII.
        internal::type_mem _mem;


        static internal::type_mem _create_mem(str fullname, mid_t mid, module::item info);
        static internal::type_mem _create_mem(str new_fullname, const type_instance& other);
        static void _destroy_mem(internal::type_mem mem) noexcept;
    };

    template<yama::const_type C>
    inline void type_instance::put(const_t x, const const_data_of_t<C>& v) {
        YAMA_ASSERT(_mem->info.consts().const_type(x) == C); // Out-of-bounds, or wrong C.
        // Decr stubs if we're assigning to a stub.
        if (_mem.elems()[x].holds_stub()) {
            _mem->stubs--;
        }
        _mem.elems()[x] = internal::type_mem_elem::init<C>(v);
    }
}

