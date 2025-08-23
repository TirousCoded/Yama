

#pragma once


#include "ids.h"
#include "module.h"

#include "../internals/safeptr.h"


namespace yama {


    class module_ref;

    namespace internal {
        struct imported_module;
        yama::module_ref create_module(const imported_module& im);
    }


    // TODO: module_ref has not been unit tested.

    class module_ref final {
    public:
        module_ref() = delete;
        module_ref(const module_ref&) = default;
        module_ref(module_ref&&) noexcept = default;
        ~module_ref() noexcept = default;
        module_ref& operator=(const module_ref&) = default;
        module_ref& operator=(module_ref&&) noexcept = default;


        const module& info() const noexcept;
        mid_t id() const noexcept;

        size_t count() const noexcept;
        bool exists(const str& name) const noexcept;

        // Compares by reference.
        bool operator==(const module_ref&) const noexcept = default;


        // TODO: Maybe add in backend concept of modules knowing their import path in domain env?

        //std::string fmt() const;


    private:
        friend yama::module_ref yama::internal::create_module(const internal::imported_module& im);


        const internal::safeptr<const module> _m;
        mid_t _id;


        module_ref(const internal::imported_module& im);
    };
}


//YAMA_SETUP_FORMAT(yama::module_ref, x.fmt());

