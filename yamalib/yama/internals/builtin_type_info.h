

#pragma once


#include "../core/type_info.h"


namespace yama::internal {


    struct builtin_type_info final {
        type_info None_info, Int_info, UInt_info, Float_info, Bool_info, Char_info;
    };

    builtin_type_info get_builtin_type_info() noexcept;
}

