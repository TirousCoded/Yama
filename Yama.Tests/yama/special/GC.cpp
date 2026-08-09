

#include <unordered_set>

#include <gtest/gtest.h>
#include <yama/yama.h>
#include <yama++/general.h>
#include <yama++/Safe.h>
#include <yama++/print.h>

#include "../../utils/utils.h"


namespace {
	class GC : public ::testing::Test {
	public:
		YmDm* dm = nullptr;
		YmCtx* ctx = nullptr;
		std::unordered_set<ym::Safe<YmObj>> destroyedObjs;
		std::unordered_set<ym::Safe<YmObj>> objsExpectedToBeDestroyed;


		void shouldDestroy(YmObj& obj) {
			objsExpectedToBeDestroyed.insert(obj);
		}
		template<typename... Ts>
		void shouldDestroy(Ts&&... objs) {
			(shouldDestroy(std::forward<Ts>(objs)), ...);
		}
		void shouldDestroyTopN(YmLocals n) {
			for (YmLocals i = 0; i < n; i++) {
				shouldDestroy(*ymCtx_Local(ctx, -n + i, YM_BORROW));
			}
		}
		void expectedObjsWereDestroyed() {
			// TODO: If needed, add ways to better document failure details.
			EXPECT_EQ(destroyedObjs, objsExpectedToBeDestroyed);
		}
		void destroyCtxEarly() {
			ymCtx_Release(ctx);
		}
		void performGCCollect() {
			ymCtx_GCCollect(ctx);
			ymCtx_GCCollect(ctx);
		}
		void generateRefCycleGarbage(size_t n) {
			auto Any = load(ctx, "yama:Any");
			auto S = load(ctx, "p:S[yama:Any, yama:Any]");
			auto S_b = load(ctx, "p:S[yama:Any, yama:Any]::b");

			YmObj* first = nullptr;
			for (size_t i = 0; i < n; i++) {
				// Each iter of this loops consumes a p:S from the prev iter,
				// alongside a yama:None, to create next p:S in the chain.
				if (i == 0) {
					// Push yama:None in place of p:S if we're at start.
					ymCtx_PutNone(ctx, YM_PUSH);
					shouldDestroyTopN(1);
				}
				ymCtx_Convert(ctx, Any, YM_PUSH);
				shouldDestroyTopN(1);
				ymCtx_PutNone(ctx, YM_PUSH);
				shouldDestroyTopN(1);
				ymCtx_Convert(ctx, Any, YM_PUSH);
				shouldDestroyTopN(1);
				ymCtx_StructInit(ctx, S, "a,b", YM_PUSH);
				shouldDestroyTopN(1);
				if (i == 0) {
					first = ymCtx_Local(ctx, -1, YM_TAKE);
				}
			}
			if (first) {
				// Top stack obj is the final p:S in chain, which we'll now
				// be assigning to the p:S[~]::b property of *first, to form
				// our finished strong ref cycle.
				ymCtx_Convert(ctx, Any, YM_PUSH);
				shouldDestroyTopN(1);
				ymCtx_Put(ctx, YM_PUSH, first, YM_TAKE);
				ymCtx_Swap(ctx, -1, -2);
				ymCtx_SetProperty(ctx, S_b);
			}
			// Just to make sure stack is empty.
			ymCtx_PopAll(ctx);
		}

	protected:
		void SetUp() override {
			dm = ymDm_Create();
			ctx = ymCtx_Create(dm);
			ymCtx_SetObjDestroyCallback(ctx,
				[](YmObj* obj, void* user) {
#if 1
					auto xx = ymObj_Fmt(obj);
					ym::println("-- destroyed {} {} @ {}", ymType_Fullname(ymObj_Type(obj)), xx, (void*)obj);
					std::free((void*)xx);
#endif
					ym::deref((GC*)user).destroyedObjs.insert(ym::deref(obj));
				},
				(void*)this);

			auto pdef = ymParcelDef_Create();
			ymParcelDef_AddStruct(pdef, "S");
			ymParcelDef_AddTypeParam(pdef, "S", "A", "yama:Any");
			ymParcelDef_AddTypeParam(pdef, "S", "B", "yama:Any");
			ymParcelDef_AddStoredProperty(pdef, "S", "a", "$A");
			ymParcelDef_AddStoredProperty(pdef, "S", "b", "$B");
			ymDm_BindParcelDef(dm, "p", pdef);
			ymParcelDef_Release(pdef);
		}
		void TearDown() override {
			ymCtx_Release(ctx);
			ymDm_Release(dm);
		}
	};
}

TEST_F(GC, AllObjsDestroyedUponCtxDestroy) {
	// Gotta be able to cleanup frontend ref roots.
	auto a = ymCtx_NewInt(ctx, -4);
	auto b = ymCtx_NewUInt(ctx, 10);
	auto c = ymCtx_NewFloat(ctx, 3.14159);
	shouldDestroy(*a, *b, *c);

	// Gotta be able to cleanup obj stack roots.
	ymCtx_PutInt(ctx, YM_PUSH, -4);
	ymCtx_PutUInt(ctx, YM_PUSH, 10);
	ymCtx_PutFloat(ctx, YM_PUSH, 3.14159);
	shouldDestroyTopN(3);

	destroyCtxEarly();
	
	expectedObjsWereDestroyed();
}

