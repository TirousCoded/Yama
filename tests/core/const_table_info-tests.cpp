

#include <gtest/gtest.h>

#include <yama/debug-impls/stderr_debug.h>
#include <yama/core/const_table_info.h>


using namespace yama::string_literals;


TEST(ConstTableInfoTests, Construction) {
    static_assert(yama::const_types == 7);
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    EXPECT_EQ(a.size(), yama::const_types);
    EXPECT_EQ(a.size(), a.consts.size());

    static_assert(yama::const_types == 7);
    ASSERT_TRUE(a.get<yama::int_const>(0));
    ASSERT_TRUE(a.get<yama::uint_const>(1));
    ASSERT_TRUE(a.get<yama::float_const>(2));
    ASSERT_TRUE(a.get<yama::bool_const>(3));
    ASSERT_TRUE(a.get<yama::char_const>(4));
    ASSERT_TRUE(a.get<yama::primitive_type_const>(5));
    ASSERT_TRUE(a.get<yama::function_type_const>(6));

    static_assert(yama::const_types == 7);
    EXPECT_EQ(*a.get<yama::int_const>(0), yama::int_const_info{ -4 });
    EXPECT_EQ(*a.get<yama::uint_const>(1), yama::uint_const_info{ 301 });
    EXPECT_EQ(*a.get<yama::float_const>(2), yama::float_const_info{ 3.14159 });
    EXPECT_EQ(*a.get<yama::bool_const>(3), yama::bool_const_info{ true });
    EXPECT_EQ(*a.get<yama::char_const>(4), yama::char_const_info{ U'y' });
    EXPECT_EQ(*a.get<yama::primitive_type_const>(5), yama::primitive_type_const_info{ "abc"_str });
    // NOTE: need 'ff' to stop the macro from messing up
    const auto ff = yama::function_type_const_info{ "def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5) };
    EXPECT_EQ(*a.get<yama::function_type_const>(6), ff);

    YAMA_LOG(std::make_shared<yama::stderr_debug>(), yama::general_c, "{}", a);
}

TEST(ConstTableInfoTests, Size) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_primitive_type("abc"_str)
        .add_float(3.14159);

    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(a.size(), a.consts.size());
}

TEST(ConstTableInfoTests, Get) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_primitive_type("abc"_str)
        .add_float(3.14159);

    ASSERT_TRUE(a.get<yama::int_const>(0));
    ASSERT_TRUE(a.get<yama::primitive_type_const>(1));
    ASSERT_TRUE(a.get<yama::float_const>(2));

    EXPECT_EQ(*a.get<yama::int_const>(0), yama::int_const_info{ -4 });
    EXPECT_EQ(*a.get<yama::primitive_type_const>(1), yama::primitive_type_const_info{ "abc"_str });
    EXPECT_EQ(*a.get<yama::float_const>(2), yama::float_const_info{ 3.14159 });
}

TEST(ConstTableInfoTests, Get_OutOfBounds) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_int(-4)
        .add_int(-4);

    ASSERT_TRUE(a.get<yama::int_const>(0));
    ASSERT_TRUE(a.get<yama::int_const>(1));
    ASSERT_TRUE(a.get<yama::int_const>(2));
    ASSERT_FALSE(a.get<yama::int_const>(3)); // out-of-bounds
}

TEST(ConstTableInfoTests, Get_WrongConstType) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159);

    ASSERT_FALSE(a.get<yama::uint_const>(0)); // failed
    ASSERT_TRUE(a.get<yama::uint_const>(1));
    ASSERT_FALSE(a.get<yama::uint_const>(2)); // failed
}

TEST(ConstTableInfoTests, ConstType) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_primitive_type("abc"_str)
        .add_float(3.14159);

    EXPECT_EQ(a.const_type(0), std::make_optional(yama::int_const));
    EXPECT_EQ(a.const_type(1), std::make_optional(yama::primitive_type_const));
    EXPECT_EQ(a.const_type(2), std::make_optional(yama::float_const));
}

TEST(ConstTableInfoTests, ConstType_OutOfBounds) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_primitive_type("abc"_str)
        .add_float(3.14159);

    EXPECT_EQ(a.const_type(3), std::nullopt); // out-of-bounds
}

TEST(ConstTableInfoTests, Kind) {
    static_assert(yama::const_types == 7);
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    EXPECT_EQ(a.kind(0), std::nullopt);
    EXPECT_EQ(a.kind(1), std::nullopt);
    EXPECT_EQ(a.kind(2), std::nullopt);
    EXPECT_EQ(a.kind(3), std::nullopt);
    EXPECT_EQ(a.kind(4), std::nullopt);
    EXPECT_EQ(a.kind(5), std::make_optional(yama::kind::primitive));
    EXPECT_EQ(a.kind(6), std::make_optional(yama::kind::function));
}

TEST(ConstTableInfoTests, Kind_OutOfBounds) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159);

    EXPECT_EQ(a.kind(3), std::nullopt); // out-of-bounds
}

TEST(ConstTableInfoTests, Fullname) {
    static_assert(yama::const_types == 7);
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    EXPECT_EQ(a.fullname(0), std::nullopt);
    EXPECT_EQ(a.fullname(1), std::nullopt);
    EXPECT_EQ(a.fullname(2), std::nullopt);
    EXPECT_EQ(a.fullname(3), std::nullopt);
    EXPECT_EQ(a.fullname(4), std::nullopt);
    EXPECT_EQ(a.fullname(5), std::make_optional("abc"_str));
    EXPECT_EQ(a.fullname(6), std::make_optional("def"_str));
}

TEST(ConstTableInfoTests, Fullname_OutOfBounds) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159);

    EXPECT_EQ(a.fullname(3), std::nullopt); // out-of-bounds
}

TEST(ConstTableInfoTests, CallSig) {
    static_assert(yama::const_types == 7);
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    EXPECT_FALSE(a.callsig(0));
    EXPECT_FALSE(a.callsig(1));
    EXPECT_FALSE(a.callsig(2));
    EXPECT_FALSE(a.callsig(3));
    EXPECT_FALSE(a.callsig(4));
    EXPECT_FALSE(a.callsig(5));
    EXPECT_TRUE(a.callsig(6));

    if (a.callsig(6)) EXPECT_EQ(*a.callsig(6), yama::make_callsig_info({ 5, 6, 5 }, 5));
}

TEST(ConstTableInfoTests, CallSig_OutOfBounds) {
    const auto a =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159);

    EXPECT_FALSE(a.callsig(3)); // out-of-bounds
}

TEST(ConstTableInfoTests, Equality) {
    // this equality test is mainly just to ensure that C++ generated
    // the expected operator== for our #_const_info structs, and the
    // std::variant and std::vector containing them

    const auto a1 =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    const auto a2 =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5, 6, 5 }, 5));

    const auto b =
        yama::const_table_info()
        .add_int(-4)
        //.add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        //.add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 4, 4, 4 }, 4));

    EXPECT_EQ(a1, a1);
    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, b);

    EXPECT_EQ(a2, a2);
    EXPECT_NE(a2, b);

    EXPECT_EQ(b, b);
}

