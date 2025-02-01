

#pragma once


#include <taul/source_code.h>

#include "general.h"
#include "res.h"
#include "api_component.h"
#include "module_info.h"


namespace yama {


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


    class parcel : public api_component {
    public:
        // TODO: add later
        //class services : public api_component {
        //public:
        //    services(std::shared_ptr<debug> dbg = nullptr);
        //
        //    virtual ~services() noexcept = default;
        //
        //
        //    virtual std::optional<module_info> compile(const taul::source_code& src) = 0;
        //};


        parcel(std::shared_ptr<debug> dbg = nullptr);

        virtual ~parcel() noexcept = default;


        // deps returns the dependency requirements of the parcel

        virtual const dep_reqs& deps() const = 0;

        // TODO: add later
        //// import imports the module at relative_path in the parcel, if any
        //
        //virtual std::shared_ptr<module_info> import(
        //    res<services> services,
        //    str relative_path) = 0;
    };


    class null_parcel final : public parcel {
    public:
        null_parcel() = default;


        inline const dep_reqs& deps() const override final { return dep_reqs{}; }
        //inline std::shared_ptr<module_info> import(res<services>, str) override final { return nullptr; }
    };
}

