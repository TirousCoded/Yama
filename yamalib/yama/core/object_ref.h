

#pragma once


#include <string>
#include <format>

#include "macros.h"
#include "scalars.h"
#include "type.h"


namespace yama {


    // NOTE: object_ref is unit tested in context-tests.cpp

    // object_ref encapsulates a strong reference to a Yama object which
    // resides within the state of some context

    // object_ref has no null state (though the encapsulated Yama object may,
    // if it's some kind of 'optional')

    // object_ref is not exposed to whether the object it references is
    // of a canonicalized type

    // the lifetimes of objects in Yama are governed by ARC, however, being
    // quite low-level, object_ref reference counting is NOT automatic, and
    // must be performed manually via context methods, w/ undefined behaviour
    // if this is performed improperly

    struct object_ref final {
        type t;
        union {
            int_t i = 0;
            uint_t ui;
            float_t f;
            bool_t b;
            char_t c;
        } v;


        int_t as_int() const noexcept;
        uint_t as_uint() const noexcept;
        float_t as_float() const noexcept;
        bool_t as_bool() const noexcept;
        char_t as_char() const noexcept;


        bool operator==(const object_ref& other) const noexcept;


        std::string fmt() const;
    };

    static_assert(std::is_trivially_copyable_v<object_ref>);
}


YAMA_SETUP_FORMAT(yama::object_ref, x.fmt());


namespace yama {


    // manual ref counting can get quite confusing, as it involves multiple
    // portions of the program having to keep straight what is/isn't their
    // responsibility w/ regards to ref counts

    // to this end, Yama will employ Python conventions w/ regards to notions
    // of reference ownership semantics to try and simplify things
    // (see https://docs.python.org/3/c-api/intro.html#objects-types-and-reference-counts)

    // the details of these conventions are, unless otherwise specified, the
    // same as those in the Python API

    // IMPORTANT:
    //      in our API's terminology, the concept of 'ownership moving' will
    //      refer to the idea of 'moving' ownership of a reference such that
    //      the original owner no longer has ownership of it
    //
    //      this concept is important, as it occurring means that no incr/decr
    //      to the referenced object's ref count need occur
    //
    //      this concept is mentioned commonly
    //
    //      ownership moving describes the behaviour of both the default 
    //      semantics associated w/ returning an object_ref, and of the notion
    //      of reference *stealing*

    // IMPORTANT:
    //      by default, returning an object_ref from a function implies that
    //      the reference was ownership moved out of the function call
    //
    //      for scenarios where a borrowed reference is to be returned (which
    //      is sometimes what's wanted), yama::borrowed_ref should be used as
    //      the return type to indicate this

    // these two type aliases are provided to help communicate intent

    using borrowed_ref = object_ref;
    using stolen_ref = object_ref;

    // TODO: maybe try and rewrite the below to be more *terse*

    // alongside borrowed_ref and stolen_ref, Yama will employ one additional
    // notion of a 'canonical' reference, for situations where we KNOW w/ 100%
    // certainty that the object in question is of a canonicalized type

    // canonical references are functionally identical to regular strong
    // references, and can interact w/ regular references, and reference
    // borrowing/stealing exactly like one

    // what makes canonical references unique is that, so long as we KNOW
    // the object is canonical (ie. if we're using canonical_ref, which
    // communicates intent) then the decr of it can be forwent, as we KNOW
    // that it's not necessary

    // canonical references exist to help avoid the situation where we have
    // a strong reference to a primitive, function, etc. and we forget to
    // decr it *properly*, but the system reports no error, as of course,
    // the decr is a noop, and failing to following our conventions in that
    // scenario doesn't actually compromise the system

    // canonical references thus help keep our conventions used properly,
    // which helps code readability, maintainability, etc. in addition to
    // streamlining usage by letting end-user have to do less work in 
    // scenarios where we know it isn't needed

    // finally, take note that if we have a object_ref, borrowed_ref, or
    // stolen_ref, these should NOT be converted to a canonical_ref w/out
    // being sure clear about why we know it's safe to do so

    using canonical_ref = object_ref;
}

