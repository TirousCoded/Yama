

#include <gtest/gtest.h>
#include <internal/SpecSolver.h>
#include <yama++/Context.h>
#include <yama++/Domain.h>
#include <yama++/ParcelDef.h>
#include <yama++/print.h>


using namespace taul::string_literals;


TEST(SpecSolver, Substitution) {
	// TODO: Due to chicken-n'-egg problem, this can't really be tested right now.
	//		 Try writing a nice test for this later (maybe make SpecSolver env mockable?)
}

TEST(SpecSolver, Normalizes) {
	if (auto actual = _ym::SpecSolver()(
		"  a  /  \r\n  b   \r\n  :  A  [ a / b : B  [ a  / b :C  ] ::m  ,a/  b \r\n : D  :: m  ]  ::  m   "_str
		)) {
		EXPECT_EQ(*actual, "a/b:A[a/b:B[a/b:C]::m, a/b:D::m]::m");
	}
	else FAIL();
}

