

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


    template<typename... Args>
    void emit(std::format_string<Args...> fmt, Args&&... args) {
        output += std::format(fmt, std::forward<Args>(args)...);
    }

    void syntaxErr() override { emit("{}\n", __func__); }
    void rootId(const taul::str& id) override { emit("{} {}\n", __func__, id); }
    void slashId(const taul::str& id) override { emit("{} {}\n", __func__, id); }
    void colonId(const taul::str& id) override { emit("{} {}\n", __func__, id); }
    void dblColonId(const taul::str& id) override { emit("{} {}\n", __func__, id); }
    void openItemArgs() override { emit("{}\n", __func__); }
    void itemArgsArgDelimiter() override { emit("{}\n", __func__); }
    void closeItemArgs() override { emit("{}\n", __func__); }
    void openCallSuff() override { emit("{}\n", __func__); }
    void callSuffParamDelimiter() override { emit("{}\n", __func__); }
    void callSuffReturnType() override { emit("{}\n", __func__); }
    void closeCallSuff() override { emit("{}\n", __func__); }
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
openItemArgs
rootId math
slashId vec
colonId Vec3
closeItemArgs
dblColonId size
)");
    succeed("%yama:%List[%math/%vec:%Vec3]::%size", R"(
rootId %yama
colonId %List
openItemArgs
rootId %math
slashId %vec
colonId %Vec3
closeItemArgs
dblColonId %size
)");
    succeed("$yama:$List[$math/$vec:$Vec3]::$size", R"(
rootId $yama
colonId $List
openItemArgs
rootId $math
slashId $vec
colonId $Vec3
closeItemArgs
dblColonId $size
)");
    // Test w/ multiple item args.
    succeed("yama:Map[yama:Str, yama:Int]::contains", R"(
rootId yama
colonId Map
openItemArgs
rootId yama
colonId Str
itemArgsArgDelimiter
rootId yama
colonId Int
closeItemArgs
dblColonId contains
)");
    succeed("p:A[p:B](yama:Int, yama:Float) -> yama:None", R"(
rootId p
colonId A
openItemArgs
rootId p
colonId B
closeItemArgs
openCallSuff
rootId yama
colonId Int
callSuffParamDelimiter
rootId yama
colonId Float
callSuffReturnType
rootId yama
colonId None
closeCallSuff
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

