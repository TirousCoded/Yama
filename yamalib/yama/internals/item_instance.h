

#pragma once


#include "../core/general.h"
#include "../core/const_type.h"
#include "../core/const_table_ref.h"
#include "../core/module.h"

#include "item_mem.h"


namespace yama::internal {
    
    
    class item_instance;


    item_mem get_item_mem(const internal::item_instance& x) noexcept;


    // Encapsulates ownership of the memory of a loaded Yama language item.
    class item_instance final {
    public:
        friend class yama::const_table_ref;
        friend class yama::item_ref;


        // The fullname is expected to be valid according to Yama API semantics
        // regarding item fullnames, and especially that it is valid for use
        // describing a item using the other information used to instantiate
        // the item_instance.
        //
        // the 'Yama API semantics' mentioned above are beyond the scope of 
        // item_instance to define, being left to item_instance's end-users.
        item_instance(str fullname, mid_t mid, module::item info);

        // Clones a item_instance, w/ clone being under a new name.
        //
        // This ctor exists to allow for the cloning of item_instance objects
        // to allow for *incomplete* items (ie. things like generic items w/out
        // resolved params) to be used to derive more *complete* items.
        item_instance(str new_fullname, const item_instance& other);

        item_instance() = delete;
        item_instance(item_instance&&) noexcept = delete;
        ~item_instance() noexcept;
        item_instance& operator=(const item_instance&) = delete;
        item_instance& operator=(item_instance&&) noexcept = delete;


        // Constant table has no stubs.
        bool complete() const noexcept;

        const str& fullname() const noexcept;


        // Put assigns v to the constant at index x in the constant table
        // of the item_instance, overwriting any existing value.
        //
        // The change in the item_instance's state caused by put will be visible
        // to any yama::const_table_ref which exists in association w/ this item_instance.
        //
        // Behaviour is undefined if x is out-of-bounds.
        // Behaviour is undefined if C is not the constant item of the constant at x.
        template<const_type C>
        inline void put(const_t x, const const_data_of_t<C>& v);


    private:
        friend yama::internal::item_mem yama::internal::get_item_mem(const internal::item_instance& x) noexcept;


        // Released via RAII.
        internal::item_mem _mem;


        static internal::item_mem _create_mem(str fullname, mid_t mid, module::item info);
        static internal::item_mem _create_mem(str new_fullname, const item_instance& other);
        static void _destroy_mem(internal::item_mem mem) noexcept;
    };

    template<yama::const_type C>
    inline void item_instance::put(const_t x, const const_data_of_t<C>& v) {
        YAMA_ASSERT(_mem->info.consts().const_type(x) == C); // Out-of-bounds, or wrong C.
        // Decr stubs if we're assigning to a stub.
        if (_mem.elems()[x].holds_stub()) {
            _mem->stubs--;
        }
        _mem.elems()[x] = internal::item_mem_elem::init<C>(v);
    }
}

