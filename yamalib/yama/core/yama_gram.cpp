

#include "yama_gram.h"


#define _YAMA_USE_SRCGEN_GRAMMAR 1

#if _YAMA_USE_SRCGEN_GRAMMAR
#include "../internals/srcgen/yama-gram.h"
#else
#include <taul/load.h>
static taul::grammar _yama_gram = []() -> taul::grammar {
    const auto p = std::filesystem::current_path() / "..\\yamalib\\yama\\taul-specs\\yama.taul";
    const auto result = taul::load(p, taul::stderr_lgr());
    if (!result) std::abort();
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

