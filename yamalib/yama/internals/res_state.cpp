

#include "res_state.h"


void yama::internal::res_state::set_upstream(res_state& upstream) noexcept {
    types.set_upstream(upstream.types);
    modules.set_upstream(upstream.modules);
}

void yama::internal::res_state::set_no_upstream() noexcept {
    types.set_no_upstream();
    modules.set_no_upstream();
}

void yama::internal::res_state::discard() noexcept {
    types.discard();
    modules.discard();
}

void yama::internal::res_state::commit() {
    types.commit();
    modules.commit();
}

void yama::internal::res_state::commit_or_discard(bool commit) {
    types.commit_or_discard(commit);
    modules.commit_or_discard(commit);
}

