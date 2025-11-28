

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


    /* TODO: Below description doesn't really work w/, for example, ymCtx_Dm.
    */

    /* NOTE: When Yama API functions accept pointers to resources (ie. domains, contexts, parcel defs., etc.)
    *        the function DOES NOT TAKE OWNERSHIP of the resource, unless otherwise specified.
    * 
    *        When Yama API functions return pointers to resources, the function TRANSFERS OWNERSHIP of
    *        the resource to the end-user, unless otherwise specified, or unless the resource is one that
    *        the end-user cannot release (ie. parcels, items, etc.)
    */


    /* TODO: Need to later include 16-bit value for specifying an ID for the item argument tuple
    *        parameterizing it in case of instantiated generic types.
    * 
    *        This'll mean bumping up sizeof(YmGID) to 8.
    */

    typedef YmUInt16 YmPID; /* Domain-wide stable ID of an imported parcel. */
    typedef YmUInt16 YmLID; /* Local ID of an item within its parcel. */
    typedef YmUInt32 YmGID; /* Domain-wide stable global ID of a loaded item, combining its LID and its parcel's PID. */

#define YM_NO_LID (YmLID(-1))

    /* Returns a GID created from pid and lid. */
#define ymGID(pid, lid) (YmGID(pid) << 16 | YmGID(lid))

    /* Extracts PID component of gid. */
#define ymGID_PID(gid) (YmPID(gid >> 16))

    /* Extracts LID component of gid. */
#define ymGID_LID(gid) (YmLID(gid))


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
        YmKind_Fn = 0,

        YmKind_Num, /* Enum size. Not a valid type kind. */
    } YmKind;

    /* TODO: ymFmtKind hasn't been unit tested. */

    /* Returns the string name of type kind x, or "???" if x is invalid. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtKind(YmKind x);


    /* Constant table entry types. */
    typedef enum : YmUInt8 {
        YmConstType_Int = 0,
        YmConstType_UInt,
        YmConstType_Float,
        YmConstType_Bool,
        YmConstType_Rune,

        YmConstType_Ref,

        YmConstType_Num, /* Enum size. Not a valid constant table entry type. */
    } YmConstType;

    /* TODO: ymFmtConstType hasn't been unit tested. */

    /* Returns the string name of constant type x, or "???" if x is invalid. */
    /* The memory of the returned string is static and is valid for the lifetime of the process. */
    const YmChar* ymFmtConstType(YmConstType x);

    /* Index of a constant table entry. */
    typedef YmUInt8 YmConst;

    /* Max legal constant index. */
#define YM_MAX_CONST (YmConst(-2))

    /* Sentinel index for no constant. */
