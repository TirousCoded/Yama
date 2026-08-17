

#pragma once


#include "../yama/yama.h"


namespace _ym {


	class ObjManager;


	// TODO: If we ever wanna make GCCycleID 8-bit, we gotta account for overflow
	//		 making the current ID equal to GCNoCycle.

	using GCCycleID = YmUInt32;
	static constexpr GCCycleID GCNoCycle = 0;

	enum class GCEvent : YmUInt8 {
		Ack = 1 << 0, // Acknowledge newly created obj.
		Collect = 1 << 1, // Manual collect call.
	};
	constexpr GCEvent operator|(GCEvent lhs, GCEvent rhs) noexcept {
		return GCEvent((YmUInt8)lhs | (YmUInt8)rhs);
	}
	constexpr bool checkGCEvent(GCEvent actual, GCEvent expect) noexcept {
		return (YmUInt8(actual) & YmUInt8(expect)) != 0;
	}

	class GC {
	public:
		// events dictates which GC events should be enabled.
		GC(ObjManager& om, GCEvent events) noexcept;

		virtual ~GC() noexcept = default;


		ObjManager& om() const noexcept;

		void reset() noexcept;
		// Acknowledge newly created obj.
		void ack(YmObj& obj);
		void collect();


	protected:
		virtual void onReset() noexcept; // Always enabled.
		virtual void onAck(YmObj& obj);
		virtual void onCollect();


	private:
		ObjManager* const _om;
		const GCEvent _events;


		bool _has(GCEvent e) const noexcept;
	};


	class StopTheWorldGC final : public GC {
	public:
		StopTheWorldGC(ObjManager& om) noexcept;


	protected:
		void onReset() noexcept override;
		void onAck(YmObj& obj) override;
		void onCollect() override;


	private:
		// TODO: For now, we'll use a 'threshold' number of objs to determine when
		//		 to trigger our simple stop-the-world collector, w/ us making this
		//		 number 150% of the obj count after the collection cycle.
		//			* We may want to switch to bytes-based threshold.

		size_t _gcThreshold = size_t{};
		size_t _gcThresholdInitial = 100;
		double _gcThresholdGrowthFactor = 1.5;
		GCCycleID _gcCurrentCycle = GCNoCycle;


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

