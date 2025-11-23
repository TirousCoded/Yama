

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Parcels, PID) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    const YmPID a_pid = ymParcel_PID(a);
    const YmPID b_pid = ymParcel_PID(b);
    const YmPID c_pid = ymParcel_PID(c);
    // Ensure PID uniqueness.
    EXPECT_NE(a_pid, b_pid);
    EXPECT_NE(a_pid, c_pid);
    EXPECT_NE(b_pid, c_pid);
    // Ensure subsequent calls to ymParcel_PID yield same result.
    EXPECT_EQ(a_pid, ymParcel_PID(a));
    EXPECT_EQ(b_pid, ymParcel_PID(b));
    EXPECT_EQ(c_pid, ymParcel_PID(c));
}

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

