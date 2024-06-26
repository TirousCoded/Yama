

#pragma once


#include "type_data.h"


namespace yama {


    // this header file defines the impls of type_info


    static_assert(kind_count == 2);


    // IMPORTANT:
    //      as we update the below, be sure to update type_data-tests.h also
    //      (specifically the tests at the vary bottom under the static_assert)

    
    struct primitive_info final : public type_info {
        // TODO


        static constexpr auto kind() noexcept { return kind::primitive; }
    };

    static_assert(type_info_derivative<primitive_info>);


    struct function_info final : public type_info {
        // TODO


        static constexpr auto kind() noexcept { return kind::function; }
    };

    static_assert(type_info_derivative<function_info>);
}

