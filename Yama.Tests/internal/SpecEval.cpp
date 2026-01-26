

#include <gtest/gtest.h>
#include <internal/SpecParser.h>


class TestImpl final : public _ym::SpecEval {
public:
    std::string output;


    TestImpl() = default;


    std::optional<std::string> operator()(const _ym::SpecParser::Result& x) {
        output = "\n";
        return
            eval(x)
            ? std::make_optional(output)
            : std::nullopt;
    }

    void syntaxErr() override { output += "syntaxErr\n"; }
    void rootId(const taul::str& id) override { output += std::format("rootId {}\n", id); }
    void openArgs() override { output += "openArgs\n"; }
    void closeArgs() override { output += "closeArgs\n"; }
    void slashId(const taul::str& id) override { output += std::format("slashId {}\n", id); }
    void colonId(const taul::str& id) override { output += std::format("colonId {}\n", id); }
    void dblColonId(const taul::str& id) override { output += std::format("dblColonId {}\n", id); }
};

void test_(int line, bool shouldSucceed, const std::string& input, const std::string& output) {
    TestImpl sp{};
    auto actual = TestImpl{}(_ym::SpecParser{}(taul::str(input)));
    ASSERT_EQ(actual.has_value(), shouldSucceed)
        << "input: " << input
        << "\nline: " << line;
    if (shouldSucceed) {
        EXPECT_EQ(actual.value(), output)
            << "input: " << input
            << "\nline: " << line;
    }
}

#define succeed(input, output) test_(__LINE__, true, (input), (output))
#define fail(input, output) test_(__LINE__, false, (input), (output))


TEST(SpecParser, Usage) {
    succeed("math/vec:Vec3::length", R"(
rootId math
slashId vec
colonId Vec3
dblColonId length
)");
    succeed("yama:List[math/vec:Vec3]::size", R"(
rootId yama
colonId List
openArgs
rootId math
slashId vec
colonId Vec3
closeArgs
dblColonId size
)");
    succeed("%yama:%List[%math/%vec:%Vec3]::%size", R"(
rootId %yama
colonId %List
openArgs
rootId %math
slashId %vec
colonId %Vec3
closeArgs
dblColonId %size
)");
    succeed("$yama:$List[$math/$vec:$Vec3]::$size", R"(
rootId $yama
colonId $List
openArgs
rootId $math
slashId $vec
colonId $Vec3
closeArgs
dblColonId $size
)");
    // Invalid, but legal at this level of abstraction.
    succeed("A::B/C:D", R"(
rootId A
dblColonId B
slashId C
colonId D
)");

    fail("yama:L$ist", R"(
rootId yama
colonId L
syntaxErr
)");
}

