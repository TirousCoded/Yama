

#pragma once


#include <unordered_map>

#include "type_info.h"


namespace yama {


    // TODO: module_info methods have not really been unit tested

    struct module_info final {
        std::unordered_map<str, yama::type_info> types;


        size_t size() const noexcept;
        bool contains(const str& unqualified_name) const noexcept;

        // type behaviour is undefined if no type exists under unqualified_name

        const yama::type_info& type(const str& unqualified_name) const noexcept;
        inline const yama::type_info& operator[](const str& unqualified_name) const noexcept { return type(unqualified_name); }

        inline auto cbegin() const noexcept { return types.begin(); }
        inline auto begin() const noexcept { return cbegin(); }
        inline auto cend() const noexcept { return types.end(); }
        inline auto end() const noexcept { return cend(); }

        bool operator==(const module_info&) const noexcept = default;

        std::string fmt(const char* tab = "    ") const;
    };
}

YAMA_SETUP_FORMAT(yama::module_info, x.fmt());

namespace yama {


    // IMPORTANT: module_factory is designed to abstract away details of the way module
    //            data is structured so we can revise things w/out having to refactor
    //            as many unit tests

    class module_factory final {
    public:
        module_factory() = default;
        module_factory(module_factory&&) noexcept = default;

        ~module_factory() noexcept = default;

        module_factory& operator=(module_factory&&) noexcept = default;


        module_info done() noexcept; // returns finished module and resets factory


        // TODO: maybe in the future add unit tests for defined behaviour replacing below UB

        // behaviour is undefined if unqualified_name is already used

        // TODO: add_type hasn't itself been unit tested yet

        module_factory& add_type(type_info&& x);

        static_assert(kinds == 2);

        module_factory& add_primitive_type(
            str unqualified_name,
            const_table_info&& consts,
            ptype ptype);

        // native fn overload
        module_factory& add_function_type(
            str unqualified_name,
            const_table_info&& consts,
            std::vector<str>&& param_names,
            callsig_info&& callsig,
            size_t max_locals,
            call_fn call_fn);
        // bcode overload
        module_factory& add_function_type(
            str unqualified_name,
            const_table_info&& consts,
            std::vector<str>&& param_names,
            callsig_info&& callsig,
            size_t max_locals,
            bc::code&& code,
            bc::syms&& syms = bc::syms{});


    private:
        module_info _result;
    };
}

