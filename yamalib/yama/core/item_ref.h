

#pragma once


#include <optional>

#include "kind.h"
#include "ptype.h"
#include "module.h"
#include "module_ref.h"

// TODO: Revert back to using frwd decls below if this causes cyclical include issues.
#include "callsig_ref.h"
#include "const_table_ref.h"
#include "ids.h"

#include "../internals/type_mem.h"


namespace yama {


    class callsig_ref;
    class const_table_ref;
    class item_ref;

    namespace internal {
        class type_instance;
        type_mem get_type_mem(item_ref x) noexcept;
        item_ref create_type(const type_instance& x) noexcept;
    }


    // A lightweight non-owning reference to a loaded Yama language type.
    // Behaviour is undefined if used in the context of a domain other than
    // the one the type object exists in relation to.
    class item_ref final {
    public:
        item_ref() = delete;
        item_ref(const item_ref&) = default;
        item_ref(item_ref&&) noexcept = default;
        ~item_ref() noexcept = default;
        item_ref& operator=(const item_ref&) = default;
        item_ref& operator=(item_ref&&) noexcept = default;


        module::item info() const noexcept;
        gid_t id() const noexcept;

        // TODO: At some point add a 'owner_type' method which returns item_ref of type
        //       which owns this type (if this type is a member type.)
        //
        //       To do this, we'll need to do something like adding a property specifying
        //       a constant table entry which refs this owner type.

        str fullname() const noexcept;
        kind kind() const noexcept;
        const_table_ref consts() const noexcept;
        std::optional<ptype> ptype() const noexcept;
        std::optional<callsig_ref> callsig() const noexcept;

        // Compares by reference.
        bool operator==(const item_ref&) const noexcept = default;

        std::string fmt() const;


    private:
        friend class internal::type_instance;
        friend internal::type_mem internal::get_type_mem(item_ref x) noexcept;
        friend item_ref internal::create_type(const internal::type_instance& x) noexcept;


        internal::type_mem _mem;


        explicit item_ref(const internal::type_instance& instance) noexcept;
    };

    static_assert(sizeof(const_table_ref) <= sizeof(void*)); // Guarantee no more than a pointer in size.
    static_assert(std::is_trivially_copyable_v<item_ref>);
}

YAMA_SETUP_FORMAT(yama::item_ref, x.fmt());

