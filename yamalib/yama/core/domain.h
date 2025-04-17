

#pragma once


#include <optional>

#include "res.h"
#include "api_component.h"
#include "type_info.h" // <- for quality-of-life (eg. exposes yama::primitive_info)
#include "type.h"
#include "module.h"
#include "parcel.h"
#include "install_batch.h"

#include "../internals/domain_data.h"


namespace yama {


    // TODO: later on, a feature that might be worth looking into is maybe incorporating
    //       something like ARC to dealloc instantiated types when there's no demand for
    //       the type in question
    //
    //       this, and other things involving domain impl being free to discard loaded
    //       types or imported modules, such as if compilation fails, are things we should
    //       also think hard on what details we do/don't want to be expressed in unit tests

    // IMPORTANT: the memory underlying type objects is guaranteed to be valid for the
    //            lifetime of the domain, and to be entirely thread-safe, due to said type
    //            info memory being immutable

    // TODO: add write-ups like below for qualified names, fullnames, etc.

    // IMPORTANT: import paths are period-seperated lists of identifiers specifying
    //            where a module is found, and what it's called, w/ each module having
    //            a unique import path
    //
    //            the first identifier in this list is called the 'head', and specifies
    //            the parcel from which the module is imported
    //
    //            the meaning of the head identifier depends upon the 'environment', or
    //            'env', in which importing is occurring
    // 
    //            two types of environments exist:
    //                  1) 'domain envs' identify parcels by their install names
    //                  2) 'parcel envs' identify parcels by their dep names (in the
    //                     context of a particular parcel and its deps)
    //                          * also, the 'self-name' can be used in parcel envs to
    //                            access the parcel of the env itself
    //
    //            removing the head identifier from the import path yields the 'relative
    //            path' of the import path, which is used within parcels to specify
    //            modules w/out exposing info about said parcel existing within the
    //            context of a broader environment of parcels
    // 
    //            this lack of broader context is important as parcels are not supposed
    //            to be coupled to domains
    // 
    //            a relative path which is an empty string refers to the 'root' module
    //            of a parcel, w/ root modules having the notable characteristic that
    //            import paths to them only contain a head identifier
    //
    //            given an import path 'std.io.net.http', 'std' is the head, and
    //            '.io.net.http' is the relative path

    // TODO: domain thread-safety hasn't really been unit tested (not sure how)

    // IMPORTANT: domains are designed to be thread-safe


    class domain final : public api_component {
    public:
        domain(std::shared_ptr<debug> dbg = nullptr);


        // TODO: add unit tests for get_installed
        // TODO: add unit tests for parcel_id overloads of below

        bool install(install_batch&& x);
        size_t install_count() const noexcept;
        bool is_installed(const str& install_name) const noexcept;
        bool is_installed(parcel_id id) const noexcept;
        std::shared_ptr<parcel> get_installed(const str& install_name) const noexcept;
        std::shared_ptr<parcel> get_installed(parcel_id id) const noexcept;

        // TODO: make our unit tests for syntax rules for load/import use more robust, as
        //       right now they're, I feel, fairly loose

        std::optional<yama::module> import(const str& import_path);

        std::optional<type> load(const str& fullname);

        // load builtins w/ minimal overhead

        type load_none();
        type load_int();
        type load_uint();
        type load_float();
        type load_bool();
        type load_char();


    private:
        mutable internal::domain_data _dd;


        void _finish_setup();
        void _install_builtin_yama_parcel();
        void _preload_builtin_types();
    };
}

