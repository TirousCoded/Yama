

#pragma once


#include "../core/object_ref.h"

#include "ha_struct.h"
#include "type_mem.h"


namespace yama {
    
    
    class context;


    namespace internal {


        // call_frame_mem encapsulates the underlying state of a Yama call frame


        struct call_frame_mem_header final {
            context* ctx; // the context
            size_t max_locals; // max locals allowed in the frame
            size_t locals; // locals currently used in the frame


            inline size_t length() const noexcept { return max_locals; }
        };

        
        // call_frame_mem_elem will replace use of object_ref, as I can't default
        // construct yama::object_ref, and I don't wanna incur the extra storage
        // space cost of using std::optional<object_ref>, as I like how object_ref
        // being exactly 16-bytes makes it super compact (ie. 4 per L1 cache line)

        // exploiting object_ref being trivially copyable, and that we KNOW that
        // yama::type will encapsulate a non-null pointer at all times, we can
        // use a pair of 64-bit integers to contain the memory of the object_ref,
        // and use the case where both are 0 as a null state due to the above about
        // yama::type never having a nullptr in it

        struct call_frame_mem_elem final {
            int64_t a = 0, b = 0;


            bool good() const noexcept;

            object_ref& view() const noexcept;
            object_ref& view_unchecked() const noexcept; // need this for initializing objs


            bool operator==(const call_frame_mem_elem&) const noexcept = default;


            static call_frame_mem_elem make(const object_ref& x) noexcept;
        };


        using call_frame_mem = ha_struct<call_frame_mem_header, call_frame_mem_elem>;
    }
}

