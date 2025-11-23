


#include <gtest/gtest.h>
#include <yama/yama.h>

#include <unordered_set>

#include "../utils/utils.h"


TEST(ParcelIters, PriorToFirstStart) {
    SETUP_DM;
    SETUP_CTX(ctx);
    EXPECT_EQ(ymParcelIter_Get(), nullptr);
    ymParcelIter_Advance(1);
    EXPECT_EQ(ymParcelIter_Get(), nullptr);
}

TEST(ParcelIters, Traversal) {
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    SETUP_PARCELDEF(d_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    BIND_AND_IMPORT(ctx, d, d_def, "d");
    SETUP_CTX(ctx2);
    SETUP_PARCELDEF(unseen_def);
    BIND_AND_IMPORT(ctx2, unseen, unseen_def, "unseen"); // ctx can't see this.
    std::unordered_set<YmParcel*> visited{};
    ymParcelIter_Start(ctx);
    for (; !ymParcelIter_Done(); ymParcelIter_Advance(1)) {
        visited.insert(ymParcelIter_Get());
    }
    EXPECT_EQ(visited.size(), 4);
    EXPECT_TRUE(visited.contains(a));
    EXPECT_TRUE(visited.contains(b));
    EXPECT_TRUE(visited.contains(c));
    EXPECT_TRUE(visited.contains(d));
}

TEST(ParcelIters, TraversalFrom) {
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    SETUP_PARCELDEF(d_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    BIND_AND_IMPORT(ctx, d, d_def, "d");
    std::unordered_set<YmParcel*> expected{}, actual{};
    YmParcel* startFrom{};
    ymParcelIter_Start(ctx);
    ymParcelIter_Advance(1); // Skip first one.
    startFrom = ymParcelIter_Get(); // Start from this.
    ASSERT_NE(startFrom, nullptr);
    for (; !ymParcelIter_Done(); ymParcelIter_Advance(1)) {
        expected.insert(ymParcelIter_Get());
    }
    ASSERT_EQ(expected.size(), 3);
    ymParcelIter_StartFrom(ctx, startFrom);
    for (; !ymParcelIter_Done(); ymParcelIter_Advance(1)) {
        actual.insert(ymParcelIter_Get());
    }
    EXPECT_EQ(expected, actual);
}

TEST(ParcelIters, Advance_IncrEquiv) {
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    SETUP_PARCELDEF(d_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    BIND_AND_IMPORT(ctx, d, d_def, "d");
    YmParcel* expected{}, * actual{};
    // Traversal by 3 incrs of 1.
    ymParcelIter_Start(ctx);
    for (size_t i = 0; i < 3; i++) {
        ymParcelIter_Advance(1);
    }
    expected = ymParcelIter_Get();
    EXPECT_NE(expected, nullptr);
    // Traversal by 1 incr of 3.
    ymParcelIter_Start(ctx);
    ymParcelIter_Advance(3);
    actual = ymParcelIter_Get();
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(expected, actual);
}

TEST(ParcelIters, Advance_PastTheEnd) {
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    SETUP_PARCELDEF(d_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    BIND_AND_IMPORT(ctx, d, d_def, "d");
    ymParcelIter_Start(ctx);
    ymParcelIter_Advance(4); // Iterate past everything.
    ASSERT_EQ(ymParcelIter_Get(), nullptr);
    ymParcelIter_Advance(1); // Keep iterating.
    EXPECT_EQ(ymParcelIter_Get(), nullptr);
}

TEST(ParcelIters, Advance_Zero) {
    SETUP_DM;
    SETUP_CTX(ctx);
    SETUP_PARCELDEF(a_def);
    SETUP_PARCELDEF(b_def);
    SETUP_PARCELDEF(c_def);
    SETUP_PARCELDEF(d_def);
    BIND_AND_IMPORT(ctx, a, a_def, "a");
    BIND_AND_IMPORT(ctx, b, b_def, "b");
    BIND_AND_IMPORT(ctx, c, c_def, "c");
    BIND_AND_IMPORT(ctx, d, d_def, "d");
    ymParcelIter_Start(ctx);
    YmParcel* expected = ymParcelIter_Get();
    ASSERT_NE(expected, nullptr);
    ymParcelIter_Advance(0); // Should do nothing.
    YmParcel* actual = ymParcelIter_Get();
    EXPECT_EQ(expected, actual);
}

