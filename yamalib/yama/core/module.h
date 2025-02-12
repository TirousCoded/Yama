

#pragma once


#include "module_info.h"


namespace yama {


    // TODO: module has not been unit tested

    class module final {
    public:
        module() = delete;
        module(const module&) = default;
        module(module&&) noexcept = default;

        ~module() noexcept = default;

        module& operator=(const module&) = default;
        module& operator=(module&&) noexcept = default;


        module_info info() const; // TODO: temporary


        size_t size() const noexcept;
        bool contains(const str& name) const noexcept;


        // yama::module equality compares by reference

        bool operator==(const module&) const noexcept = default;


        // TODO: maybe add in backend concept of modules knowing their import path in domain env?

        //std::string fmt() const;


    private:
        friend class default_domain;


        const module_info* _ptr;


        module(const module_info* ptr);
    };
}


//YAMA_SETUP_FORMAT(yama::module, x.fmt());

