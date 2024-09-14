

#pragma once


#include "../core/general.h"
#include "../core/const_type.h"
#include "../core/type_info.h"


#define _DUMP_LOG 0


namespace yama::internal {
    template<typename Allocator>
    inline type_mem get_type_mem(const dm::type_instance<Allocator>& x) noexcept;
}

namespace yama::dm {


    // TODO: maybe resolve some of the TODOs below about type_instance not
    //       being independently unit tested by writing some tests for it

    // IMPORTANT:
    //      type_instance has not been unit tested on its own, instead being
    //      unit tested as a component of yama::type, in its unit tests


    // yama::dm::type_instance encapsulates the state of an instantiated
    // Yama language type, being responsible for the ownership of it

    // type_instance exists for use ONLY in yama::domain impl backends, 
    // and should not appear outside that context

    // type_instance is mutable so that the yama::domain impl can
    // populate its constant table as needed

    template<typename Allocator>
    class type_instance final {
    public:

        friend class yama::const_table;
        friend class yama::type;


        // ctor for instantiating a type_instance

        // the fullname is expected to be valid according to Yama API semantics
        // regarding type fullnames, and especially that it is valid for use
        // describing a type using the other information used to instantiate
        // the type_instance

        // the 'Yama API semantics' mentioned above are beyond the scope of 
        // type_instance to define, being left to type_instance's end-users

        inline type_instance(
            Allocator al,
            str fullname,
            res<type_info> info);

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


        // TODO: put hasn't been unit tested

        // put assigns v to the constant at index x in the constant table
        // of the type_instance, overwriting any existing value

        // the change in the type_instance's state caused by put will be visible
        // to any yama::const_table which exists in association w/ this type_instance

        // behaviour is undefined if x is out-of-bounds

        // behaviour is undefined if C is not the constant type of the constant at x

        template<const_type C>
        inline void put(const_t x, const const_data_of_t<C>& v);


    private:

        friend yama::internal::type_mem yama::internal::get_type_mem<Allocator>(const dm::type_instance<Allocator>& x) noexcept;


        Allocator           _al;    // the allocator associated w/ the type_instance

        // the type_instance will manage _mem via RAII

        internal::type_mem  _mem;


        static inline internal::type_mem _create_mem(Allocator al, str fullname, res<type_info> info);
        static inline internal::type_mem _create_mem(Allocator al, str new_fullname, const type_instance& other);
        static inline void _destroy_mem(Allocator al, internal::type_mem mem) noexcept;
    };
}

namespace yama::internal {
    template<typename Allocator>
    inline type_mem get_type_mem(const dm::type_instance<Allocator>& x) noexcept {
        return x._mem;
    }
}


template<typename Allocator>
yama::dm::type_instance<Allocator>::type_instance(Allocator al, str fullname, res<type_info> info)
    : _al(al),
    _mem(_create_mem(al, fullname, info)) {}

template<typename Allocator>
yama::dm::type_instance<Allocator>::type_instance(Allocator al, str new_fullname, const type_instance& other)
    : _al(al),
    _mem(_create_mem(al, new_fullname, other)) {}

template<typename Allocator>
inline yama::dm::type_instance<Allocator>::~type_instance() noexcept {
#if _DUMP_LOG
    std::cerr << std::format("~type_instance @ {}\n", (void*)this);
#endif
    _destroy_mem(get_allocator(), _mem); // RAII cleanup of _mem
}

template<typename Allocator>
inline const yama::str& yama::dm::type_instance<Allocator>::fullname() const noexcept {
    return _mem->fullname;
}

template<typename Allocator>
inline yama::internal::type_mem yama::dm::type_instance<Allocator>::_create_mem(Allocator al, str fullname, res<type_info> info) {
    internal::type_mem_header header{
        .fullname = fullname,
        .len    = info->consts.size(),
        .stubs  = info->consts.size(),
        .info   = info,
        .kind   = info->kind(),
    };
    return internal::type_mem::create(al, std::move(header));
}

template<typename Allocator>
inline yama::internal::type_mem yama::dm::type_instance<Allocator>::_create_mem(Allocator al, str new_fullname, const type_instance<Allocator>& other) {
    auto result = internal::type_mem::clone(al, other._mem);
    result->fullname = new_fullname;
    return result;
}

template<typename Allocator>
inline void yama::dm::type_instance<Allocator>::_destroy_mem(Allocator al, internal::type_mem mem) noexcept {
    internal::type_mem::destroy(al, mem);
}

template<typename Allocator>
template<yama::const_type C>
inline void yama::dm::type_instance<Allocator>::put(const_t x, const const_data_of_t<C>& v) {
    YAMA_ASSERT(_mem->info->consts.const_type(x) == C); // out-of-bounds, or wrong C
    // decr stubs if we're assigning to a stub
    if (_mem.elems()[x].holds_stub()) {
        _mem->stubs--;
    }
    _mem.elems()[x] = internal::type_mem_elem::init<C>(v);
}

