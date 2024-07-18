

#pragma once


#include <optional>

#include "res.h"
#include "api_component.h"
#include "mas.h"
#include "type_data.h"
#include "type.h"


namespace yama {


    class domain : public api_component {
    public:

        domain(std::shared_ptr<debug> dbg = nullptr);


        // get_mas returns the MAS used associated w/ this domain

        virtual res<mas> get_mas() = 0;


        // get_type_data returns the type_data under fullname, if any

        virtual std::optional<type_data> get_type_data(const str& fullname) = 0;


        // get_type returns the type under fullname, if any

        virtual std::optional<type> get_type(const str& fullname) = 0;


        // push attempts to push new type information to the domain,
        // returning if successful

        virtual bool push(type_data x) = 0;

        // this overload generates a type_data from x, then pushing it

        template<type_info_derived_type T>
        inline bool push(T&& x);
    };


    template<type_info_derived_type T>
    inline bool domain::push(T&& x) {
        return do_push(type_data(std::forward<T&&>(x)));
    }
}

