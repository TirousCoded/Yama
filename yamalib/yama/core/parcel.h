

#pragma once


#include <taul/source_code.h>

#include "general.h"
#include "res.h"
#include "api_component.h"
#include "module_info.h"


namespace yama {


    class domain;


    // TODO: dep_reqs has not been unit tested

    // dep_reqs defines a set of dependency requirements of a parcel,
    // organizing said details as a set (for O(1) lookup time) of dep
    // names, alongside dep_info metadata

    struct dep_reqs final {
        struct metadata final {
            //
        };


        std::unordered_map<str, const metadata> reqs;


        bool exists(const str& dep_name) const noexcept;
        const metadata& lookup(const str& dep_name) const noexcept;

        inline auto cbegin() const noexcept { return reqs.cbegin(); }
        inline auto begin() const noexcept { return cbegin(); }
        inline auto cend() const noexcept { return reqs.cend(); }
        inline auto end() const noexcept { return cend(); }


        dep_reqs& add(str dep_name);
    };


    // NOTE: services classes are unit tested as part of domain impl unit tests

    class parcel_services final {
    public:
        parcel_services() = delete;


        const str& install_name() const noexcept;
        std::shared_ptr<const module_info> compile(const taul::source_code& src);


    private:
        friend class domain;


        parcel_services(domain& client, const str& install_name); // for use in domain


        domain* _client;
        str _install_name;
    };


    class parcel : public api_component {
    public:
        parcel(std::shared_ptr<debug> dbg = nullptr);

        virtual ~parcel() noexcept = default;


        // NOTE: self-name mappings are considered a special form of dep mapping

        // self_name returns the self-name of the parcel

        // the self-name of a parcel is a dependency-like name the parcel uses internally to refer to itself

        virtual str self_name() const noexcept = 0;

        // deps returns the dependency requirements of the parcel

        virtual const dep_reqs& deps() = 0;

        // import imports the module at relative_path in the parcel, if any
        
        virtual std::shared_ptr<const module_info> import(parcel_services services, str relative_path) = 0;
    };


    class null_parcel final : public parcel {
    public:
        null_parcel() = default;


        inline str self_name() const noexcept { return str::lit("self"); }
        inline const dep_reqs& deps() override final { return dep_reqs{}; }
        inline std::shared_ptr<const module_info> import(parcel_services, str) override final { return nullptr; }
    };
}

