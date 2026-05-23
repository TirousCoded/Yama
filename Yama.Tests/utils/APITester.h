

#pragma once


#include <yama/yama.h>


namespace {


	class APITester final {
	public:
		bool good = true;


		APITester() = default;


		inline void fail() {
			good = false;
		}

		inline bool locals(YmLocals n) {

		}

		inline bool local(YmLocal where, YmType* type, YmRefCount refs) {

		}
		inline bool local_none(YmLocal where, YmRefCount refs) {

		}
		inline bool local_i(YmLocal where, YmInt v, YmRefCount refs) {

		}
		inline bool local_ui(YmLocal where, YmUInt v, YmRefCount refs) {

		}
		inline bool local_f(YmLocal where, YmFloat v, YmRefCount refs) {

		}
		inline bool local_b(YmLocal where, YmBool v, YmRefCount refs) {

		}
		inline bool local_r(YmLocal where, YmRune v, YmRefCount refs) {

		}
	};
}

