

#include <unordered_set>

#include <gtest/gtest.h>
#include <yama++/Context.h>
#include <yama++/ParcelDef.h>
#include <yama/../internal/YmCtx.h>


TEST(YmCtx, ForEachRoot) {
	ym::Domain dm{};
	ym::Context ctx(dm);

	ym::ParcelDef pdef{};
	pdef.addStruct("S");
	pdef.addTypeParam("S", "A", "yama:Any");
	pdef.addTypeParam("S", "B", "yama:Any");
	dm.bind("p", pdef);

	auto A = ctx.newInt(10);
	auto B = ctx.newFloat(1.03);
	auto C = ctx.newBool(false);
	ctx.pushInt(11);
	ctx.pushFloat(1.04);
	ctx.pushBool(true);

	std::unordered_set<YmObj*> expected{
		A.get(),
		B.get(),
		C.get(),
		ctx.local(0).value().get(),
		ctx.local(1).value().get(),
		ctx.local(2).value().get(),
	};
	std::unordered_set<YmObj*> actual{};

	ctx.get()->forEachRoot([&actual](YmObj& root) {
		actual.insert(&root);
		});

	EXPECT_EQ(expected, actual);
}

