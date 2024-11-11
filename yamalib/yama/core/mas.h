

#pragma once


#include <memory>
#include <string>

#include "asserts.h"
#include "api_component.h"


namespace yama {


    // IMPORTANT:
    //      when writing a new MAS impl, write unit tests for the following:
    //          1) basic alloc/dealloc functionality
    //          2) max size querying functionality
    //          3) allocs being aligned as required
    //          4) anything specific to the MAS impl


    // 'Memory Allocation Systems', or 'MASs', are Yama's alternative to std::memory_resource

    // MASs differ from std::memory_resource in the following ways:
    //      1) MASs operate by returning a C++ standard library conforming allocator which
    //         is then used to perform allocations/deallocations/etc.
    //      2) MASs avoid virtual method call indirection as much as possible, w/ the C++
    //         allocator above having the functions used to perform allocation/deallocation/etc.
    //         inline w/ the memory of the C++ allocator itself
    //      3) MASs have a built-in way of reporting memory diagnostics, and w/out needing to
    //         use a proxy like w/ std::memory_resource

    // MASs do not provide a guarantee of allocated memory still outstanding at MAS destruction
    // being cleaned up, presume instead that it will leak

    // MASs provide a guarantee of allocated memory being able to be deallocated in some
    // arbitrary order

    // MAS allocated memory blocks are guaranteed to be aligned to alignof(std::max_align_t)
    

    class mas;


    // mas_allocator_info is used to provide information about a MAS to the
    // C++ allocators used to interact w/ it

    // mas_allocator_info objects are passed four pieces of information:
    //      1) 'client_ptr' which points to the MAS the allocator is acting as a proxy of
    //      2) 'alloc_fn' which defines allocation behaviour
    //      3) 'dealloc_fn' which defines deallocation behaviour
    //      4) 'max_size_fn' which defines max size specification behaviour

    struct mas_allocator_info final {
        using client_ptr_t = mas*;
        using alloc_fn_t = void* (*)(client_ptr_t client_ptr, size_t bytes);
        using dealloc_fn_t = void(*)(client_ptr_t client_ptr, void* block, size_t bytes) noexcept;
        using max_size_fn_t = size_t(*)(client_ptr_t client_ptr) noexcept;


        client_ptr_t client_ptr;
        alloc_fn_t alloc_fn;
        dealloc_fn_t dealloc_fn;
        max_size_fn_t max_size_fn;


        constexpr bool equal(const mas_allocator_info& other) const noexcept {
            return
                client_ptr == other.client_ptr &&
                alloc_fn == other.alloc_fn &&
                dealloc_fn == other.dealloc_fn &&
                max_size_fn == other.max_size_fn;
        }

        constexpr bool operator==(const mas_allocator_info& rhs) const noexcept { return equal(rhs); }
        constexpr bool operator!=(const mas_allocator_info& rhs) const noexcept { return !equal(rhs); }
    };


    // the C++ standard library conforming allocator used by MASs

    template<typename T>
    class mas_allocator final {
    public:

        template<typename U>
        friend class mas_allocator;


        using value_type = T;

        using size_type = size_t;
        using difference_type = std::ptrdiff_t;

        using pointer = value_type*;
        using const_pointer = const value_type*;
        using void_pointer = void*;
        using const_void_pointer = const void*;

        // important that the allocator is not guaranteed to always be equal

        using is_always_equal = std::false_type;

        // important that the allocator propagate under all conditions

        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;

        template<typename U>
        struct rebind final { using other = mas_allocator<U>; };


        mas_allocator() = delete;

        inline mas_allocator(mas_allocator_info info)
            : _info(info) {}

        inline mas_allocator(const mas_allocator<value_type>& other) 
            : _info(other._info) {}

        inline mas_allocator(mas_allocator<value_type>&& other) noexcept 
            : _info(std::move(other._info)) {}

        template<typename U>
        inline mas_allocator(const mas_allocator<U>& other)
            : _info(other._info) {}

        ~mas_allocator() noexcept = default;

        inline mas_allocator<value_type>& operator=(const mas_allocator<value_type>& rhs) {
            _info = rhs._info;
            return *this;
        }

        inline mas_allocator<value_type>& operator=(mas_allocator<value_type>&& rhs) noexcept {
            if (this != &rhs) {
                _info = std::move(rhs._info);
            }
            return *this;
        }


        inline mas_allocator<value_type> select_on_container_copy_construction() const noexcept {
            return mas_allocator<value_type>(*this);
        }


        inline bool equal(const mas_allocator<value_type>& other) const noexcept {
            return _info == other._info;
        }

        inline bool operator==(const mas_allocator<value_type>& rhs) const noexcept { return equal(rhs); }
        inline bool operator!=(const mas_allocator<value_type>& rhs) const noexcept { return !equal(rhs); }


        inline pointer allocate(size_type n) {
            pointer result{};
            YAMA_DEREF_SAFE(_info.alloc_fn) {
                result = (pointer)_info.alloc_fn(_info.client_ptr, n * sizeof(value_type));
            }
            return result;
        }

        inline void deallocate(pointer p, size_type n) noexcept {
            YAMA_DEREF_SAFE(p && _info.dealloc_fn) {
                _info.dealloc_fn(_info.client_ptr, (void_pointer)p, n * sizeof(value_type));
            }
        }

        inline size_type max_size() const noexcept {
            size_type result{};
            YAMA_DEREF_SAFE(_info.max_size_fn) {
                result = _info.max_size_fn(_info.client_ptr);
            }
            return result;
        }

        template<typename U, typename... Args>
        inline void construct(U* xp, Args&&... args) {
            YAMA_DEREF_SAFE(xp) {
                std::construct_at(xp, std::forward<Args&&>(args)...);
            }
        }

        template<typename U>
        inline void destroy(U* xp) noexcept {
            YAMA_DEREF_SAFE(xp) {
                std::destroy_at(xp);
            }
        }


    private:

        mas_allocator_info _info;
    };


    // the main MAS abstract base class itself

    class mas : public api_component {
    public:

        inline mas(std::shared_ptr<debug> dbg = nullptr) 
            : api_component(dbg) {}

        virtual ~mas() noexcept = default;


        // report returns a string encapsulating diagnostic
        // information about MAS memory usage

        // the details of this string are impl specific

        // impls are encouraged to have their diagnostic 
        // string stay on one line, but they are free to
        // have it span multiple lines

        virtual std::string report() const = 0;


        // get returns a MAS allocator for this MAS

        template<typename T>
        inline mas_allocator<T> get() noexcept {
            return mas_allocator<T>(get_info());
        }


    protected:

        virtual mas_allocator_info get_info() noexcept = 0;
    };
}

