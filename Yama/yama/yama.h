

#pragma once
#ifndef _YAMA_DOT_H
#define _YAMA_DOT_H 1


#define _YM_FORBID_INCLUDE_IN_YAMA_DOT_H 1

#include "platform.h"
#include "config.h"
#include "macros.h"
#include "basics.h"
#include "asserts.h"
#include "errors.h"

#undef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H


extern "C" {


    /* TODO: I'm not 100% sure if we'll have domain item graph GC in our impl, though
    *        it could still be a cool thing to have in something like an asset manager
    *        for a game or something.
    */

    /* TODO: Quick-n'-dirty idea about managing mem. of domain item graph via a GC:
    *           - Each item gets a ref count for the number of refs it has by things
    *             external to the domain/item system.
    *           - GC has a remembered-set-esque data structure for the root set.
    *               - Items are added when their extern. ref count becomes non-zero.
    *               - Items w/ zero extern. refs get removed from this set during
    *                 collection cycle scanning of this set.
    *           - We 100% know that the item ref graph's internal refs are IMMUTABLE,
    *             meaning the graph can grow, and shrink due to GC, but beyond that
    *             it will NOT MUTATE.
    *           - From startup, or after latest GC cycle, we 100% know that there's
    *             nothing to cleanup (and so no GC cycle needed) UNTIL the extern.
    *             ref count of a GC item gets decremented to zero, at which point
    *             there MIGHT BE newly dead items in the graph.
    *           - We'd need to have a dedicated thread for performing GC cycles, w/
    *             this in turn meaning a stop-the-world or generational mark-and-sweep
    *             system might be best w/ regards to overhead.
    *           - We could have it decide to schedule a new GC cycle when the amount
    *             of memory allocated exceeds, say, 150% or whatever, of what it was
    *             after the last GC cycle.
    *               - This also taking into consideration the detail about us ONLY
    *                 needing to think about new GC cycles once an item's extern. ref
    *                 count is decremented to zero, after last GC cycle.
    * 
    *           - The more I think about the different possible scenarios in this system,
    *             the more I discern that: For this system's impl, we should ALWAYS have
    *             fns which query an item via another item (eg. getting the return type
    *             item from a fn item) result in the returned item pointer having had an
    *             extern. ref count increment made to it, and thus NEVER returning a
    *             borrowed reference.
    *               - The reason for this is that if a borrowed referenced item relies
    *                 on another item being a *root* to survive, if a GC cycle occurs and
    *                 said other item isn't a root, it could lead to memory corruption
    *                 where the borrowed reference pointer becomes DANGLING!!!
    * 
    *           - I REALLY like the above design, and so I think that's what I want.
    *               - Look into maybe doing design like it for object ref graph in
    *                 contexts.
    *                   - Though object ref graph is mutable, and a stop-the-world GC
    *                     would be the opposite of ideal for it, so maybe there's
    *                     actually too much different about the two.
    *               - Not 100% how we'd best change our current design of resource
    *                 'kinds' (ie. RC, non-RC, subres) for the above to be added.
    */


    /* NOTE: When importing a Yama parcel, a 'path' is used to identify it in the
    *        'import environment' of the domain.
    * 
    *        Path strings are UTF-8.
    * 
    *        Paths are simply '/' delimited sequences of 'identifiers', which here
    *        are strings w/out any '/' or ':' characters.
    * 
    *           abc.com/def/ghi
    *           yama/math
    * 
    *        The ':' character is forbidden as it's used to specify the end of the
    *        path part of item decorated names.
    * 
    *           yama/math:Vec3::length
    *           yama:println
    * 
    *           * TODO: This is ONLY an issue in the Yama API, and not in import decls
    *                   in Yama code. To this end, in the former we could allow for
    *                   ':' characters if they're escaped.
    * 
    *        Yama parcels are exposed to the import environment in the following ways:
    * 
    *           1) Parcels can be *pushed* to the import environment by 'binding' them
    *              to specific paths such that after binding said parcel is available
    *              for import via said path.
    * 
    *              The bindings are mutable, being free to be added/removed/changed.
    * 
    *              When import occurs, these parcels take priority over those exposed
    *              via #2 below (w/ said other parcels thus being *shadowed*.)
    * 
    *           2) Parcels can be *pulled* from the external environment (eg. filesystem)
    *              via a specific path, such that said parcel is implicitly exposed to
    *              the import environment.
    * 
    *              The external environment is free to be volatile w/ regards to the set
    *              of mappings from paths to parcels that it exposes.
    * 
    *               * TODO: This hasn't been added yet.
    * 
    *        Once imported, Yama ensures that the path used to import a given parcel
    *        thereafter ALWAYS refers to said parcel, no matter what, completely ignoring
    *        the import environment.
    * 
    *           * TODO: But what about domain resource GC? How would this work if parcels
    *                   are immutable like this?
    */


    /* NOTE: Yama resource memory is managed via intrusive reference counting, sometimes
    *        supplemented by garbage collection to deal w/ strong reference cycles.
    * 
    *        Some resources are use non-atomic reference counting (RC) and others use
    *        atomic reference counting (ARC).
    * 
    *        In either case, ymAddRef and ymDrop are used to add/remove references.
    * 
    *        Resources, in truth, have two reference counts: one affected by end-use of
    *        the Yama API ('public'), and one affected by Yama API internals ('private').
    *        These two reference counts are combined into a single number when querying
    *        the reference count of a resource.
    *           * TODO: Actually decided to remove this for now, but we may add it back later.
    * 
    *        When a resource's reference count becomes 0, the Yama API internals decide
    *        when, if ever, the resource is actually released, w/ this behaviour being
    *        often unobservable to the end-user.
    * 
    *        Likewise, resource's in a strong reference cycle may, as a result of garbage
    *        collection, be cleaned up while still having positive reference counts.
    */

    /* NOTE: The Yama frontend uses Python's notion of RC reference ownership
    *        rules (ie. things like 'borrowed' and 'stolen' references.)
    *           * See https://docs.python.org/3/c-api/intro.html#objects-types-and-reference-counts.
    *
    *        By default, parameters to API functions are *borrowed* references,
    *        being *stolen* references only if specified.
    *
    *        By default, return values from API functions (and out parameters)
    *        are *stolen* references, being *borrowed* references only if specified.
    */


    /* NOTE: Below, resources are to be presumed to be thread-unsafe unless specified otherwise.
    * 
    *        A resource being ARC does NOT imply that they are generally thread-safe, just that
    *        their reference count is thread-safe.
    */

    /* NOTE: For RC resources, the Yama API impl is free to internally treat them as ARC instead
    *        if the impl allows for it.
    * 
    *        In these cases, these are just impl details, and the end-user should still treat
    *        said resources as RC w/ thread-unsafe reference counting.
    */

    /* Domains are ARC resources encapsulating data shared between Yama contexts. */
    /* Domains are thread-safe. */
    struct YmDm;

    /* Contexts are ARC resources encapsulating a Yama execution environment. */
    struct YmCtx;

    /* Parcel defs. are ARC resources encapsulating a Yama parcel specification. */
    /* Parcel defs. fully describe Yama parcels in the absence of linkage and other contextual metadata. */
    /* Parcels are Yama's unit of code distribution. */
    struct YmParcelDef;
#error maybe make internals of below ARC

#error also, make marking res as RC/ARC a static constexpr thing in class

#error also, tomorrow maybe add ym::Option
    /* Parcels are RC resources encapsulating imported Yama parcels. */
    struct YmParcel;


    typedef enum : YmUInt8 {
        YmRType_Dm = 0,
        YmRType_Ctx,
        YmRType_ParcelDef,
        YmRType_Parcel,

        YmRType_Num, /* Enum size. Not a valid resource type. */
    } YmRType;

    /* TODO: ymFmtYmRType hasn't been unit tested. */

    /* Returns the string name of resource type rtype, or "???" if rtype is invalid. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtYmRType(YmRType rtype);


    /* Polymorphic API */

    /* NOTE: These macro-based functions are 'polymorphic' in that their exact behaviour
    *        will depend upon the specific resource which is passed to them.
    * 
    *        Each resource's unit test suite should cover its unique semantics when used
    *        w/ these functions.
    */

    /* Returns the YmRType of Yama resource x. */
    /* Behaviour is undefined if x is invalid. */
#define ymRType(x) _ymRType((void*)x)

    /* TODO: If we ever add back public/private ref. count distinction, their combined
    *        ref. count value should be 64-bit.
    */

    typedef YmUInt32 YmRefCount;

    /* Returns the reference count of resource x. */
    /* Behaviour is undefined if x is invalid. */
#define ymRefCount(x) _ymRefCount((void*)x)

    /* Increments the reference count of resource x. */
    /* Behaviour is undefined if x is invalid. */
#define ymAddRef(x) _ymAddRef((void*)x)

    /* Decrements the reference count of resource x, potentially releasing the resource if its reference count becomes 0. */
    /* Behaviour is undefined if x is invalid. */
    /* Behaviour is undefined if decrements a reference count increment held by Yama API internals. */
#define ymDrop(x) _ymDrop((void*)x)


    /* Domain API */

    /* Creates a new Yama domain, returning a pointer to it. */
    struct YmDm* ymDm_Create(void);

    /* Binds a parcel (defined by parceldef) to path, replacing any existing binding, returning if successful. */
    /* Behaviour is undefined if dm is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmBool ymDm_BindParcelDef(struct YmDm* dm, const YmChar* path, struct YmParcelDef* parceldef);


    /* Context API */

    /* Creates a new Yama context, returning a pointer to it. */
    /* domain is the Yama domain the context is to be associated with. */
    /* Behaviour is undefined if domain is invalid. */
    struct YmCtx* ymCtx_Create(struct YmDm* dm);

    /* Returns the Yama domain associated with ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmDm* ymCtx_Dm(struct YmCtx* ctx);

    /* Imports the parcel at path, returning a pointer to it, or YM_NIL on failure. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    struct YmParcel* ymCtx_Import(struct YmCtx* ctx, const YmChar* path);


    /* Parcel Def. API */

    /* Creates a new Yama parcel def., returning a pointer to it. */
    struct YmParcelDef* ymParcelDef_Create(void);


    /* Parcel API */

    /* Returns the path parcel was imported under. */
    /* The return string's memory is managed internally. */
    /* Behaviour is undefined if parcel is invalid. */
    const YmChar* ymParcel_Path(struct YmParcel* parcel);


    /* Internals */

    YmRType _ymRType(void* x);
    YmRefCount _ymRefCount(void* x);
    void _ymAddRef(void* x);
    void _ymDrop(void* x);
}


#endif

