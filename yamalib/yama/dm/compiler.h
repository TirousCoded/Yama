

#pragma once


#include <optional>
#include <vector>

#include <taul/source_code.h>

#include "../core/api_component.h"
#include "../core/type_info.h"
#include "../core/domain.h"


namespace yama::dm {


    // TODO: right now we're just gonna expose ALL types in the domain to
    //       the compiler to use
    //
    //       later on, however, we're gonna need to revise it to instead take
    //       in some kind of 'environment' object which mediates access to
    //       some namespace of types
    //
    //       this system will need to be able to handle both compiling modules
    //       which see only a limited set of named dependencies, and scripts
    //       which might be allowed to see EVERYTHING


    // compiler takes in a taul::source_code, and compiles it into a vector
    // of type_info ready for upload


    class compiler final : public api_component {
    public:

        // TODO: I'm not 100% sure how we should expose existing type info to the compiler,
        //       including whether it should be pre-or-post instantiation, so for now we'll
        //       just inject a domain ptr, and revise later

        compiler(domain& dm, std::shared_ptr<debug> dbg = nullptr);


        std::optional<std::vector<type_info>> compile(const taul::source_code& src);


    private:

        domain* _dm_ptr = nullptr;

        inline domain& _dm() const noexcept { return deref_assert(_dm_ptr); }
    };
}

