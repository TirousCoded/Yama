

#pragma once


#include <array>
#include <optional>
#include <string>

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
    

    class ParcelData;

    // A superset of YmRType which includes also internal resource types.
    enum class RType : YmUInt8 {
        Dm,
        Ctx,
        ParcelDef,
        Parcel,

        ParcelData,

        Num,
    };

    static_assert(sizeof(RType) == sizeof(YmRType));
    static_assert(enumSize<RType> >= YmRType_Num);

    using RTypeImplTypes = ym::ParamPack<
        YmDm,
        YmCtx,
        YmParcelDef,
        YmParcel,

        ParcelData
    >;

    static_assert(RTypeImplTypes::size == enumSize<RType>);

    struct RTypeProperties final {
        RType rtype;
        RMode rmode;
        const YmChar* name;
        // Only applies to frontend resource types.
        std::optional<YmRType> corresponding = std::nullopt;
    };

    // Compile-time table of resource type properties.
    constexpr std::array<RTypeProperties, enumSize<RType>> rtypePropertiesTable{
        RTypeProperties{ RType::Dm,         RMode::ARC, "Domain",       YmRType_Dm },
        RTypeProperties{ RType::Ctx,        RMode::ARC, "Context",      YmRType_Ctx },
        RTypeProperties{ RType::ParcelDef,  RMode::ARC, "Parcel Def.",  YmRType_ParcelDef },
        RTypeProperties{ RType::Parcel,     RMode::RC,  "Parcel",       YmRType_Parcel },

        RTypeProperties{ RType::ParcelData, RMode::ARC, "Parcel Data" },
    };

    inline const YmChar* fmtRType(RType rtype) {
        ymAssert(rtype < RType::Num);
        return rtypePropertiesTable[size_t(rtype)].name;
    }

    inline RMode rmodeOf(RType rtype) noexcept {
        ymAssert(rtype < RType::Num);
        return rtypePropertiesTable[size_t(rtype)].rmode;
    }

    // NOTE: The below enforces that, given a RType or YmRType which specifies a FRONTEND resource
    //       type, that it's safe to cast between the two, w/ their integer values lining up.

    static_assert([]() constexpr -> bool {
        for (const auto& props : rtypePropertiesTable) {
            if (props.corresponding &&
                YmRType(props.rtype) != props.corresponding) {
                return false;
            }
        }
        return true;
        }());

    // NOTE: Given the above, the below can be used to also CHECK if RType is frontend or backend.

    constexpr bool isFrontend(RType rtype) noexcept {
        return YmRType(rtype) < YmRType_Num;
    }
    constexpr bool isBackend(RType rtype) noexcept {
        return !isFrontend(rtype);
    }

    // RType to YmRType conversion w/ safety check.
    inline YmRType toYmRType(RType rtype) noexcept {
        ymAssert(isFrontend(rtype));
        return YmRType(rtype);
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
        const RType rtype;
        const RMode rmode;


        Resource(RType rtype);
        virtual ~Resource() noexcept;


        bool usesARC() const noexcept;
        YmRefCount refcount() const noexcept;

        void addRef() const noexcept;
        void drop() const noexcept;


        static void destroy(ym::Safe<const Resource> x) noexcept;


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


        template<size_t... Is>
            requires (sizeof...(Is) == RTypeImplTypes::size)
        static inline void _destroyHelper(ym::Safe<const Resource> x, std::integer_sequence<size_t, Is...>) noexcept {
            // Here we're constructing a static constexpr table of fns (w/ a uniform signature) where each index
            // corresponds to a RType value, and each one's entry likewise calls it's associated type's destroy method.
            typedef void(*DestroyFn)(ym::Safe<const Resource> x) noexcept; // <- Wrapper (for our uniform signature.)
            static constexpr std::array<DestroyFn, RTypeImplTypes::size> destroyFns{ // <- Table
                [](ym::Safe<const Resource> x) noexcept {
                    using ImplType = RTypeImplTypes::template TypeAt<Is>;
                    ImplType::destroy(x.downcastInto<const ImplType>()); // <- Dispatch
                }... // <- Notice the parameter pack expansion used to construct the table.
            };
            // Now we query the RType of x, acquire the appropriate destroy method wrapper, and call it.
            destroyFns[size_t(x->rtype)](x);
        }
    };

    static_assert(sizeof(Resource) == 16);
    static_assert(Destroyable<Resource>);


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

