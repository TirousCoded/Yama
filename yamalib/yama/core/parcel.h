

#pragma once


#include <unordered_set>
#include <atomic>

#include <taul/source_code.h>

#include "general.h"
#include "res.h"
#include "api_component.h"
#include "module_info.h"


namespace yama {


    // IMPORTANT:
    //      self-names are dependency-like name parcel uses (internally) to refer to itself
    //
    //      regarding things like import paths, qualified names, fullnames, etc. self-names
    //      are treated as special dep-names


    // TODO: parcel_metadata has not been unit tested

    struct parcel_metadata final {
        str                     self_name;
        std::unordered_set<str> dep_names;


        bool is_dep_name(const str& name) const noexcept;
        bool is_self_or_dep_name(const str& name) const noexcept;


        void add_dep_name(const str& name);
    };


    // TODO: import_result has not been unit tested

    class import_result final {
    public:
        import_result(module_info&& x);
        import_result(res<module_info>&& x);
        import_result(taul::source_code&& x);

        import_result() = delete;
        import_result(const import_result&) = default;
        import_result(import_result&&) noexcept = default;

        ~import_result() noexcept = default;
        
        import_result& operator=(const import_result&) = default;
        import_result& operator=(import_result&&) noexcept = default;


        bool holds_module() const noexcept;
        bool holds_source() const noexcept;

        // behaviour is undefined if import_result doesn't hold module/source

        res<module_info>& get_module();
        const res<module_info>& get_module() const;

        taul::source_code& get_source();
        const taul::source_code& get_source() const;


    private:
        std::variant<res<module_info>, taul::source_code> _data;
    };


    // TODO: one potential issue w/ parcel_id is if we want to install a parcel object multiple
    //       times on a domain, just w/ different linkage
    //
    //       this shouldn't really be an issue, as we can just *disallow* this, but I'm making
    //       a note here as I don't think our impl currently does

    // parcels (non-portable) process-wide IDs upon initialization

    using parcel_id = uint64_t;


    class parcel : public api_component {
    public:
        parcel(std::shared_ptr<debug> dbg = nullptr);

        virtual ~parcel() noexcept = default;


        parcel_id id() const noexcept;


        virtual const parcel_metadata& metadata() = 0;
        virtual std::optional<import_result> import(const str& relative_path) = 0;


    private:
        parcel_id _id;


        static std::atomic<parcel_id> _next_id; // incr ID counter

        static parcel_id _acquire_id() noexcept;
    };


    class null_parcel final : public parcel {
    public:
        null_parcel();


        inline const parcel_metadata& metadata() override final {
            static const parcel_metadata m{ str::lit("self"), {} };
            return m;
        }
        inline std::optional<import_result> import(const str&) override final {
            return std::nullopt;
        }
    };
}

