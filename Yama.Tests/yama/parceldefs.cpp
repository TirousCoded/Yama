

#include <gtest/gtest.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(ParcelDefs, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef); // Macro will do the create/destroy.
}

TEST(ParcelDefs, RType) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef);
    EXPECT_EQ(ymRType(parceldef), YmRType_ParcelDef);
}

TEST(ParcelDefs, RC) {
    SETUP_ERRCOUNTER;
    SETUP_PARCELDEF(parceldef);
    ASSERT_EQ(ymRefCount(parceldef), 1); // Initial value.
    ymAddRef(parceldef);
    EXPECT_EQ(ymRefCount(parceldef), 2);
    ymAddRef(parceldef);
    EXPECT_EQ(ymRefCount(parceldef), 3);
    ymDrop(parceldef);
    EXPECT_EQ(ymRefCount(parceldef), 2);
    ymDrop(parceldef);
    EXPECT_EQ(ymRefCount(parceldef), 1);
    // ScopedDrop will drop final one, and the actual release of
    // the resource is unobservable.
}

