

#pragma once


#include "general.h"


namespace _ym {


    constexpr bool usesARC(YmRType rtype) noexcept {
        static_assert(YmRType_Num == 4);
        switch (rtype) {
        case YmRType_Dm:        return true;
        case YmRType_Ctx:       return true;
        case YmRType_ParcelDef: return true;
        case YmRType_Parcel:    return false;
        default:                return bool{};
        }
    }


    // NOTE: Turns out if we have a base class w/out virt dtor, and then derive a version
    //       w/ one, then the vtable ptr of the ladder will be placed BEFORE fields, w/
    //       this meaning that fields from first class, and their equiv in derived class
    //       will NOT HAVE THE SAME MEMORY OFFSET!!!!
    //
    //       This means that we can't have Resource NOT have a vtable...

    // Base class of all Yama resources.
    class Resource {
    public:
        Resource(YmRType rtype);
        virtual ~Resource() noexcept = default;


        YmRType rtype() const noexcept;
        YmRefCount refcount() const noexcept;

        void addRef() noexcept;
        void drop() noexcept;


        // Performs the appropriate destroy behaviour based on rtype.
        // This can destroy resources ymDrop doesn't have permission to.
        static void destroy(ym::Safe<Resource> x) noexcept;


    private:
        YmRType _rtype;
        // NOTE: Remember that C++ unions have one 'active' member, which is
        //       dictated to be the member LAST WRITTEN TO.
        union {
            YmRefCount _refcount;
            std::atomic<YmRefCount> _refcountA;
        };


        // NOTE: See https://stackoverflow.com/questions/41424539/release-consume-ordering-for-reference-counting.

        void _initAtomicRefsIfARC() noexcept;
        void _incrRefs() noexcept;
        // Return if decr made reference count 0.
        bool _decrRefs() noexcept;
    };

    static_assert(sizeof(Resource) == 16);
    static_assert(Destroyable<Resource>);


    template<std::derived_from<Resource> T>
    inline ym::Safe<T> cloneRef(ym::Safe<T> x) noexcept {
        x->addRef();
        return x;
    }

    template<typename T>
    class Res;

    template<std::derived_from<Resource> T>
    inline ym::Safe<T> cloneRef(const Res<T>& x) noexcept {
        return cloneRef(x.borrow());
    }
}

