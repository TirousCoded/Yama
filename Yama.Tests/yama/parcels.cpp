

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Parcels, RType) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    SETUP_PARCEL(parcel, p, path);
    EXPECT_EQ(ymRType(parcel), YmRType_Parcel);
}

TEST(Parcels, RC) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    SETUP_PARCEL(parcel, p, path);
    ASSERT_EQ(ymRefCount(parcel), 2); // Initial value.
    ymAddRef(parcel);
    EXPECT_EQ(ymRefCount(parcel), 3);
    ymAddRef(parcel);
    EXPECT_EQ(ymRefCount(parcel), 4);
    ymDrop(parcel);
    EXPECT_EQ(ymRefCount(parcel), 3);
    ymDrop(parcel);
    EXPECT_EQ(ymRefCount(parcel), 2);
    // ScopedDrop and API internals will drop final two, and the actual release
    // of the resource is unobservable.
}

TEST(Parcels, Path) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    SETUP_PARCEL(parcel, p, path);
    const YmChar* result = ymParcel_Path(parcel); // Query import path.
    ASSERT_TRUE(result);
    EXPECT_STREQ(result, path.c_str());
}

