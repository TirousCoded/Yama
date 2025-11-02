

#pragma once


#include <array>

#include <yama/yama.h>


class ErrCounter final {
public:
	ErrCounter() = default;


	YmWord count(YmErrCode code) const noexcept;
	YmWord operator[](YmErrCode code) const noexcept;

	void report(YmErrCode code, const YmChar* msg);
	void reset() noexcept;

	void setupCallbackForThisThread();


private:
	std::array<YmWord, YmErrCode_Num> _counts;
};

#define SETUP_ERRCOUNTER ::ErrCounter err{}; err.setupCallbackForThisThread()

