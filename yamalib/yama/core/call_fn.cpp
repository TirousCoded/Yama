

#include "call_fn.h"

#include "context.h"
#include "const_table.h"

#include "../internals/bcode_interp.h"


void yama::noop_call_fn(context&, const_table) {
    // do nothing
}

void yama::bcode_call_fn(context& ctx, const_table consts) {
    internal::bcode_interp interp{ .ctx_ptr = &ctx, .consts = consts };
    interp.fire();
}

