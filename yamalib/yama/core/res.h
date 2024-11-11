

#pragma once


#include <stdexcept>
#include <memory>

#include "general.h"
#include "asserts.h"


namespace yama {


    // IMPORTANT:
    //      it's not actually suitable to use std::convertible_to in yama::res
    //
    //      given two types: A and B, such that B is derived from A, if A is
    //      an abstract base class, and so can't be instantiated, then we can
    //      conclude that std::convertible_to<B, A> == false
    //
    //      the above fact is why std::convertible_to is unsuitable for use
    //      in the below use case


    // herein, 'res' is short for 'resource'

    // yama::res is a wrapper around std::shared_ptr which specifically
    // removes the 'nullptr' state, guaranteeing that an initialized
    // yama::res will ALWAYS be in a usable state

    // yama::res is implicitly convertible to std::shared_ptr, and
    // std::shared_ptr is explicitly convertible to yama::res, w/ the
    // ladder throwing yama::res_error if it's nullptr


    template<typename T>
    class res;


    class res_error final : public std::runtime_error {
    public:

        explicit res_error(const std::string& msg);
        explicit res_error(const char* msg);
    };


    template<typename T, typename... Args>
    inline res<T> make_res(Args&&... args);


    template<typename T>
    class res final {
    public:

        template<typename U>
        friend class yama::res;


        // default ctor is deleted

        res() = delete;

        // copy/move ctor

        inline res(const res<T>& other);
        inline res(res<T>&& other) noexcept;

        template<typename U>
        inline res(const res<U>& other);
        template<typename U>
        inline res(res<U>&& other) noexcept;

        // ctor for explicit convert to yama::res

        // throws yama::res_error if other == nullptr

        inline explicit res(std::shared_ptr<T> other);

        template<typename U>
        inline explicit res(std::shared_ptr<U> other);

        res(std::nullptr_t) = delete; // delete to avoid confusing runtime errors due to implicit casts

        ~res() noexcept = default;

        // copy/move assign

        inline res<T>& operator=(const res<T>& other);
        inline res<T>& operator=(res<T>&& other) noexcept;
        
        template<typename U>
        inline res<T>& operator=(const res<U>& other);
        template<typename U>
        inline res<T>& operator=(res<U>&& other) noexcept;


        // base returns the underlying std::shared_ptr

        inline std::shared_ptr<T> base() const noexcept { YAMA_ASSERT(_base); return _base; }

        // implicit convert from yama::res

        template<typename U>
        inline operator std::shared_ptr<U>() const noexcept { return (std::shared_ptr<U>)base(); }


        // these are guaranteed to never return nullptr

        inline T* get() const noexcept { YAMA_ASSERT(_base); return _base.get(); }

        inline T& operator*() const noexcept { return *get(); }
        inline T* operator->() const noexcept { return get(); }


        // NOTE: using 'long' here as that's what cppreference says use_count returns

        inline long use_count() const noexcept { YAMA_ASSERT(_base); return _base.use_count(); }


        // NOTE: this has been excluded, as yama::res cannot be nullptr

        //constexpr explicit operator bool() const noexcept { return true; }


        inline bool operator==(const res<T>& other) const noexcept;
        inline bool operator==(const std::shared_ptr<T>& other) const noexcept;
        
        template<typename U>
        inline bool operator==(const res<U>& other) const noexcept;
        template<typename U>
        inline bool operator==(const std::shared_ptr<U>& other) const noexcept;

        constexpr bool operator==(std::nullptr_t) const noexcept { return false; }


    private:

        std::shared_ptr<T> _base;


        static inline void _throw();
    };


    template<typename T, typename... Args>
    res<T> make_res(Args&&... args) {
        return res<T>(std::make_shared<T>(std::forward<Args&&>(args)...));
    }

    template<typename T>
    inline res<T>::res(const res<T>& other)
        : _base(other._base) {
        YAMA_ASSERT(_base);
    }
    
    template<typename T>
    inline res<T>::res(res<T>&& other) noexcept
        // note how this puts other in an invalid state!
        : _base(std::move(other._base)) {
        YAMA_ASSERT(_base);
    }
    
    template<typename T>
    template<typename U>
    inline res<T>::res(const res<U>& other)
        : _base(std::static_pointer_cast<T>(other._base)) {
        YAMA_ASSERT(_base);
    }
    
    template<typename T>
    template<typename U>
    inline res<T>::res(res<U>&& other) noexcept
        // note how this puts other in an invalid state!
        : _base(std::static_pointer_cast<T>(std::move(other._base))) {
        YAMA_ASSERT(_base);
    }
    
    template<typename T>
    inline res<T>::res(std::shared_ptr<T> other)
        : _base(other) {
        if (!other) _throw();
    }
    
    template<typename T>
    template<typename U>
    inline res<T>::res(std::shared_ptr<U> other)
        : _base(std::static_pointer_cast<T>(other)) {
        if (!other) _throw();
    }

    template<typename T>
    inline res<T>& res<T>::operator=(const res<T>& other) {
        _base = other._base;
        return *this;
    }
    
    template<typename T>
    inline res<T>& res<T>::operator=(res<T>&& other) noexcept {
        if (this == &other) return *this;
        // note how this puts other in an invalid state!
        _base = std::move(other._base);
        return *this;
    }
    
    template<typename T>
    template<typename U>
    inline res<T>& res<T>::operator=(const res<U>& other) {
        _base = std::static_pointer_cast<T>(other._base);
        return *this;
    }
    
    template<typename T>
    template<typename U>
    inline res<T>& res<T>::operator=(res<U>&& other) noexcept {
        //if (this == &other) return *this;
        // note how this puts other in an invalid state!
        _base = std::static_pointer_cast<T>(std::move(other._base));
        return *this;
    }

    template<typename T>
    inline bool res<T>::operator==(const res<T>& other) const noexcept {
        return get() == other.get();
    }
    
    template<typename T>
    inline bool res<T>::operator==(const std::shared_ptr<T>& other) const noexcept {
        return get() == other.get();
    }
    
    template<typename T>
    template<typename U>
    inline bool res<T>::operator==(const res<U>& other) const noexcept {
        return get() == (U*)other.get();
    }
    
    template<typename T>
    template<typename U>
    inline bool res<T>::operator==(const std::shared_ptr<U>& other) const noexcept {
        return get() == (U*)other.get();
    }

    template<typename T>
    inline void res<T>::_throw() {
        throw res_error("yama::res initialized with nullptr!");
    }
}

