

#pragma once


#include "macros.h"
#include "general.h"

#include "../query-systems/general.h"


namespace yama {


    class type;


    // query key

    struct type_k final {
        static constexpr qs::qtype_t qtype = "type";
        using result = type;


        str fullname;   // the fullname of the type


        bool operator==(const type_k& other) const noexcept;

        size_t hash() const noexcept;
        std::string fmt() const;
    };
}

YAMA_SETUP_HASH(yama::type_k, x.hash());
YAMA_SETUP_FORMAT(yama::type_k, x.fmt());

static_assert(yama::qs::key_type<yama::type_k>);

