

#pragma once


#include <optional>

#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "type_data.h"
#include "type.h"

// NOTE: this is so things like yama::primitive_info are available to end-user
//       w/out forcing them to include type_info.h first

#include "type_info.h"


namespace yama {


    class domain : public api_component {
    public:

        domain(std::shared_ptr<debug> dbg = nullptr);


        // get_mas returns the MAS used associated w/ this domain

        virtual res<mas> get_mas() = 0;


        // get_type returns the type under fullname, if any

        virtual std::optional<type> get_type(const str& fullname) = 0;


        // push attempts to push new type information to the domain,
        // returning if successful

        virtual bool push(type_data x) = 0;

        template<type_info_derived_type T>
        inline bool push(T&& x);
    };


    template<type_info_derived_type T>
    inline bool domain::push(T&& x) {
        return push(type_data(std::forward<T&&>(x)));
    }
}

