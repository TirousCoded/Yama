

#pragma once
#ifndef _YAMA_DOT_H
#define _YAMA_DOT_H 1


#define _YM_FORBID_INCLUDE_IN_YAMA_DOT_H 1

#include "platform.h"
#include "config.h"
#include "macros.h"
#include "scalars.h"
#include "asserts.h"
#include "errors.h"

#undef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H


extern "C" {


    /* TODO: Not 100% sure where to put this so I don't forget, so I'm putting it here.
    * 
    *        Reading material regarding CJK, EPA, and physics constraints:
    *           https://winter.dev/articles/gjk-algorithm
    *           https://dyn4j.org/2010/05/epa-expanding-polytope-algorithm/#epa-alternatives
    *           
    *           https://en.wikipedia.org/wiki/Minkowski_Portal_Refinement
    *           http://xenocollide.snethen.com/
    *           https://github.com/danfis/libccd
    * 
    *           https://dyn4j.org/2010/07/equality-constraints/
    *           https://dyn4j.org/2010/07/point-to-point-constraint/
    *           https://dyn4j.org/2010/09/distance-constraint/
    *           https://dyn4j.org/2010/12/max-distance-constraint/
    *           https://dyn4j.org/2010/12/weld-constraint/
    *           https://dyn4j.org/2010/12/pulley-constraint/
    *           https://dyn4j.org/2010/12/line-constraint/
    *           https://dyn4j.org/2010/12/angle-constraint/
    *           https://dyn4j.org/2011/03/prismatic-constraint/
    */


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

    /* NOTE: When loading a Yama Item a 'fullname' is used to specify the item to load.
    * 
    *        Regular non-member item fullnames are simply identifiers paired w/ an import
    *        path specifying their parcel, seperated by a ':'.
    * 
    *           <path>:<identifier>
    * 
    *           yama:Int
    *           yama:println
    * 
    *        Regular member item fullnames qualify their identifier w/ an owner identifier.
    * 
    *           <path>:<owner>::<identifier>
    * 
    *           math:Vec3::fmt
    * 
    *        Assigner item fullnames suffix their identifier w/ a '=', w/ this applying
    *        both to member and non-member item kinds.
    * 
    *           <path>:<identifier>=
    *           <path>:<owner>::<identifier>=
    * 
    *           p:x=
    *           p:A::x=
    * 
    *        TODO: Keep defining as we add item kinds.
    *        TODO: Explain that we're gonna call the part after the '<path>:' part the 'local name'.
    *        TODO: I think local names are gonna ALWAYS EXCLUDE any overload-related type info from
    *              the name, so operators like '+' will have local names which don't include their
    *              parameter types... maybe.
    */


    /* TODO: The below policy, w/ regards to its impl is somewhat inconsistent w/ regards to API descriptions.
    */

    /* NOTE: When Yama API functions accept pointers to resources (ie. domains, contexts, parcel defs., etc.)
    *        the function DOES NOT TAKE OWNERSHIP of the resource, unless otherwise specified.
    * 
    *        When Yama API functions accept pointers but DOES take ownership of the resource, it is said
    *        to 'steal' the reference (and this should be stated in its description.) This reference stealing
    *        occurs even when the Yama API function call OTHERWISE FAILS (meaning that the resource will be
    *        released by the internals of the failed function call.)
    * 
    *        When Yama API functions return pointers to resources, the function TRANSFERS OWNERSHIP of
    *        the resource to the end-user, unless otherwise specified, or unless the resource is one that
    *        the end-user cannot release (ie. parcels, items, etc.)
    * 
    *        When Yama API functions return pointers but DOES NOT transfer ownership of the resource, it
    *        is said to return a 'borrowed' reference (and this should be stated in its description.)
    */


    /* NOTE: Resources are thread-unsafe unless otherwise specified.
    */

    /* Domains are resources encapsulating data shared between Yama contexts. */
    /* Domains are thread-safe. */
    struct YmDm;

    /* Contexts are resources encapsulating a Yama execution environment. */
    struct YmCtx;

    /* Parcel defs. are resources encapsulating a Yama parcel specification. */
    struct YmParcelDef;

    /* Parcels are resources encapsulating imported Yama parcels. */
    struct YmParcel;

    /* Items are resources encapsulating loaded Yama items. */
    struct YmItem;


    typedef enum : YmUInt8 {
        YmKind_Struct = 0,
        YmKind_Fn,
        YmKind_Method,

        YmKind_Num, /* Enum size. Not a valid type kind. */
    } YmKind;

    /* TODO: ymFmtKind hasn't been unit tested.
    */

    /* Returns the string name of type kind x, or "???" if x is invalid. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtKind(YmKind x);

    /* TODO: ymKind_IsCallable hasn't been unit tested.
    */

    /* Returns if kind x is for callable types. */
    /* Behaviour is undefined if x is invalid. */
    YmBool ymKind_IsCallable(YmKind x);

    /* TODO: ymKind_IsMember hasn't been unit tested.
    */

    /* Returns if kind x is for member types. */
    /* Behaviour is undefined if x is invalid. */
    YmBool ymKind_IsMember(YmKind x);

    /* TODO: ymKind_HasMembers hasn't been unit tested.
    */

    /* Returns if kind x is types which can have members. */
    /* Behaviour is undefined if x is invalid. */
    YmBool ymKind_HasMembers(YmKind x);


    /* TODO: Maybe do this aliasing for ALL (more or less) of our C-strings.
    */

    /* TODO: YmRefSym C-strings are specifically for reference strings in the
    *        constant tables of items.
    * 
    *        Be sure to better explain what these are (remember, constant
    *        tables are in backend) and what their syntax is, and how they
    *        differ from regular fullname strings.
    */

    /* Item Reference Symbol */
    typedef const YmChar* YmRefSym;


    /* Integer ID of an item's reference to another item. */
    typedef YmUInt16 YmRef;

    /* Sentinel ID for no reference. */
#define YM_NO_REF (YmRef(-1))


    /* Index of an item in a parcel. */
    typedef YmUInt16 YmItemIndex;

    /* Sentinel index for no item. */
#define YM_NO_ITEM_INDEX (YmItemIndex(-1))


    /* Index of an item member (ie. in its owner's member list.) */
    typedef YmUInt16 YmMemberIndex;

    /* Number of members. */
    typedef YmMemberIndex YmMembers;

    /* Sentinel index for no member. */
#define YM_NO_MEMBER_INDEX (YmMemberIndex(-1))


    /* Index of a callable item parameter. */
    typedef YmUInt8 YmParamIndex;

    /* Number of parameters. */
    typedef YmParamIndex YmParams;

    /* Sentinel index for no parameter. */
#define YM_NO_PARAM_INDEX (YmParamIndex(-1))

    /* Max number of parameters of callable item may have. */
#define YM_MAX_PARAMS (YmParams(24))


    /* Domain API */

    /* Creates a new Yama domain, returning a pointer to it. */
    struct YmDm* ymDm_Create(void);

    /* Destroys domain dm. */
    /* Behaviour is undefined if dm is invalid. */
    void ymDm_Destroy(struct YmDm* dm);

    /* TODO: ymDm_BindParcelDef overwriting existing bindings hasn't been unit tested.
    */

    /* Binds a parcel (defined by parceldef) to path, replacing any existing binding, returning if successful. */
    /* Fails if path specified is illegal. */
    /* Fails if parceldef fails verification. */
    /* Behaviour is undefined if dm is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmBool ymDm_BindParcelDef(struct YmDm* dm, const YmChar* path, struct YmParcelDef* parceldef);


    /* Context API */

    /* Creates a new Yama context, returning a pointer to it. */
    /* domain is the Yama domain the context is to be associated with. */
    /* Behaviour is undefined if domain is invalid. */
    struct YmCtx* ymCtx_Create(struct YmDm* dm);

    /* Destroys context ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    void ymCtx_Destroy(struct YmCtx* ctx);

    /* Returns (a borrowed reference to) the Yama domain associated with ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmDm* ymCtx_Dm(struct YmCtx* ctx);

    /* Imports the parcel at path, returning a pointer to it, or YM_NIL on failure. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    struct YmParcel* ymCtx_Import(struct YmCtx* ctx, const YmChar* path);

    /* Loads the item with fullname, returning a pointer to it, or YM_NIL on failure. */
    /* Fails if fullname does not describe a loadable item. */
    /* Loading may involve the importing of parcels or the loading of other items. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if fullname (as a pointer) is invalid. */
    struct YmItem* ymCtx_Load(struct YmCtx* ctx, const YmChar* fullname);

    /* TODO: Better explain the specifics of why naturalization is needed.
    * 
    *        Explain that it's needed to legally use parcel/item ptrs across
    *        context boundaries.
    */

    /* TODO: These 'naturalization' fns have not been unit tested yet.
    */

    /* Makes it legal to use parcel with ctx, if it wasn't already. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if parcel is invalid. */
    void ymCtx_NaturalizeParcel(struct YmCtx* ctx, struct YmParcel* parcel);

    /* Makes it legal to use parcel with ctx, if it wasn't already. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if item is invalid. */
    void ymCtx_NaturalizeItem(struct YmCtx* ctx, struct YmItem* item);


    /* Parcel Def. API */

    /* Creates a new Yama parcel def., returning a pointer to it. */
    struct YmParcelDef* ymParcelDef_Create(void);

    /* Destroys parcel def. parceldef. */
    /* Behaviour is undefined if parceldef is invalid. */
    void ymParcelDef_Destroy(struct YmParcelDef* parceldef);

#if __cplusplus
    static_assert(YmKind_Num == 3);
#endif

    /* Adds a new struct to parceldef, returning its index, or YM_NO_ITEM_INDEX on failure. */
    /* Fails if new item's fullname conflicts with an existing declaration. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmItemIndex ymParcelDef_AddStruct(struct YmParcelDef* parceldef, const YmChar* name);

    /* Adds a new function to parceldef, returning its index, or YM_NO_ITEM_INDEX on failure. */
    /* Fails if new item's fullname conflicts with an existing declaration. */
    /* Fails if returnType is illegal. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if returnType (as a pointer) is invalid. */
    YmItemIndex ymParcelDef_AddFn(struct YmParcelDef* parceldef, const YmChar* name, YmRefSym returnType);

    /* Adds a new method to parceldef, owned by owner, returning its index, or YM_NO_ITEM_INDEX on failure. */
    /* Fails if new item's fullname conflicts with an existing declaration. */
    /* Fails if owner is not a valid item index. */
    /* Fails if owner is not allowed to have members. */
    /* Fails if returnType is illegal. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if returnType (as a pointer) is invalid. */
    YmItemIndex ymParcelDef_AddMethod(struct YmParcelDef* parceldef, YmItemIndex owner, const YmChar* name, YmRefSym returnType);

    /* Adds a new parameter to the specified item, returning its index, or YM_NO_PARAM_INDEX on failure. */
    /* Fails if item is not the index of an item in parceldef. */
    /* Fails if item is not callable. */
    /* Fails if name conflicts with an existing parameter. */
    /* Fails if paramType is illegal. */
    /* Fails if adding new param would exceed YM_MAX_PARAMS. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if paramType (as a pointer) is invalid. */
    YmParamIndex ymParcelDef_AddParam(struct YmParcelDef* parceldef, YmItemIndex item, const YmChar* name, YmRefSym paramType);

    /* Explicitly adds to item in parceldef a reference to the item specified by symbol, returning a reference ID, or YM_NO_REF on failure. */
    /* These IDs are not guaranteed to be unique, nor sequential. */
    /* Fails if item is not the index of an item in parceldef. */
    /* Fails if symbol is illegal. */
    /* Fails if Yama API internals are unable to allocate a reference ID. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if symbol (as a pointer) is invalid. */
    YmRef ymParcelDef_AddRef(struct YmParcelDef* parceldef, YmItemIndex item, YmRefSym symbol);


    /* Parcel API */

    /* Returns the path parcel was imported under. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if parcel is invalid. */
    const YmChar* ymParcel_Path(struct YmParcel* parcel);


    /* Item API */

    /* Returns the parcel the item belongs to. */
    /* Behaviour is undefined if item is invalid. */
    YmParcel* ymItem_Parcel(struct YmItem* item);

    /* Returns the fullname of item. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if item is invalid. */
    const YmChar* ymItem_Fullname(struct YmItem* item);
    
    /* Returns the kind of item. */
    /* Behaviour is undefined if item is invalid. */
    YmKind ymItem_Kind(struct YmItem* item);

    /* Returns the owner of item, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    YmItem* ymItem_Owner(struct YmItem* item);

    /* Returns the number of members item has, if any. */
    /* Behaviour is undefined if item is invalid. */
    YmMembers ymItem_Members(struct YmItem* item);

    /* Returns the member at member in item, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    YmItem* ymItem_MemberByIndex(struct YmItem* item, YmMemberIndex member);

    /* Returns the member under name in item, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmItem* ymItem_MemberByName(struct YmItem* item, const YmChar* name);

    /* Returns the return type of the item, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    YmItem* ymItem_ReturnType(struct YmItem* item);

    /* Returns the number of parameters the item has, if any. */
    /* Behaviour is undefined if item is invalid. */
    YmParams ymItem_Params(struct YmItem* item);

    /* Returns the name of param in item, or YM_NIL on failure. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if item is invalid. */
    const YmChar* ymItem_ParamName(struct YmItem* item, YmParamIndex param);

    /* Returns the type of param in item, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    YmItem* ymItem_ParamType(struct YmItem* item, YmParamIndex param);

    /* Returns the item referenced under reference, or YM_NIL on failure. */
    /* Behaviour is undefined if item is invalid. */
    YmItem* ymItem_Ref(struct YmItem* item, YmRef reference);

    /* Returns if the reference ID item has for its reference to referenced, or YM_NO_REF if item doesn't reference referenced. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if referenced is invalid. */
    YmRef ymItem_FindRef(struct YmItem* item, struct YmItem* referenced);

    /* Returns if the conversion 'from -> to' is legal. */
    /* coercion specifies if 'from -> to' is a legal coercion (aka. implicit conversion.) */
    /* Behaviour is undefined if from is invalid. */
    /* Behaviour is undefined if to is invalid. */
    YmBool ymItem_Converts(struct YmItem* from, struct YmItem* to, YmBool coercion);


    /* Parcel Iterator API */

    /* TODO: Maybe replace this API w/ a ymCtx_ForEachParcel fn which takes in a fn ptr and
    *        a void* and calls said fn ptr w/ it for each imported parcel, letting the end-user
    *        use the fn to observe each one.
    * 
    *        Then, in yama++, we can have the ym::Context abstraction be able to provide the
    *        end user w/ a std::vector<ym::Parcel> upon query, constructed in the backend w/
    *        ymCtx_ForEachParcel.
    * 
    *        This would make thread-safety easier to deal w/, as ymCtx_ForEachParcel impl can
    *        lock domain state for the duration of ymCtx_ForEachParcel.
    */

    /* NOTE: Parcel iterator state is invalidated EVEN IF all that changes is an adding
    *        of a new imported parcel.
    */

    /* (Re)starts parcel iteration, revalidating parcel iterator. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if ctx is invalid. */
    void ymParcelIter_Start(struct YmCtx* ctx);

    /* (Re)starts parcel iteration, starting from startFrom, revalidating parcel iterator. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if startFrom is invalid. */
    /* Behaviour is undefined if startFrom was not imported from ctx. */
    void ymParcelIter_StartFrom(struct YmCtx* ctx, struct YmParcel* startFrom);

    /* Advances the parcel iterator n times. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if parcel iterator state is invalid. */
    void ymParcelIter_Advance(size_t n);

    /* Queries the parcel iterator, returning a pointer to the current parcel, or YM_NIL on failure. */
    /* Fail indicates that the parcel iterator is past-the-end, or iteration hasn't started. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if parcel iterator state is invalid. */
    struct YmParcel* ymParcelIter_Get(void);

    /* Returns if parcel iterator is past-the-end. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if parcel iterator state is invalid. */
    YmBool ymParcelIter_Done(void);


    /* Item Iterator API */

    /* TODO */


    /* Internals */

    /* TODO */
}


#endif

