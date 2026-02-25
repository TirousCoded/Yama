

#pragma once


#include <array>

#include <yama/yama.h>


class ErrCounter final {
public:
	ErrCounter() = default;


	size_t count(YmErrCode code) const noexcept;
	size_t operator[](YmErrCode code) const noexcept;

	void report(YmErrCode code, const YmChar* msg);
	void reset() noexcept;

	void setSuppressLog(bool suppress) noexcept;

	void setupCallbackForThisThread();


private:
	std::array<size_t, YmErrCode_Num> _counts = {0};
	bool _suppressLog = false;
};

#define SETUP_ERRCOUNTER ::ErrCounter err{}; err.setupCallbackForThisThread()

