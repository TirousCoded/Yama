

#pragma once


#include <array>
#include <optional>
#include <string>

#include "../yama++/print.h"
#include "general.h"


namespace _ym {


    template<typename T>
    class Res;


    // Dictates resource RC/ARC/GC semantics.
    enum class RMode : YmUInt8 {
        RC,
        ARC,

        Num,
    };

    inline const YmChar* fmtRMode(RMode rmode) {
        static constexpr std::array<const YmChar*, enumSize<RMode>> names{
            "RC",
            "ARC",
        };
        ymAssert(rmode < RMode::Num);
        return names[size_t(rmode)];
    }


    // Base class of all Yama resources.
    class Resource {
    public:
        const RMode rmode;


        Resource(RMode rmode);
        virtual ~Resource() noexcept = default;


        bool usesARC() const noexcept;
        YmRefCount refcount() const noexcept;

        void addRef() const noexcept;
        void drop() const noexcept;


        // TODO: Maybe make rtype be injected as an enum value like RMode is, if we ever find
        //       ourselves needing to use rtype for things that are really performance-critical.

        // Returns the YmRType of this resource, if any.
        inline virtual std::optional<YmRType> rtype() const noexcept {
            return std::nullopt;
        }

        // Returns a name for the type of resource this is, for debug purposes.
        virtual const YmChar* debugName() const noexcept = 0;

        // TODO: W/ the virtual dtor, currently resource cleanup involves DOUBLE DISPATCH,
        //       first to destroy, then the dtor. This is not NECESSARILY too bad, as it's
        //       only really a performance cost from a data/instr cache standpoint, but it's
        //       still something to look into later when optimizing.
        //
        //       If we end up wanting to de-virtualize destroy, what we can do is something
        //       akin to RMode where we have well-known protocols regarding resource destruction,
        //       and then we use an enum value to specify which the resource uses.
        //
        //       As part of this, we could still have virtual method call for destroy, but
        //       as an *optional* thing enabled by a certain mode.

        // TODO: destroy being const method is due to drop being const method, but I'm not 100%
        //       on how I feel about that...

        // NOTE: The below destroy method is expected to be implemented alongside a STATIC
        //       'create' method which returns a pointer to the resource.
        //
        //       The create methods allocs/inits a resource object, and destroy deinits/deallocs
        //       it, such that the resource impl controls HOW resources are allocated.
        //
        //       This create method must return a 'Res<T>', where T is the impl type.
        //
        //       The reference count for this new object must start off as 1 (ie. not 0.)

        // Deinitializes *this and deallocates its memory block.
        virtual void destroy() const noexcept = 0;


    private:
        // NOTE: Remember that C++ unions have one 'active' member, which is
        //       dictated to be the member LAST WRITTEN TO.
        union {
            mutable YmRefCount _refcount;
            mutable std::atomic<YmRefCount> _refcountA;
        };


        // NOTE: See https://stackoverflow.com/questions/41424539/release-consume-ordering-for-reference-counting.
        // NOTE: Also https://github.com/gershnik/intrusive_shared_ptr/blob/master/doc/reference_counting.md.
        //          * This second one is REALLY useful!

        void _initAtomicRefsIfARC() noexcept;
        void _incrRefs() const noexcept;
        // Return if decr made reference count 0.
        bool _decrRefs() const noexcept;
    };

    static_assert(sizeof(Resource) == 16);


    // Helper used to summarize definition of frontend resources.
    template<YmRType RTYPE, RMode RMODE>
    class FrontendResource : public Resource {
    public:
        inline FrontendResource() : Resource(RMODE) {}


        inline std::optional<YmRType> rtype() const noexcept override final {
            return RTYPE;
        }
        inline const YmChar* debugName() const noexcept override final {
            return ymFmtYmRType(RTYPE);
        }
    };


    template<std::derived_from<Resource> T>
    inline ym::Safe<T> cloneRef(ym::Safe<T> x) noexcept {
        x->addRef();
        return x;
    }
    template<std::derived_from<Resource> T>
    inline ym::Safe<T> cloneRef(const Res<T>& x) noexcept {
        return cloneRef(x.borrow());
    }
}

