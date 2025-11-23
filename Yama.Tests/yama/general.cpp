

#include <gtest/gtest.h>
#include <yama/yama.h>


TEST(General, GIDCreation) {
	EXPECT_EQ(ymGID(0, 0), YmGID(0));
	EXPECT_EQ(ymGID(5, 3), YmGID((5 << 16) | 3));
	EXPECT_EQ(ymGID(YmPID(-1), YmLID(-1)), YmGID(-1));
}

TEST(General, PIDFromGID) {
	EXPECT_EQ(ymGID_PID(ymGID(0, 0)), YmPID(0));
	EXPECT_EQ(ymGID_PID(ymGID(5, 3)), YmPID(5));
	EXPECT_EQ(ymGID_PID(ymGID(YmPID(-1), YmLID(-1))), YmPID(-1));
}

TEST(General, LIDFromGID) {
	EXPECT_EQ(ymGID_LID(ymGID(0, 0)), YmLID(0));
	EXPECT_EQ(ymGID_LID(ymGID(5, 3)), YmLID(3));
	EXPECT_EQ(ymGID_LID(ymGID(YmPID(-1), YmLID(-1))), YmLID(-1));
}

