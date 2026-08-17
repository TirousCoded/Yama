

#pragma once


#include <functional>
#include <unordered_set>

#include "../yama/yama.h"
#include "../yama++/meta.h"
#include "../yama++/Safe.h"
#include "FRootTracker.h"
#include "MAS.h"
#include "obj-ref-helpers.h"
#include "GC.h"


namespace _ym {


	class ObjManager final {
	public:
		// Returns obj on global stk by index, or nullptr on fail.
		using GetGlobalObj = std::function<YmObj*(YmUInt32)>;


		ObjManager(YmCtx* ctx, GetGlobalObj getGlobalObj, std::unique_ptr<GC> gc);


		size_t count() const noexcept;

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

		// NOTE: These are for use by GC impl.

		const std::unordered_set<YmObj*>& objects() const noexcept;
		void reportDestroy(YmObj& obj);
		void deinitObj(YmObj& obj, bool releaseOutgoingRefs);
		void deallocObj(YmObj& obj) noexcept;


	private:
		YmCtx* _ctx = nullptr;
		GetGlobalObj _getGlobalObj;
		std::unique_ptr<GC> _gc;
		YmObjDestroyCallbackFn _objDestroyCallback = nullptr;
		void* _objDestroyCallbackUser = nullptr;
		std::unordered_set<YmObj*> _allocatedObjs; // TODO: Replace this.
		FRootTracker _froots;
		HeapMAS _mas;


		// Not appropriate for destroying objs in ref cycles.
		void _destroy(YmObj& obj);
		void _destroyAll();
	};
}

