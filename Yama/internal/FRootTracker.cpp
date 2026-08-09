

#include "FRootTracker.h"

#include "YmObj.h"


#define _TRACE_FROOT 0

#if _TRACE_FROOT
#include "../yama++/print.h"
#endif


YmUInt32 _ym::FRootTracker::count() const noexcept {
	return (YmUInt32)_tracked.size();
}

bool _ym::FRootTracker::isTracked(YmObj& obj) const noexcept {
	return obj.froot != NO_FROOT;
}

void _ym::FRootTracker::track(YmObj& obj) {
	if (!isTracked(obj)) {
#if _TRACE_FROOT
		ym::println("-- FRoot Track: {} @ {}", obj.type->fullname(), (void*)&obj);
#endif
		obj.froot = count();
		_tracked.push_back(obj);
	}
}

void _ym::FRootTracker::untrack(YmObj& obj) noexcept {
	if (isTracked(obj)) {
#if _TRACE_FROOT
		ym::println("-- FRoot Untrack: {} @ {}", obj.type->fullname(), (void*)&obj);
#endif
		// Swap the back entry w/ the one we're deleting, to densify the
		// array, and position entry we're deleting as the new back one,
		// then pop this back entry.
		_tracked.back()->froot = obj.froot;
		std::swap(_tracked[obj.froot], _tracked.back());
		_tracked.pop_back();
		obj.froot = NO_FROOT;
	}
}

void _ym::FRootTracker::untrackAll() noexcept {
#if _TRACE_FROOT
	ym::println("-- FRoot Track All");
#endif
	for (auto& obj : _tracked) {
		obj->froot = NO_FROOT;
	}
	_tracked.clear();
}

