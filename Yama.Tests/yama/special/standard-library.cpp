

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/general.h>

#include "../../utils/utils.h"


TEST(StandardLibrary, YamaParcel) {
	SETUP_ALL(ctx);
	
	auto p = import(ctx, "yama");
	EXPECT_STREQ(ymParcel_Path(p), "yama");

	// NOTE: Gotta remember to keep below up-to-date.
	EXPECT_TRUE(load(ctx, "yama:None"));
	EXPECT_TRUE(load(ctx, "yama:Int"));
	EXPECT_TRUE(load(ctx, "yama:UInt"));
	EXPECT_TRUE(load(ctx, "yama:Float"));
	EXPECT_TRUE(load(ctx, "yama:Bool"));
	EXPECT_TRUE(load(ctx, "yama:Rune"));
	EXPECT_TRUE(load(ctx, "yama:Type"));
	EXPECT_TRUE(load(ctx, "yama:Any"));
}

