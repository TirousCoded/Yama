

#pragma once


#include <vector>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "../yama++/meta.h"


namespace _ym {


	using FRootID = YmUInt32;
	constexpr FRootID NO_FROOT = -1;

	// Maintains a sparse-set of refs to objects which are made GC roots, in part
	// or in whole, due to C API frontend refs existing for it.
	class FRootTracker final {
	public:
		FRootTracker() = default;


		YmUInt32 count() const noexcept;
		bool isTracked(YmObj& obj) const noexcept;

		// Fails quietly if obj's type isn't a ref carrier.
		void track(YmObj& obj);
		void untrack(YmObj& obj) noexcept;
		void untrackAll() noexcept;

		inline void forEachTracked(ym::Callable<void, YmObj&> auto&& visitor) const {
			for (const auto& obj : _tracked) {
				visitor(*obj);
			}
		}


	private:
		// NOTE: This tracks roots via a 'dense array' (ie. like in a sparse set.)

		std::vector<ym::Safe<YmObj>> _tracked;
	};
}

