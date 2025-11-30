

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Parcels, Path) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    BIND_AND_IMPORT(ctx, parcel, p, path.c_str());
    const YmChar* result = ymParcel_Path(parcel); // Query import path.
    ASSERT_TRUE(result);
    EXPECT_STREQ(result, path.c_str());
}

