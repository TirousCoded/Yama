

#pragma once


#include "../core/ids.h"


namespace yama::internal {


	class mid_provider final {
	public:
		mid_provider() = default;


		mid_t pull() noexcept;

		void commit() noexcept;
		void discard() noexcept;
		void commit_or_discard(bool commit) noexcept;


	private:
		mid_t _id = 0, _last_committed_id = 0;
	};
}

