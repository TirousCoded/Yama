

#pragma once


#include <concepts>

#include "../core/general.h"


namespace yama::dm {


    // this stuff is used to assist things like res_db in identifying the
    // fullname of a given type


    // 'type1' fullnamed types have a 'fullname' *method* accessible via a '.'
    // 'type2' fullnamed types have a 'fullname' *method* accessible via a '->'
    // 'type3' fullnamed types have a 'fullname' *field* accessible via a '.'
    // 'type4' fullnamed types have a 'fullname' *field* accessible via a '->'

    template<typename T>
    concept type1_fullnamed_type =
        requires (const T cv)
    {
        { cv.fullname() } -> std::convertible_to<str>;
    };

    template<typename T>
    concept type2_fullnamed_type =
        requires (const T cv)
    {
        { cv->fullname() } -> std::convertible_to<str>;
    };
    
    template<typename T>
    concept type3_fullnamed_type =
        requires (const T cv)
    {
        { cv.fullname } -> std::convertible_to<str>;
    };

    template<typename T>
    concept type4_fullnamed_type =
        requires (const T cv)
    {
        { cv->fullname } -> std::convertible_to<str>;
    };

    // exploit template parameter overloading via concepts to allow for 
    // getting of fullname of objects regardless of whether it requires
    // accessing via '.' or '->', and regardless of whether it uses
    // a method or a field

    template<type1_fullnamed_type T>
    inline str fullname_of(const T& x) {
        return x.fullname();
    }

    template<type2_fullnamed_type T>
    inline str fullname_of(const T& x) {
        return x->fullname();
    }
    
    template<type3_fullnamed_type T>
    inline str fullname_of(const T& x) {
        return x.fullname;
    }

    template<type4_fullnamed_type T>
    inline str fullname_of(const T& x) {
        return x->fullname;
    }

    // use above to define fully general 'fullnamed_type' concept
    
    template<typename T>
    concept fullnamed_type =
        requires (const T cv)
    {
        { yama::dm::fullname_of(cv) } -> std::convertible_to<str>;
    };
}

