

#include "mid_provider.h"

#include "../core/asserts.h"


yama::mid_t yama::internal::mid_provider::pull() noexcept {
	YAMA_ASSERT(_id != std::numeric_limits<mid_t>::max());
	return _id++;
}

void yama::internal::mid_provider::commit() noexcept {
	_last_committed_id = _id;
}

void yama::internal::mid_provider::discard() noexcept {
	_id = _last_committed_id;
}

void yama::internal::mid_provider::commit_or_discard(bool commit) noexcept {
	if (commit) this->commit();
	else		discard();
}

