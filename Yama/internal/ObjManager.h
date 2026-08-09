

#pragma once


#include <functional>
#include <unordered_set>

#include "../yama/yama.h"
#include "../yama++/meta.h"
#include "../yama++/Safe.h"
#include "FRootTracker.h"
#include "MAS.h"
#include "obj-ref-helpers.h"


namespace _ym {


	// TODO: If we ever wanna make GCCycleID 8-bit, we gotta account for overflow
	//		 making the current ID equal to GCNoCycle.

	using GCCycleID = YmUInt32;
	static constexpr GCCycleID GCNoCycle = 0;


	class ObjManager final {
	public:
		// Returns obj on global stk by index, or nullptr on fail.
		using GetGlobalObj = std::function<YmObj*(YmUInt32)>;


		ObjManager(YmCtx* ctx, GetGlobalObj getGlobalObj);


		size_t count() const noexcept;
		bool exists(YmObj& obj) const noexcept;

		// A given root object may be traversed multiple times.
		inline void forEachRoot(ym::Callable<void, YmObj&> auto&& visitor) const {
			for (YmUInt32 i = 0;; i++) {
				if (auto obj = _getGlobalObj(i)) {
					visitor(*obj);
				}
				else break;
			}
			_froots.forEachTracked(visitor);
		}

		void setObjDestroyCallback(YmObjDestroyCallbackFn fn, void* user) noexcept;
		void reset();
		// Slots will be nullptr or 0.
		TempRef create(YmType& type, bool frontend);
		YmRefCount secure(YmObj& obj, bool frontend);
		YmRefCount release(YmObj& obj, bool frontend);

		TempRef newNone(bool frontendRef);
		TempRef newInt(YmInt v, bool frontendRef);
		TempRef newUInt(YmUInt v, bool frontendRef);
		TempRef newFloat(YmFloat v, bool frontendRef);
		TempRef newBool(YmBool v, bool frontendRef);
		TempRef newRune(YmRune v, bool frontendRef);
		TempRef newType(YmType& v, bool frontendRef);
		TempRef newDefault(YmType* type, bool frontendRef);

		void gcCollect();


	private:
		YmObjDestroyCallbackFn _objDestroyCallback = nullptr;
		void* _objDestroyCallbackUser = nullptr;
		YmCtx* _ctx = nullptr;
		GetGlobalObj _getGlobalObj;
		std::unordered_set<YmObj*> _allocatedObjs; // TODO: Replace this.
		FRootTracker _froots;
		HeapMAS _mas;

		// TODO: For now, we'll use a 'threshold' number of objs to determine when
		//		 to trigger our simple stop-the-world collector, w/ us making this
		//		 number 150% of the obj count after the collection cycle.

		size_t _gcThreshold = size_t{};
		size_t _gcThresholdInitial = 100;
		double _gcThresholdGrowthFactor = 1.5;
		GCCycleID _gcCurrentCycle = GCNoCycle;


		// Not appropriate for destroying objs in ref cycles.
		void _destroy(YmObj& obj);
		void _destroyAll();
		void _reportDestroy(YmObj& obj);
		void _deinitObj(YmObj& obj, bool releaseOutgoingRefs);
		void _deallocObj(YmObj& obj) noexcept;

		// TODO: Our current impl uses a quick-n'-dirty stop-the-world mark-and-sweep
		//		 GC, which is super rough, and can be GREATLY improved.

		void _gcReset();
		void _gcAcknowledgeNewObj(YmObj& obj);
		void _gcCollect(YmObj* triggerObj);
		bool _gcIsReachable(YmObj& obj);
		void _gcBeginCycle();
		void _gcMarkPhase(YmObj* triggerObj);
		void _gcMark(YmObj& obj);
		void _gcMarkObjCycleID(YmObj& obj) noexcept;
		void _gcMarkOutgoingRefs(YmObj& obj);
		void _gcSweepPhase();
		void _gcDropOutgoingRefsToReachableObjs(YmObj& obj);
		void _gcEndCycle();
		void _gcUpdateThreshold() noexcept;
	};
}

