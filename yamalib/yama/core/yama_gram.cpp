

#include "yama_gram.h"


// TODO: figure out a better way of organizing this stuff
//
//       in particular, have it be that we can maybe unit test both the source code, and srcgen,
//       forms of yama.taul at the same time

#define _YAMA_USE_SRCGEN_GRAMMAR 1

#if _YAMA_USE_SRCGEN_GRAMMAR
#include "../internals/srcgen/yama-gram.h"
#else
#include <taul/load.h>
static taul::grammar _yama_gram = []() -> taul::grammar {
    // TODO: transcompile later
    const auto p = std::filesystem::current_path() / "..\\yamalib\\yama\\taul-specs\\yama.taul";
    const auto result = taul::load(p, taul::make_stderr_logger());
    if (!result) {
        std::abort();
    }
    return *result;
    }();
#endif

taul::grammar yama::yama_gram() {
#if _YAMA_USE_SRCGEN_GRAMMAR
    return taul::fetchers::yama();
#else
    return _yama_gram;
#endif
}