#define YM_NO_CONST (YmConst(-1))


    /* Domain API */

    /* Creates a new Yama domain, returning a pointer to it. */
    struct YmDm* ymDm_Create(void);

    /* Destroys domain dm. */
    /* Behaviour is undefined if dm is invalid. */
    void ymDm_Destroy(struct YmDm* dm);

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

    /* Destroys context ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    void ymCtx_Destroy(struct YmCtx* ctx);

    /* Returns the Yama domain associated with ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmDm* ymCtx_Dm(struct YmCtx* ctx);

    /* Imports the parcel at path, returning a pointer to it, or YM_NIL on failure. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    struct YmParcel* ymCtx_Import(struct YmCtx* ctx, const YmChar* path);

    /* Imports the parcel under pid, returning a pointer to it, or YM_NIL on failure. */
    /* The parcel must first have already been imported by this context, or another under the same domain. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmParcel* ymCtx_ImportByPID(struct YmCtx* ctx, YmPID pid);

    /* Loads the item with fullname, returning a pointer to it, or YM_NIL on failure. */
    /* Loading may involve the importing of parcels or the loading of other items. */
    /* Fails if fullname does not describe a loadable item. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if fullname (as a pointer) is invalid. */
    struct YmItem* ymCtx_Load(struct YmCtx* ctx, const YmChar* fullname);

    /* TODO: Unit test LoadByGID_AutoImportsDeps is incomplete.
    */

    /* Loads the item under gid, returning a pointer to it, or YM_NIL on failure. */
    /* Loading may involve the importing of parcels or the loading of other items. */
    /* Fails if gid does not describe a loadable item. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmItem* ymCtx_LoadByGID(struct YmCtx* ctx, YmGID gid);


    /* Parcel Def. API */

    /* Creates a new Yama parcel def., returning a pointer to it. */
    struct YmParcelDef* ymParcelDef_Create(void);

    /* Destroys parcel def. parceldef. */
    /* Behaviour is undefined if parceldef is invalid. */
    void ymParcelDef_Destroy(struct YmParcelDef* parceldef);

    /* Adds a new function to parceldef, returning its LID, or YM_NO_LID on failure. */
    /* Fails if function's fullname conflicts with an existing declaration. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmLID ymParcelDef_FnItem(struct YmParcelDef* parceldef, const YmChar* name);

    /* Given parceldef and an item therein, returns the index of the int constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_IntConst(struct YmParcelDef* parceldef, YmLID item, YmInt value);

    /* Given parceldef and an item therein, returns the index of the uint constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_UIntConst(struct YmParcelDef* parceldef, YmLID item, YmUInt value);

    /* Given parceldef and an item therein, returns the index of the float constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_FloatConst(struct YmParcelDef* parceldef, YmLID item, YmFloat value);

    /* Given parceldef and an item therein, returns the index of the bool constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_BoolConst(struct YmParcelDef* parceldef, YmLID item, YmBool value);

    /* Given parceldef and an item therein, returns the index of the runic constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_RuneConst(struct YmParcelDef* parceldef, YmLID item, YmRune value);

    /* Given parceldef and an item therein, returns the index of the item reference constant with value, or YM_NO_CONST on failure. */
    /* If no existing constant could be found, a new constant will attempt to be added. */
    /* Fails if item is not the LID of an item in parceldef. */
    /* Fails if adding a new item would require an LID exceeding YM_MAX_CONST. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmConst ymParcelDef_RefConst(struct YmParcelDef* parceldef, YmLID item, const YmChar* symbol);


    /* Parcel API */

    /* Returns the PID of parcel. */
    /* Behaviour is undefined if parcel is invalid. */
    YmPID ymParcel_PID(struct YmParcel* parcel);

    /* Returns the path parcel was imported under. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if parcel is invalid. */
    const YmChar* ymParcel_Path(struct YmParcel* parcel);


    /* Item API */

    /* Returns the GID of item. */
    /* Behaviour is undefined if item is invalid. */
    YmGID ymItem_GID(struct YmItem* item);

    /* Returns the fullname of item. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if item is invalid. */
    const YmChar* ymItem_Fullname(struct YmItem* item);
    
    /* Returns the kind of item. */
    /* Behaviour is undefined if item is invalid. */
    YmKind ymItem_Kind(struct YmItem* item);

    /* Returns the size of the constant table of item. */
    /* Behaviour is undefined if item is invalid. */
    YmWord ymItem_Consts(struct YmItem* item);

    /* Returns the type of the constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    YmConstType ymItem_ConstType(struct YmItem* item, YmConst index);

    /* TODO: MAYBE revise below ymItem_***Const fns to return an indeterminate value
    *        upon failure (due to invalid index or const type mismatch), rather than
    *        having them be UB.
    * 
    *        These would be more testable, and less able to crash end-user's code, but
    *        MIGHT cause subtle bugs in end-user code due to them failing quietly.
    */

    /* Returns the value of int constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not an int constant. */
    YmInt ymItem_IntConst(struct YmItem* item, YmConst index);

    /* Returns the value of uint constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not an uint constant. */
    YmUInt ymItem_UIntConst(struct YmItem* item, YmConst index);

    /* Returns the value of float constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not a float constant. */
    YmFloat ymItem_FloatConst(struct YmItem* item, YmConst index);

    /* Returns the value of bool constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not a bool constant. */
    YmBool ymItem_BoolConst(struct YmItem* item, YmConst index);

    /* Returns the value of runic constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not a runic constant. */
    YmRune ymItem_RuneConst(struct YmItem* item, YmConst index);

    /* Returns the value of item reference constant at index in item. */
    /* Behaviour is undefined if item is invalid. */
    /* Behaviour is undefined if no constant at index. */
    /* Behaviour is undefined if constant at index is not an item reference constant. */
    YmItem* ymItem_RefConst(struct YmItem* item, YmConst index);


    /* Parcel Iterator API */

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
    void ymParcelIter_Advance(YmWord n);

    /* Queries the parcel iterator, returning a pointer to the current parcel, or YM_NIL on failure. */
    /* Fail indicates that the parcel iterator is past-the-end, or iteration hasn't started. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if parcel iterator state is invalid. */
    struct YmParcel* ymParcelIter_Get();

    /* Returns if parcel iterator is past-the-end. */
    /* Parcel iterator state is thread-local. */
    /* Parcel iterator state is invalidated when the set of imported parcels in the traversed context changes. */
    /* Parcel iterator state is invalidated if traversed context is deinitialized. */
    /* Behaviour is undefined if parcel iterator state is invalid. */
    YmBool ymParcelIter_Done();


    /* Item Iterator API */

    /* TODO */


    /* Internals */

    /* TODO */
}


#endif

