

#include <gtest/gtest.h>

#include <yama/core/type_data.h>
#include <yama/core/type-info-structs.h>


using namespace yama::string_literals;


template<typename T, size_t Extent>
static bool span_eq(std::span<T, Extent> a, std::span<T, Extent> b) noexcept {
    return
        a.size() == b.size() &&
        std::equal(a.begin(), a.end(), b.begin());
}


// use these to test basics w/out having to worry about updating our tests
// as we change things like yama::primitive_info

struct test_primitive_info final : yama::type_info {
    size_t value;


    static constexpr auto kind() noexcept { return yama::kind::primitive; }
    static constexpr bool uses_callsig() noexcept { return false; }
};

struct test_function_info final : yama::type_info {
    size_t value;


    static constexpr auto kind() noexcept { return yama::kind::function; }
    static constexpr bool uses_callsig() noexcept { return true; }
};


// just some data that appears in our tests over-and-over

static std::vector<yama::linksym> linksyms0{
    yama::make_linksym("a"_str, yama::kind::primitive),
    yama::make_linksym("b"_str, yama::kind::primitive),
    yama::make_linksym("c"_str, yama::kind::primitive),
};

static std::vector<yama::linksym> linksyms1{
    yama::make_linksym("aa"_str, yama::kind::primitive),
    yama::make_linksym("bb"_str, yama::kind::primitive),
    yama::make_linksym("cc"_str, yama::kind::primitive),
    yama::make_linksym("dd"_str, yama::kind::primitive),
};

static yama::callsig_info callsig_info0 = yama::make_callsig_info({ 0, 1, 2 }, 1);

static yama::callsig_info callsig_info1 = yama::make_callsig_info({ 2, 0, 1 }, 0);


TEST(TypeDataTests, TypeInfoInjectionCtor) {
    yama::type_data a(
        test_primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            31,
        });

    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(a.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(a.info<test_primitive_info>().value, 31);
}

TEST(TypeDataTests, CopyCtor) {
    yama::type_data a(
        test_primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            31,
        });

    yama::type_data b(a);

    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(a.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(a.info<test_primitive_info>().value, 31);

    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(b.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(b.info<test_primitive_info>().value, 31);
}

TEST(TypeDataTests, MoveCtor) {
    yama::type_data a(
        test_primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            31,
        });

    yama::type_data b(std::move(a));

    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(b.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(b.info<test_primitive_info>().value, 31);
}

TEST(TypeDataTests, CopyAssign) {
    yama::type_data a(
        test_primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            31,
        });
    yama::type_data b(
        test_function_info{
            "def"_str,
            std::make_optional(callsig_info1),
            linksyms1,
            100,
        });

    b = a;

    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(a.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(a.info<test_primitive_info>().value, 31);

    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(b.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(b.info<test_primitive_info>().value, 31);
}

TEST(TypeDataTests, MoveAssign) {
    yama::type_data a(
        test_primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            31,
        });
    yama::type_data b(
        test_function_info{
            "def"_str,
            std::make_optional(callsig_info1),
            linksyms1,
            100,
        });

    b = std::move(a);

    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(b.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.info<test_primitive_info>().fullname, "abc"_str);
    EXPECT_EQ(b.info<test_primitive_info>().value, 31);
}

static_assert(yama::kinds == 2);

// here we use the actual yama::***_info types

TEST(TypeDataTests, PrimitiveInfo) {
    yama::type_data a(
        yama::primitive_info{
            "abc"_str,
            std::nullopt,
            linksyms0,
        });

    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.callsig(), std::nullopt);
    EXPECT_TRUE(span_eq(a.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.info<yama::primitive_info>().fullname, "abc"_str);
}

TEST(TypeDataTests, FunctionInfo) {
    yama::type_data a(
        yama::function_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
        });

    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.callsig(), std::make_optional(callsig_info0));
    EXPECT_TRUE(span_eq(a.linksyms(), std::span<const yama::linksym>(linksyms0)));
    EXPECT_EQ(a.kind(), yama::kind::function);
    EXPECT_EQ(a.info<yama::function_info>().fullname, "abc"_str);
}
