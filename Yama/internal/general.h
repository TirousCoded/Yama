

#pragma once


#include <format>
#include <iostream>
#include <bitset>
#include <concepts>

#include "../yama/yama.h"

#include "Safe.h"


namespace _ym {


    void debugbreak() noexcept;


    template<typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    inline void println(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...) << "\n";
    }


    template<typename T>
    concept Destroyable =
        std::same_as<T, std::remove_cvref_t<T>> &&
        requires (Safe<T> ptr)
    {
        { T::destroy(ptr) } noexcept;
    };
    template<Destroyable T>
    inline void destroy(Safe<T> x) noexcept {
        (void)T::destroy(Safe(x));
    }


    enum class RType : YmUInt8 {
        Domain,
        Context,
    };

    // NOTE: Turns out if we have a base class w/out virt dtor, and then derive a version
    //       w/ one, then the vtable ptr of the ladder will be placed BEFORE fields, w/
    //       this meaning that fields from first class, and their equiv in derived class
    //       will NOT HAVE THE SAME MEMORY OFFSET!!!!
    //
    //       This means that we can't have Resource NOT have a vtable...

    // Base class of all Yama resources.
    class Resource {
    public:
        Resource(RType rtype);
        virtual ~Resource() noexcept = default;


        RType rtype() const noexcept;


    private:
        RType _rtype;
        YmUInt16 _innerRefCount = 0; // Refs held internally.
        YmUInt32 _outerRefCount = 0; // Refs held by end-user.
    };

    YM_STATIC_ASSERT(sizeof(Resource) == 16);
}

