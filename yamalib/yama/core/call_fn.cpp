

#include "call_fn.h"

#include "context.h"


void yama::noop_call_fn(context&) {
    // do nothing
}

void yama::bcode_call_fn(context&) {
    // NOTE: bcode_call_fn is inert, existing solely to indicate to yama::context when to invoke bcode
}

