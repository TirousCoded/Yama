

#pragma once


#include <concepts>

#include <yama/yama.h>
#include <yama++/print.h>


namespace {
	template<typename T>
	struct Comparer final {
		static inline void compare(const T& a, const T& b) {
			EXPECT_EQ(a, b);
		}
	};
	template<>
	struct Comparer<YmFloat> final {
		static inline void compare(YmFloat a, YmFloat b) {
			EXPECT_DOUBLE_EQ(a, b);
		}
	};
}

static inline YmParseStatus parse(const YmChar* input, YmInt& output, size_t& bytes, bool) {
	return ymParseInt(input, &output, &bytes);
}
static inline YmParseStatus parse(const YmChar* input, YmUInt& output, size_t& bytes, bool ignoreU) {
	return ymParseUInt(input, &output, &bytes, ignoreU);
}
static inline YmParseStatus parse(const YmChar* input, YmFloat& output, size_t& bytes, bool) {
	return ymParseFloat(input, &output, &bytes);
}
static inline YmParseStatus parse(const YmChar* input, YmBool& output, size_t& bytes, bool) {
	return ymParseBool(input, &output, &bytes);
}
static inline YmParseStatus parse(const YmChar* input, YmRune& output, size_t& bytes, bool) {
	return ymParseRune(input, &output, &bytes);
}

template<typename T>
static inline void success(const YmChar* input, const T& expected, size_t expectedBytes, bool ignoreU = false) {
#if _YM_SUPPRESS_LOGGING == 0
	ym::println("success: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Success);
	Comparer<T>::compare(actual, expected);
	EXPECT_EQ(actualBytes, expectedBytes);
}
template<typename T>
static inline void failure(const YmChar* input, bool ignoreU = false) {
#ifndef _YM_SUPPRESS_LOGGING
	ym::println("failure: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Failure);
}
template<std::integral T>
static inline void overflow(const YmChar* input, size_t expectedBytes, bool ignoreU = false) {
#ifndef _YM_SUPPRESS_LOGGING
	ym::println("overflow: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Overflow);
	EXPECT_EQ(actualBytes, expectedBytes);
}
template<std::floating_point T>
static inline void overflow(const YmChar* input, size_t expectedBytes, bool ignoreU = false) {
#ifndef _YM_SUPPRESS_LOGGING
	ym::println("overflow: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Overflow);
	Comparer<T>::compare((YmFloat)actual, YM_INF);
	EXPECT_EQ(actualBytes, expectedBytes);
}
template<std::integral T>
static inline void underflow(const YmChar* input, size_t expectedBytes, bool ignoreU = false) {
#ifndef _YM_SUPPRESS_LOGGING
	ym::println("underflow: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Underflow);
	EXPECT_EQ(actualBytes, expectedBytes);
}
template<std::floating_point T>
static inline void underflow(const YmChar* input, size_t expectedBytes, bool ignoreU = false) {
#ifndef _YM_SUPPRESS_LOGGING
	ym::println("underflow: {}{}", input ? input : "<nullptr>", ignoreU ? " (ignoreU)" : "");
#endif
	T actual{};
	size_t actualBytes{};
	YmParseStatus status = parse(input, actual, actualBytes, ignoreU);
	ASSERT_EQ(status, YmParseStatus_Underflow);
	Comparer<T>::compare((YmFloat)actual, -YM_INF);
	EXPECT_EQ(actualBytes, expectedBytes);
}

