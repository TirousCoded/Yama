

#pragma once


#include <optional>

#include "kind.h"
#include "link_index.h"
#include "links_view.h"
#include "callsig.h"
#include "type_data.h"

#include "../internals/type_mem.h"


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

        template<typename Allocator>
        friend class type_instance;

        friend struct links_view;


        // ctor for init via type_instance

        // notice how yama::type isn't concerned about how its underlying 
        // memory is allocated/deallocated, and so is not coupled to any 
        // particular allocator

        template<typename Allocator>
        explicit inline type(const type_instance<Allocator>& instance) noexcept;

        type() = delete;
        type(const type&) = default;
        type(type&&) noexcept = default;

        ~type() noexcept = default;

        type& operator=(const type&) = default;
        type& operator=(type&&) noexcept = default;


        // complete returns if the type is 'complete', meaning that it has
        // no stubs in its link table, and is in general ready for use

        bool complete() const noexcept;


        // fullname returns the fullname of the type

        str fullname() const noexcept;

        // kind returns the kind of type this is

        kind kind() const noexcept;

        // callsig returns the call signature of the type, if any

        std::optional<callsig> callsig() const noexcept;


        // links returns a view of the link table of the type

        links_view links() const noexcept;

        // linksyms returns a span of the link symbol table used
        // during this type's instantiation

        std::span<const linksym> linksyms() const noexcept;


        // yama::type equality compares by reference

        bool operator==(const type& other) const noexcept;


    private:

        internal::type_mem _mem;


        explicit type(internal::type_mem mem) noexcept;
    };


    // NOTE: I wanna enforce yama::type being no more than a pointer in size

    static_assert(sizeof(type) <= sizeof(void*));


    // yama::type_instance encapsulates the state of an instantiated
    // Yama language type, being responsible for the ownership of it

    // type_instance exists for use ONLY in yama::domain impl backends, 
    // and should not appear outside that context

    // type_instance is mutable so that the yama::domain impl can
    // populate its link table as needed, w/ unfilled entries
    // being default 'stub' values

    template<typename Allocator>
    class type_instance final {
    public:

        friend class type;


        // ctor for instantiating a type_instance

        // the link table of the type instance will have the expected
        // number of link stubs, which are to then be filled out manually

        // TODO: what are these semantics? who's responsibility is it to check them?

        // the fullname is expected to be valid according to Yama API semantics
        // regarding type fullnames, and especially that it is valid for use
        // describing a type using the other information used to instantiate
        // the type_instance

        inline type_instance(
            Allocator al,
            str fullname,
            const type_data& data);

        // ctor for cloning a type_instance, w/ clone being under a new name

        // this ctor exists to allow for the cloning of type_instance objects
        // to allow for *incomplete* types (ie. things like generic types w/out
        // resolved params) to be used to derive more *complete* types

        inline type_instance(
            Allocator al,
            str new_fullname,
            const type_instance& other);

        type_instance() = delete;
        type_instance(type_instance&&) noexcept = delete;

        inline ~type_instance() noexcept;

        type_instance& operator=(const type_instance&) = delete;
        type_instance& operator=(type_instance&&) noexcept = delete;


        // TODO: get_allocator hasn't been unit tested

        // get_allocator returns the allocator of the type_instance

        inline Allocator get_allocator() const noexcept { return _al; }


        // TODO: fullname hasn't been unit tested

        // fullname returns the fullname of the type of the type_instance

        inline const str& fullname() const noexcept;


        // TODO: get_type_data hasn't been unit tested

        // get_type_data returns the underlying type_data of the type_instance

        inline const type_data& get_type_data() const noexcept;


        // put_link assigns x to the link at index in the link table of the
        // type of the type_instance, overwriting any existing link value

        // the change in type_instance state caused by put_link will be
        // visible to any yama::type which exist of the type_instance

        // behaviour is undefined if index is out-of-bounds

        inline void put_link(link_index index, type x) noexcept;


    private:

        Allocator _al;

        // the type_instance will handle _mem via RAII handled by it

        internal::type_mem _mem;


        static inline internal::type_mem _create_mem(
            Allocator al,
            str fullname,
            const type_data& data);
        static inline internal::type_mem _create_mem(
            Allocator al,
            str new_fullname,
            const type_instance& other);

        static inline void _destroy_mem(
            Allocator al,
            internal::type_mem mem) noexcept;
    };


    template<typename Allocator>
    inline type::type(const type_instance<Allocator>& instance) noexcept
        : _mem(instance._mem) {}


    template<typename Allocator>
    type_instance<Allocator>::type_instance(Allocator al, str fullname, const type_data& data)
        : _al(al),
        _mem(_create_mem(al, std::move(fullname), data)) {}

    template<typename Allocator>
    type_instance<Allocator>::type_instance(Allocator al, str new_fullname, const type_instance& other)
        : _al(al),
        _mem(_create_mem(al, std::move(new_fullname), other)) {}

    template<typename Allocator>
    type_instance<Allocator>::~type_instance() noexcept {
        _destroy_mem(get_allocator(), _mem); // RAII cleanup of _mem
    }

    template<typename Allocator>
    inline const str& type_instance<Allocator>::fullname() const noexcept {
        return _mem->fullname;
    }

    template<typename Allocator>
    inline const type_data& type_instance<Allocator>::get_type_data() const noexcept {
        return _mem->data;
    }

    template<typename Allocator>
    void type_instance<Allocator>::put_link(link_index index, type x) noexcept {
        if (index >= _mem.elems().size()) return;
        // decr stubs if we're assigning to a stub
        if (!_mem.elems()[index]) _mem->stubs--;
        _mem.elems()[index] = x._mem.anon_ref();
    }

    template<typename Allocator>
    yama::internal::type_mem type_instance<Allocator>::_create_mem(Allocator al, str fullname, const type_data& data) {
        internal::type_mem_header header{
            .fullname = fullname,
            .data = data,
            .links = data.linksyms().size(),
            .kind = data.kind(),
            .stubs = data.linksyms().size(),
        };
        return internal::type_mem::create(al, std::move(header));
    }

    template<typename Allocator>
    yama::internal::type_mem type_instance<Allocator>::_create_mem(Allocator al, str new_fullname, const type_instance& other) {
        auto result = internal::type_mem::clone(al, other._mem);
        result->fullname = std::move(new_fullname);
        return result;
    }

    template<typename Allocator>
    void type_instance<Allocator>::_destroy_mem(Allocator al, internal::type_mem mem) noexcept {
        internal::type_mem::destroy(al, mem);
    }
}
