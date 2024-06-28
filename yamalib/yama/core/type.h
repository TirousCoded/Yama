

#pragma once


#include <optional>

#include "kind.h"
#include "qs.h"
#include "type_data.h"

#include "../query-systems/provider_traits.h"
#include "../query-systems/key_traits.h"

#include "../internal-api/type_mem.h"


namespace yama {


    // query key

    struct type_k final {
        str fullname;   // the fullname of the type
    };
}


template<>
struct yama::qs::key_traits<yama::qtype, yama::type_k> final {
    using qtypes = yama::qtype;
    using key = yama::type_k;
    static constexpr auto qtype = yama::type_qt;
};


namespace yama {


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

        friend class type_instance;


        // ref_view is meant to be short-lived, w/ undefined behaviour if
        // it outlives the yama::type which created it

        struct refs_view final {
            internal::type_mem _mem; // internal, do not use


            // size returns the number of references in the table

            size_t size() const noexcept;
            
            // ref returns reference at index in the table, if any

            // ref returns std::nullopt if index is out-of-bounds

            // ref returns std::nullopt if index is in-bounds, but 
            // refers to a stub

            std::optional<type> ref(size_t index) const noexcept;

            std::optional<type> operator[](size_t index) const noexcept;
        };

        static_assert(std::is_trivially_copyable_v<refs_view>);

        friend struct refs_view;


        // ctor for init via type_instance (for query provider impls)

        explicit type(const type_instance& instance) noexcept;

        type() = delete;
        type(const type&) = default;
        type(type&& other) noexcept;

        ~type() noexcept = default;

        type& operator=(const type&) = default;
        type& operator=(type&& other) noexcept;


        // complete returns if the type is 'complete', meaning that it
        // has no stubs in its reference table, and is in general
        // ready for use

        bool complete() const noexcept;


        // fullname returns the fullname of the type

        str fullname() const noexcept;

        // kind returns the kind of type this is

        kind kind() const noexcept;


        // refs returns a view of the reference table of the type

        refs_view refs() const noexcept;

        // refsyms returns a span of the reference symbol table used
        // during this type's instantiation

        std::span<const str> refsyms() const noexcept;


        bool operator==(const type& other) const noexcept;


    private:

        internal::type_mem _mem;


        explicit type(internal::type_mem mem) noexcept;
    };
}


template<>
struct yama::qs::provider_traits<yama::qtype, yama::type_qt> final {
    using qtypes = yama::qtype;
    static constexpr auto qtype = yama::type_qt;
    using key = yama::type_k;
    using result = yama::type;
};


namespace yama {


    // yama::type_instance encapsulates the state of an instantiated
    // Yama language type, being responsible for the ownership of it

    // type_instance exists for use ONLY in yama::type query provider 
    // impl backends, and should not appear outside that context

    // type_instance is mutable so that the query provider impl can
    // populate its reference table as needed, w/ unfilled entries
    // being default 'stub' values

    class type_instance final {
    public:

        friend class type;


        // ctor for instantiating a type_instance

        // the reference table of the type instance will have the expected
        // number of reference stubs, which are to then be filled out manually

        // TODO: what are these semantics? who's responsibility is it to check them?

        // the fullname is expected to be valid according to Yama API semantics
        // regarding type fullnames, and especially that it is valid for use
        // describing a type using the other information used to instantiate
        // the type_instance

        type_instance(
            str fullname,
            const type_data& data);

        // ctor for cloning a type_instance, w/ clone being under a new name

        // this ctor exists to allow for the cloning of type_instance objects
        // to allow for *incomplete* types (ie. things like generic types w/out
        // resolved params) to be used to derive more *complete* types

        type_instance(
            str new_fullname,
            const type_instance& other);

        type_instance() = delete;
        type_instance(type_instance&&) noexcept = delete;

        ~type_instance() noexcept;

        type_instance& operator=(const type_instance&) = delete;
        type_instance& operator=(type_instance&&) noexcept = delete;


        // put_ref assigns x to the reference at index in the reference 
        // table of the type of the type_instance, overwriting any existing
        // reference value

        // behaviour is undefined if index is out-of-bounds

        // behaviour is undefined if put_ref is used after the initialization
        // of a yama::type via this type_instance

        void put_ref(size_t index, type x) noexcept;


    private:

        // the type_instance will handle _mem via RAII handled by it

        internal::type_mem _mem;


        static internal::type_mem _create_mem(
            str fullname,
            const type_data& data);
        static internal::type_mem _create_mem(
            str new_fullname,
            const type_instance& other);

        static void _destroy_mem(internal::type_mem mem) noexcept;
    };
}

