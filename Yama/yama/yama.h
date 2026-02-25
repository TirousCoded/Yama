

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


    /* TODO: I'm not 100% sure if we'll have domain type graph GC in our impl, though
    *        it could still be a cool thing to have in something like an asset manager
    *        for a game or something.
    */

    /* TODO: Quick-n'-dirty idea about managing mem. of domain type graph via a GC:
    *           - Each type gets a ref count for the number of refs it has by things
    *             external to the domain/type system.
    *           - GC has a remembered-set-esque data structure for the root set.
    *               - Types are added when their extern. ref count becomes non-zero.
    *               - Types w/ zero extern. refs get removed from this set during
    *                 collection cycle scanning of this set.
    *           - We 100% know that the type ref graph's internal refs are IMMUTABLE,
    *             meaning the graph can grow, and shrink due to GC, but beyond that
    *             it will NOT MUTATE.
    *           - From startup, or after latest GC cycle, we 100% know that there's
    *             nothing to cleanup (and so no GC cycle needed) UNTIL the extern.
    *             ref count of a GC type gets decremented to zero, at which point
    *             there MIGHT BE newly dead types in the graph.
    *           - We'd need to have a dedicated thread for performing GC cycles, w/
    *             this in turn meaning a stop-the-world or generational mark-and-sweep
    *             system might be best w/ regards to overhead.
    *           - We could have it decide to schedule a new GC cycle when the amount
    *             of memory allocated exceeds, say, 150% or whatever, of what it was
    *             after the last GC cycle.
    *               - This also taking into consideration the detail about us ONLY
    *                 needing to think about new GC cycles once a type's extern. ref
    *                 count is decremented to zero, after last GC cycle.
    * 
    *           - The more I think about the different possible scenarios in this system,
    *             the more I discern that: For this system's impl, we should ALWAYS have
    *             fns which query a type via another type (eg. getting the return type
    *             type from a fn type) result in the returned type pointer having had an
    *             extern. ref count increment made to it, and thus NEVER returning a
    *             borrowed reference.
    *               - The reason for this is that if a borrowed referenced type relies
    *                 on another type being a *root* to survive, if a GC cycle occurs and
    *                 said other type isn't a root, it could lead to memory corruption
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

    /* TODO: Actually, I think I prefer to call them 'files', to specifically keep
    *        concept names IN LINE W/ the filesystem.
    */
    /* TODO: Make it so that we call each file/directory name-like part of the path
    *        a 'module' (or 'submodule'.)
    * 
    *        This naming convention is meant to abstract the import system away from
    *        the filesystem.
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
    *        path part of type decorated names.
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

    /* NOTE: When loading a Yama type a 'fullname' is used to specify the type to load.
    * 
    *        Regular non-member type fullnames are simply identifiers paired w/ an import
    *        path specifying their parcel, seperated by a ':'.
    * 
    *           <path>:<identifier>
    * 
    *           yama:Int
    *           yama:println
    * 
    *        Regular member type fullnames qualify their identifier w/ an owner identifier.
    * 
    *           <path>:<owner>::<identifier>
    * 
    *           math:Vec3::fmt
    * 
    *        Assigner type fullnames suffix their identifier w/ a '=', w/ this applying
    *        both to member and non-member type kinds.
    * 
    *           <path>:<identifier>=
    *           <path>:<owner>::<identifier>=
    * 
    *           p:x=
    *           p:A::x=
    * 
    *        TODO: Keep defining as we add type kinds.
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
    *        the end-user cannot release (ie. parcels, types, etc.)
    * 
    *        When Yama API functions return pointers but DOES NOT transfer ownership of the resource, it
    *        is said to return a 'borrowed' reference (and this should be stated in its description.)
    */


    /* TODO: Maybe impl 'Alias' and 'MemberAlias' kinds. These types would encapsulate refs to
    *        other types, and upon load instead of returning these types, the loader will instead
    *        FORWARD to the ref'd type, and return that instead.
    * 
    *        As part of this we'd be best to have a way to load w/out forwarding aliases like this,
    *        instead having load return the actual alias type itself.
    */
    /* TODO: If we impl MemberAlias kind, then we can make '$Self' a special member alias that gets
    *        added up-front to ALL non-member types, and likewise we can make '$T' type params be
    *        impl'd as special MemberAlias(es) as well.
    * 
    *        In both these cases we'd likely need special semantics for these cases, as '$Self' and
    *        '$T' have special semantics that don't necessarily apply elsewhere.
    * 
    *        Also, this would mean revising '$' prefix syntax to be a more general way for accessing
    *        member types within the type who's ref syms are being evaluated.
    */
    /* TODO: In the future we should try to add covariance/contravariance w/ regards to protocols,
    *        allowing conforming types to have param/return types of methods to not necessarily be
    *        the exact same type as in the protocol, but instead just have the types used have
    *        compatible value sets.
    *        
    *        This is a feature that'll require complexity to be added to our system, and we need this
    *        stuff to be kept in mind as we impl other parts of our API.
    */
    /* TODO: Currently, type params are presumed during loading to NOT be protocols, even if args
    *        to said types are protocols.
    * 
    *        This is a detail that'll need to be considered if we impl covariance/contravariance
    *        w/ protocol conformance checking.
    */
    /* TODO: If we impl MemberAlias kind, then this can help us more easily impl $Self nuance w/
    *        regards to generic type constraint checking, where $Self's meaning CHANGES from its
    *        usual meaning.
    * 
    *        MemberAlias kind can help as if we can query the alias type itself W/OUT forwarding to
    *        the type ref'd by it, then it can let us identify $Self during eval of the expr tree
    *        of a ref sym.
    * 
    *        What we can do is perform a parallel depth-first traversal of the expr trees of the two
    *        sides of a constraint check of an arg, in which we traverse the constraint's expr tree,
    *        and at each step we discern the corresponding in the arg type's fullname's expr tree,
    *        and then we can perform appropriate checks therein.
    * 
    *        Still not 100% how this would exactly work, but MemberAlias would enable us such that
    *        I think we'd be able to figure something out, and be left w/ a system that enables us
    *        to do this and more advanced static analysis w/ greater flexibility.
    */


    /* TODO: Currently, our loading system error messages SUCK, and are formatting-wise vary
    *        inconsistent, so improve them!
    * 
    *        Also, improve protocol conformance error messages, as right now they don't explain at
    *        all WHY a given type failed to conform to a protocol!
    */
    /* TODO: Refactor our loading system code, as it's a BIG MESS at the moment!
    * 
    *        One of the big things is the need to make the impl more consistently stable, as right
    *        now I worry it might have a bunch of edge cases in which it crashes or behaves in some
    *        unwanted fashion (such as raising *internal errors*.)
    */


    /* TODO: Currently, we lack testing for whether callsig specifiers in direct/direct loads normalize
    *        as expected.
    * 
    *        Also, we lack tests for whether callsig specifiers can properly handle nuanced things like
    *        $Self or type params appearing in their param/return type parts.
    * 
    *        Also, our indirect load tests for callsig specifiers is lacking in thorough failure testing.
    */


    /* TODO: Revise our policy on thread-safety to be per-function.
    */
    /* NOTE: Resources are thread-unsafe unless otherwise specified.
    */

    /* NOTE: There are three types of resources:
    *           1) 'ARC' resources use atomic reference counting.
    *           2) 'RC' resources use non-atomic reference counting.
    *               * Atomic above implies thread-safety for the reference counting, but
    *                 not necessarily for the resource itself.
    *           3) 'view' are views to resources who's lifetime are managed internally
    *              and not by the end-user via reference counting.
    */

    /* Domains are ARC resources encapsulating data shared between Yama contexts. */
    /* Domains are thread-safe. */
    struct YmDm;

    /* Contexts are ARC resources encapsulating a Yama execution environment. */
    struct YmCtx;

    /* Parcel defs. are ARC resources encapsulating a Yama parcel definition. */
    struct YmParcelDef;

    /* Parcels are view resources encapsulating imported Yama parcels. */
    struct YmParcel;

    /* Types are view resources encapsulating loaded Yama types. */
    struct YmType;


    typedef enum : YmUInt8 {
        YmKind_Struct = 0,
        YmKind_Protocol,
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


    /* Reference Count */
    typedef YmUInt32 YmRefCount;


    /* TODO: Maybe do this aliasing for ALL (more or less) of our C-strings.
    */
    /* TODO: YmRefSym C-strings are specifically for reference strings in the
    *        constant tables of types.
    * 
    *        Be sure to better explain what these are (remember, constant
    *        tables are in backend) and what their syntax is, and how they
    *        differ from regular fullname strings.
    * 
    *        This includes explaining '%here' when we bring it to the frontend.
    */
    /* TODO: Explain how the special 'Self' keyword works. Included in this,
    *        explain things like the quark about how a 'Self' ref for a function
    *        will resolve to the function type itself (as it has no owner, and
    *        so 'Self' behaves the same as it does for other owner-less kinds
    *        like struct/protocol.)
    */

    /* Path Specifier */
    typedef const YmChar* YmPath;
    
    /* Fullname Specifier */
    typedef const YmChar* YmFullname;

    /* Reference Symbol Specifier */
    typedef const YmChar* YmRefSym;


    /* Integer ID of a type's reference to another type. */
    typedef YmUInt16 YmRef;

    /* Sentinel ID for no reference. */
#define YM_NO_REF (YmRef(-1))


    /* Index of a type in a parcel. */
    typedef YmUInt16 YmTypeIndex;

    /* Sentinel index for no type. */
#define YM_NO_TYPE_INDEX (YmTypeIndex(-1))


    /* Index of a type member (ie. in its owner's member list.) */
    typedef YmUInt16 YmMemberIndex;

    /* Number of members. */
    typedef YmMemberIndex YmMembers;

    /* Sentinel index for no member. */
#define YM_NO_MEMBER_INDEX (YmMemberIndex(-1))


    /* Index of a type parameter. */
    typedef YmUInt8 YmTypeParamIndex;

    /* Number of type parameters. */
    typedef YmTypeParamIndex YmTypeParams;

    /* Sentinel index for no type parameter. */
#define YM_NO_TYPE_PARAM_INDEX (YmTypeParamIndex(-1))

    /* Max number of type parameters a type may have. */
#define YM_MAX_TYPE_PARAMS (YmTypeParams(25))


    /* Index of a callable type's parameter. */
    typedef YmUInt8 YmParamIndex;

    /* Number of parameters. */
    typedef YmParamIndex YmParams;

    /* Sentinel index for no parameter. */
#define YM_NO_PARAM_INDEX (YmParamIndex(-1))

    /* Max number of parameters a callable type may have. */
#define YM_MAX_PARAMS (YmParams(24))


    /* A callback function used to perform call behaviour (ie. of a fn/method/etc.) */
    /* ctx is the context the call behaviour is to be executed with, and is guaranteed to not be YM_NIL. */
    /* user is a pointer used to expose callback function to external data. */
    typedef void (*YmCallBhvrCallbackFn)(
        struct YmCtx* ctx,
        void* user);

    /* No-op YmCallBhvrCallbackFn. */
    void ymInertCallBhvrFn(struct YmCtx*, void*);


    /* Domain API */

    /* Creates a new Yama domain, returning a pointer to it. */
    /* The new domain will start off with a ref count of 1. */
    struct YmDm* ymDm_Create(void);

    /* Increments the ref count of domain dm. */
    /* Returns old ref count value of dm. */
    /* Behaviour is undefined if dm is invalid. */
    YmRefCount ymDm_Secure(struct YmDm* dm);

    /* Decrements the ref count of domain dm, destroying dm if it reaches 0. */
    /* Returns old ref count value of dm. */
    /* Behaviour is undefined if dm is invalid. */
    YmRefCount ymDm_Release(struct YmDm* dm);

    /* Returns the ref count of domain dm. */
    /* Behaviour is undefined if dm is invalid. */
    YmRefCount ymDm_RefCount(struct YmDm* dm);

    /* Binds a parcel (defined by parceldef) to path, replacing any existing binding, returning if successful. */
    /* Fails if path specified is illegal. */
    /* Fails if parceldef fails verification. */
    /* Behaviour is undefined if dm is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmBool ymDm_BindParcelDef(struct YmDm* dm, YmPath path, struct YmParcelDef* parceldef);

    /* NOTE: Imported parcels have a set of import paths depended-upon by its types. However, these paths
    *        may not always line up w/ the paths in the actual import environment needed to reach the desired
    *        parcels. In these situations 'redirects' are used to redirect part or all of a dependency path
    *        to correct it.
    * 
    *        Redirects are contextual: each use a 'subject' path prefix to define the parcels subject to the
    *        redirect (ie. for its dependency paths) as the set of parcels who's import path contains the
    *        prefix.
    * 
    *        Each redirect defines 'before' and 'after' path prefixes, the former defining the prefixes of
    *        dependency paths which get substituted w/ the 'after' path prefix.
    * 
    *        Given a parcel w/ an import path which contains the subject path prefix of two redirects, and
    *        the 'before' path prefix of the two are identical, the one selected is the one w/ the more
    *        specific import path prefix (which'll also be the longer of the two.)
    * 
    *        Given a reference symbol containing the 'before' path prefixes of two redirects in the set of
    *        its parcel, the one selected is the one w/ the more specific import path prefix (which'll also
    *        be the longer of the two.)
    */

    /* TODO: When we add things like importing parcels from filesystem, be sure to add redirect tests that
    *        ensure that redirects can affect them.
    */

    /* TODO: Overwriting redirects, plus their 'locking' semantics, presents a number of possible scenarios
    *        in which confusing redirect behaviour could occur due to the amount of implicit respecting/ignoring
    *        of redirects going on.
    * 
    *        For example, consider the following series of events:
    *           1) Add redirect 'p': 'alt' -> 'q1'.
    *           2) Load 'p:A'.
    *           3) Add redirect 'p': 'alt' -> 'q2'.
    *           4) Load 'p:B'.
    *           5) Load 'p/a:C'.
    * 
    *        Above, after the overwrite of redirect at #3, p:B load will treat 'alt' differently than how
    *        the load of 'p/a:C' will treat it, w/ this occurring implicitly. This behaviour could be confusing,
    *        so again, I'm not 100% sure about redirect overwriting, but we'll keep it for now.
    */

    /* TODO: I don't think our redirect overwriting tests cover a situation where an NEEDED redirect was not
    *        added until AFTER the import that needed it occurred. What should happen then?
    */

    /* TODO: Is it okay if before/after in a redirect is '%here'? Do we need explicit tests for this? Does
    *        it present problems that warrant it being forbidden?
    */

    /* Adds a new redirect, or overwrites an existing one, returning if successful. */
    /* The redirects of a parcel are locked for it upon import. */
    /* subject defines the path prefix of parcels subject to the redirect. */
    /* before defines the path prefix being redirected. */
    /* after defines the path prefix before is substituted with. */
    /* Fails if subject specified is illegal. */
    /* Fails if before specified is illegal. */
    /* Fails if after specified is illegal. */
    /* Behaviour is undefined if dm is invalid. */
    /* Behaviour is undefined if subject (as a pointer) is invalid. */
    /* Behaviour is undefined if before (as a pointer) is invalid. */
    /* Behaviour is undefined if after (as a pointer) is invalid. */
    YmBool ymDm_AddRedirect(struct YmDm* dm, YmPath subject, YmPath before, YmPath after);


    /* Context API */

    /* Creates a new Yama context, returning a pointer to it. */
    /* The new context will start off with a ref count of 1. */
    /* domain is the Yama domain the context is to be associated with. */
    /* Behaviour is undefined if domain is invalid. */
    struct YmCtx* ymCtx_Create(struct YmDm* dm);

    /* Increments the ref count of context ctx. */
    /* Returns old ref count value of ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    YmRefCount ymCtx_Secure(struct YmCtx* ctx);

    /* Decrements the ref count of context ctx, destroying ctx if it reaches 0. */
    /* Returns old ref count value of ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    YmRefCount ymCtx_Release(struct YmCtx* ctx);

    /* Returns the ref count of context ctx. */
    /* Behaviour is undefined if ctx is invalid. */
    YmRefCount ymCtx_RefCount(struct YmCtx* ctx);

    /* Returns the Yama domain associated with ctx. */
    /* Does not increment the returned domain's ref count. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmDm* ymCtx_Dm(struct YmCtx* ctx);

    /* Imports the parcel at path, returning a pointer to it, or YM_NIL on failure. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if path (as a pointer) is invalid. */
    struct YmParcel* ymCtx_Import(struct YmCtx* ctx, YmPath path);

    /* Loads the type with fullname, returning a pointer to it, or YM_NIL on failure. */
    /* Fails if fullname does not describe a loadable type. */
    /* Loading may involve the importing of parcels or the loading of other types. */
    /* Behaviour is undefined if ctx is invalid. */
    /* Behaviour is undefined if fullname (as a pointer) is invalid. */
    struct YmType* ymCtx_Load(struct YmCtx* ctx, YmFullname fullname);

    /* Quickly loads yama:None. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdNone(struct YmCtx* ctx);

    /* Quickly loads yama:Int. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdInt(struct YmCtx* ctx);

    /* Quickly loads yama:UInt. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdUInt(struct YmCtx* ctx);

    /* Quickly loads yama:Float. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdFloat(struct YmCtx* ctx);

    /* Quickly loads yama:Bool. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdBool(struct YmCtx* ctx);

    /* Quickly loads yama:Rune. */
    /* Behaviour is undefined if ctx is invalid. */
    struct YmType* ymCtx_LdRune(struct YmCtx* ctx);

    /* TODO: Better explain the specifics of why naturalization is needed.
    * 
    *        Explain that it's needed to legally use parcel/type ptrs across
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
    /* Behaviour is undefined if type is invalid. */
    void ymCtx_NaturalizeType(struct YmCtx* ctx, struct YmType* type);


    /* Parcel Def. API */

    /* TODO: Disallow identifier names input into fns below from containing '%', '$', ' ', '\n', etc.
    *        Also be sure to test for things like 'abc/%here' and 'abc::$m' illegality.
    */

    /* Creates a new Yama parcel def., returning a pointer to it. */
    /* The new parcel def. will start off with a ref count of 1. */
    struct YmParcelDef* ymParcelDef_Create(void);

    /* Increments the ref count of parcel def. parceldef. */
    /* Returns old ref count value of parceldef. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmRefCount ymParcelDef_Secure(struct YmParcelDef* parceldef);

    /* Decrements the ref count of parcel def. parceldef, destroying parceldef if it reaches 0. */
    /* Returns old ref count value of parceldef. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmRefCount ymParcelDef_Release(struct YmParcelDef* parceldef);

    /* Returns the ref count of parcel def. parceldef. */
    /* Behaviour is undefined if parceldef is invalid. */
    YmRefCount ymParcelDef_RefCount(struct YmParcelDef* parceldef);

#if __cplusplus
    static_assert(YmKind_Num == 4);
#endif

    /* Adds a new struct to parceldef, returning its index, or YM_NO_TYPE_INDEX on failure. */
    /* Fails if new type's fullname conflicts with an existing declaration. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmTypeIndex ymParcelDef_AddStruct(
        struct YmParcelDef* parceldef,
        const YmChar* name);

    /* Adds a new protocol to parceldef, returning its index, or YM_NO_TYPE_INDEX on failure. */
    /* Fails if new type's fullname conflicts with an existing declaration. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmTypeIndex ymParcelDef_AddProtocol(
        struct YmParcelDef* parceldef,
        const YmChar* name);

    /* TODO: Our unit tests don't cover whether callBehaviour uploads correctly, as that's currently
    *        unobservable.
    * 
    *        My plan is to make this observable behaviourally later on when we impl our object system,
    *        (ie. we observe via actually invoking the call behaviour,) so I'm making this TODO here
    *        as a reminder for us to write these tests.
    */

    /* TODO: Maybe make callBehaviour being YM_NIL not UB, but instead a proper non-fatal error.
    */

    /* Adds a new function to parceldef, returning its index, or YM_NO_TYPE_INDEX on failure. */
    /* Fails if new type's fullname conflicts with an existing declaration. */
    /* Fails if returnType is illegal. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if returnType (as a pointer) is invalid. */
    /* Behaviour is undefined if callBehaviour is invalid. */
    YmTypeIndex ymParcelDef_AddFn(
        struct YmParcelDef* parceldef,
        const YmChar* name,
        YmRefSym returnType,
        YmCallBhvrCallbackFn callBehaviour,
        void* callBehaviourData);

    /* TODO: For now, we'll forbid protocols from having regular methods, but we may change this in the future,
    *        in which case the description for below will need to be updated.
    */

    /* Adds a new method to parceldef, owned by owner, returning its index, or YM_NO_TYPE_INDEX on failure. */
    /* Fails if new type has a name conflict with an existing type, type parameter, or 'Self'. */
    /* Fails if owner is not a valid type index. */
    /* Fails if owner is not allowed to have members. */
    /* Fails if owner is a protocol. */
    /* Fails if returnType is illegal. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if returnType (as a pointer) is invalid. */
    /* Behaviour is undefined if callBehaviour is invalid. */
    YmTypeIndex ymParcelDef_AddMethod(
        struct YmParcelDef* parceldef,
        YmTypeIndex owner,
        const YmChar* name,
        YmRefSym returnType,
        YmCallBhvrCallbackFn callBehaviour,
        void* callBehaviourData);

    /* NOTE: Herein, a 'method requirement' or 'method req.' is the name given to special methods defined for
    *        protocols which define one of the methods defining the interface of the protocol.
    * 
    *        These behave otherwise like regular methods, but have this special additional role.
    */

    /* Adds a new method req. to parceldef, owned by owner, returning its index, or YM_NO_TYPE_INDEX on failure. */
    /* Fails if new type has a name conflict with an existing type, type parameter, or 'Self'. */
    /* Fails if owner is not a valid type index. */
    /* Fails if owner is not a protocol. */
    /* Fails if returnType is illegal. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if returnType (as a pointer) is invalid. */
    /* Behaviour is undefined if callBehaviour is invalid. */
    YmTypeIndex ymParcelDef_AddMethodReq(
        struct YmParcelDef* parceldef,
        YmTypeIndex owner,
        const YmChar* name,
        YmRefSym returnType);

    /* NOTE: Right now type params are treated as pseudo-types, w/ them having certain characteristics
    *        of types (ie. they can have name conflicts w/ them,) while not having others (ie. they're
    *        literally not types.)
    * 
    *        This may change in the future.
    * 
    *        The name 'Self' is reserved as another pseudo-type, for letting types refer to the owner
    *        type of a owner/members group.
    */

    /* Adds a new type parameter to the specified type, returning its index, or YM_NO_TYPE_PARAM_INDEX on failure. */
    /* type will not be able to load if constraintType doesn't specify a protocol. */
    /* Fails if new type has a name conflict with an existing type, type parameter, or 'Self'. */
    /* Fails if type is not the index of a type in parceldef. */
    /* Fails if type is a member. */
    /* Fails if constraintType is illegal. */
    /* Fails if adding new type parameter would exceed YM_MAX_TYPE_PARAMS. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if constraintType (as a pointer) is invalid. */
    YmTypeParamIndex ymParcelDef_AddTypeParam(
        struct YmParcelDef* parceldef,
        YmTypeIndex type,
        const YmChar* name,
        YmRefSym constraintType);

    /* Adds a new parameter to the specified type, returning its index, or YM_NO_PARAM_INDEX on failure. */
    /* Fails if type is not the index of a type in parceldef. */
    /* Fails if type is not callable. */
    /* Fails if name conflicts with an existing parameter. */
    /* Fails if paramType is illegal. */
    /* Fails if adding new parameter would exceed YM_MAX_PARAMS. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    /* Behaviour is undefined if paramType (as a pointer) is invalid. */
    YmParamIndex ymParcelDef_AddParam(
        struct YmParcelDef* parceldef,
        YmTypeIndex type,
        const YmChar* name,
        YmRefSym paramType);

    /* Explicitly adds to type in parceldef a reference to the type specified by symbol, returning a reference ID, or YM_NO_REF on failure. */
    /* These IDs are not guaranteed to be unique, nor sequential. */
    /* Fails if type is not the index of a type in parceldef. */
    /* Fails if symbol is illegal. */
    /* Fails if Yama API internals are unable to allocate a reference ID. */
    /* Behaviour is undefined if parceldef is invalid. */
    /* Behaviour is undefined if symbol (as a pointer) is invalid. */
    YmRef ymParcelDef_AddRef(
        struct YmParcelDef* parceldef,
        YmTypeIndex type,
        YmRefSym symbol);


    /* Parcel API */

    /* Returns the path parcel was imported under. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if parcel is invalid. */
    YmPath ymParcel_Path(struct YmParcel* parcel);


    /* Type API */

    /* TODO: Add in diagnostic fmt fns for getting nicely formatted string reprs of
     *       internal constant tables, and type dependency graphs.
    */

    /* Returns the parcel the type belongs to. */
    /* Behaviour is undefined if type is invalid. */
    YmParcel* ymType_Parcel(struct YmType* type);

    /* Returns the fullname of type. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if type is invalid. */
    YmFullname ymType_Fullname(struct YmType* type);
    
    /* Returns the kind of type. */
    /* Behaviour is undefined if type is invalid. */
    YmKind ymType_Kind(struct YmType* type);

    /* Returns the owner of type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_Owner(struct YmType* type);

    /* Returns the number of members type has, if any. */
    /* Behaviour is undefined if type is invalid. */
    YmMembers ymType_Members(struct YmType* type);

    /* Returns the member at member in type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_MemberByIndex(struct YmType* type, YmMemberIndex member);

    /* Returns the member under name in type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmType* ymType_MemberByName(struct YmType* type, const YmChar* name);

    /* Returns the number of type parameters the type has, if any. */
    /* Behaviour is undefined if type is invalid. */
    YmTypeParams ymType_TypeParams(struct YmType* type);

    /* Returns the type parameter at typeParam in type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_TypeParamByIndex(struct YmType* type, YmTypeParamIndex typeParam);

    /* Returns the type parameter under name in type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    /* Behaviour is undefined if name (as a pointer) is invalid. */
    YmType* ymType_TypeParamByName(struct YmType* type, const YmChar* name);

    /* Returns the return type of the type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_ReturnType(struct YmType* type);

    /* Returns the number of parameters the type has, if any. */
    /* Behaviour is undefined if type is invalid. */
    YmParams ymType_Params(struct YmType* type);

    /* Returns the name of param in type, or YM_NIL on failure. */
    /* This string's memory is managed internally. */
    /* Behaviour is undefined if type is invalid. */
    const YmChar* ymType_ParamName(struct YmType* type, YmParamIndex param);

    /* Returns the type of param in type, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_ParamType(struct YmType* type, YmParamIndex param);

    /* Returns the type referenced under reference, or YM_NIL on failure. */
    /* Behaviour is undefined if type is invalid. */
    YmType* ymType_Ref(struct YmType* type, YmRef reference);

    /* Returns if the reference ID type has for its reference to referenced, or YM_NO_REF if type doesn't reference referenced. */
    /* Behaviour is undefined if type is invalid. */
    /* Behaviour is undefined if referenced is invalid. */
    YmRef ymType_FindRef(struct YmType* type, struct YmType* referenced);

    /* Returns if the conversion 'from -> to' is legal. */
    /* coercion specifies if 'from -> to' is a legal coercion (aka. implicit conversion.) */
    /* Behaviour is undefined if from is invalid. */
    /* Behaviour is undefined if to is invalid. */
    YmBool ymType_Converts(struct YmType* from, struct YmType* to, YmBool coercion);


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
    * 
    *        Alternatively, we could add a YmImportedParcelsInfo struct containing reflection
    *        data about a snapshot of the current state of imported parcels, w/ us then exposing
    *        a ymImportedParcelsInfo_*** API to query reflection data, and things like ***_Destroy
    *        fn to cleanup w/.
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


    /* Type Iterator API */

    /* TODO */


    /* Internals */

    /* TODO */
}


#endif