TEST_F(GC, CtxDestroyWorksCorrectlyWithComplexDataStructures) {
	// Build complex data structure w/ structs and boxed values to test
	// that cleanup doesn't break on it (as it did at first.)

	auto Any = load(ctx, "yama:Any");
	auto S = load(ctx, "p:S[yama:Any, yama:Any]");

	auto tree0 = [&]() {
		ymCtx_PutInt(ctx, YM_PUSH, 10);
		shouldDestroyTopN(1);
		ymCtx_Convert(ctx, Any, YM_PUSH);
		ymCtx_PutInt(ctx, YM_PUSH, 10);
		shouldDestroyTopN(1);
		ymCtx_Convert(ctx, Any, YM_PUSH);
		shouldDestroyTopN(2);
		ymCtx_StructInit(ctx, S, "a,b", YM_PUSH);
		shouldDestroyTopN(1);
		ymCtx_Convert(ctx, Any, YM_PUSH);
		shouldDestroyTopN(1);
		};
	auto tree1 = [&]() {
		tree0();
		tree0();
		ymCtx_StructInit(ctx, S, "a,b", YM_PUSH);
		shouldDestroyTopN(1);
		ymCtx_Convert(ctx, Any, YM_PUSH);
		shouldDestroyTopN(1);
		};
	tree1();
	tree1();
	ymCtx_StructInit(ctx, S, "a,b", YM_PUSH);
	shouldDestroyTopN(1);

	destroyCtxEarly();

	expectedObjsWereDestroyed();
}

TEST_F(GC, CtxDestroyWorksCorrectlyWithRefCycles) {
	generateRefCycleGarbage(3);

	destroyCtxEarly();

	expectedObjsWereDestroyed();
}

TEST_F(GC, RefCountingDirectReleases) {
	ymCtx_PutInt(ctx, YM_PUSH, -4);
	shouldDestroyTopN(1);

	ymObj_Release(ymCtx_Pull(ctx));

	expectedObjsWereDestroyed();
}

TEST_F(GC, RefCountingIndirectReleases) {
	auto S1 = load(ctx, "p:S[yama:Int, yama:UInt]");
	auto S2 = load(ctx, "p:S[p:S[yama:Int, yama:UInt], yama:Float]");

	ymCtx_PutInt(ctx, YM_PUSH, -4);
	ymCtx_PutUInt(ctx, YM_PUSH, 10);
	shouldDestroyTopN(2);
	ymCtx_StructInit(ctx, S1, "a,b", YM_PUSH);
	ymCtx_PutFloat(ctx, YM_PUSH, 3.14159);
	shouldDestroyTopN(2);
	ymCtx_StructInit(ctx, S2, "a,b", YM_PUSH);
	shouldDestroyTopN(1);

	ymObj_Release(ymCtx_Pull(ctx));

	expectedObjsWereDestroyed();
}

// The below unit tests allow the impl to perform automatic GC cycles in
// the background, w/ ymCtx_GCCollect being called at the end to ensure
// that everything gets scanned as expected.

// Also, ymCtx_GCCollect gets called twice just in case in the future
// incremental collection results in it being possible for ymCtx_GCCollect
// to finish a collection cycle such that objects created during it did
// not get scanned by it.

TEST_F(GC, FrontendRefsAreRoots) {
	auto a = ymCtx_NewInt(ctx, -4);
	auto b = ymCtx_NewUInt(ctx, 10);
	auto c = ymCtx_NewFloat(ctx, 3.14159);

	performGCCollect();
	expectedObjsWereDestroyed();
}

TEST_F(GC, ObjStkEntriesAreRoots) {
	ymCtx_PutInt(ctx, YM_PUSH, -4);
	ymCtx_PutUInt(ctx, YM_PUSH, 10);
	ymCtx_PutFloat(ctx, YM_PUSH, 3.14159);

	performGCCollect();
	expectedObjsWereDestroyed();
}

TEST_F(GC, GCCleansUpStrongRefCycles) {
	ymCtx_PutInt(ctx, YM_PUSH, -4);
	ymCtx_PutUInt(ctx, YM_PUSH, 10);
	ymCtx_PutFloat(ctx, YM_PUSH, 3.14159);

	generateRefCycleGarbage(10);
	generateRefCycleGarbage(20);
	generateRefCycleGarbage(30);

	performGCCollect();
	expectedObjsWereDestroyed();
}

static_assert(YmKind_Num == 8);

TEST_F(GC, OutgoingRefs_Struct) {
	auto S = load(ctx, "p:S[yama:Int, yama:Float]");

	ymCtx_PutInt(ctx, YM_PUSH, 10);
	ymCtx_PutFloat(ctx, YM_PUSH, 4.15);
	// Should keep above two objs alive.
	ymCtx_StructInit(ctx, S, "a,b", YM_PUSH);

	performGCCollect();
	expectedObjsWereDestroyed();
}

TEST_F(GC, OutgoingRefs_Protocol) {
	auto Any = load(ctx, "yama:Any");

	ymCtx_PutInt(ctx, YM_PUSH, 10);
	// Should keep above obj alive.
	ymCtx_Convert(ctx, Any, YM_PUSH);

	performGCCollect();
	expectedObjsWereDestroyed();
}

