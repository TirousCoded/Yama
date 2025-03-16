

#include <gtest/gtest.h>

#include <yama/core/const_table.h>
#include <yama/core/type.h>

// TODO: type_instance used to be in frontend, but now it isn't,
//       meaning our unit tests have backend code dependence,
//       which is undesirable
//
//       I'm thinking maybe pull type_instance back to the frontend
//       at some point
#include <yama/internals/type_instance.h>


using namespace yama::string_literals;


static yama::internal::type_instance make_type_inst() {
    yama::type_info info{
        .unqualified_name = "A"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::int0,
        },
    };
    return yama::internal::type_instance("A"_str, yama::make_res<yama::type_info>(info));
}

static yama::res<yama::type_info> make_info_1() {
    // make const_table_info of *all* our constant types
    static_assert(yama::const_types == 7);
    auto consts =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("abc"_str)
        .add_function_type("def"_str, yama::make_callsig_info({ 5 }, 5));
    yama::type_info result{
        .unqualified_name = "info_1"_str,
        .consts = consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::int0,
        },
    };
    return yama::make_res<yama::type_info>(result);
}

static yama::res<yama::type_info> make_info_2() {
    // make const_table_info w/out regard for *all* constant types
    auto consts =
        yama::const_table_info()
        .add_float(3.14159)
        .add_int(-4)
        .add_primitive_type("abc"_str)
        .add_uint(301);
    yama::type_info result{
        .unqualified_name = "info_1"_str,
        .consts = consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::int0,
        },
    };
    return yama::make_res<yama::type_info>(result);
}


TEST(ConstTableTests, InstanceCtor) {
    // this tests init of const_table via the const_table_instance ctor
    const auto a_info = make_info_1();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.size(), yama::const_types);

    static_assert(yama::const_types == 7);
    EXPECT_EQ(a.const_type(0), std::make_optional(yama::int_const));
    EXPECT_EQ(a.const_type(1), std::make_optional(yama::uint_const));
    EXPECT_EQ(a.const_type(2), std::make_optional(yama::float_const));
    EXPECT_EQ(a.const_type(3), std::make_optional(yama::bool_const));
    EXPECT_EQ(a.const_type(4), std::make_optional(yama::char_const));
    EXPECT_EQ(a.const_type(5), std::make_optional(yama::primitive_type_const));
    EXPECT_EQ(a.const_type(6), std::make_optional(yama::function_type_const));

    static_assert(yama::const_types == 7);
    EXPECT_EQ(a.get<yama::int_const>(0), std::nullopt);
    EXPECT_EQ(a.get<yama::uint_const>(1), std::nullopt);
    EXPECT_EQ(a.get<yama::float_const>(2), std::nullopt);
    EXPECT_EQ(a.get<yama::bool_const>(3), std::nullopt);
    EXPECT_EQ(a.get<yama::char_const>(4), std::nullopt);
    EXPECT_EQ(a.get<yama::primitive_type_const>(5), std::nullopt);
    EXPECT_EQ(a.get<yama::function_type_const>(6), std::nullopt);
}

TEST(ConstTableTests, CopyCtor) {
    const auto info = make_info_1();
    yama::internal::type_instance a_inst(""_str, info);

    yama::const_table a(a_inst);
    yama::const_table b(a); // copy a

    ASSERT_EQ(b, a); // compare by inst ref
}

TEST(ConstTableTests, MoveCtor) {
    const auto info = make_info_1();
    yama::internal::type_instance a_inst(""_str, info);

    yama::const_table a(a_inst);
    yama::const_table b(std::move(a)); // move a

    ASSERT_EQ(b, yama::const_table(a_inst)); // compare by inst ref
}

TEST(ConstTableTests, CopyAssign) {
    const auto info = make_info_1();
    yama::internal::type_instance a_inst(""_str, info);
    yama::internal::type_instance b_inst(""_str, info);

    yama::const_table a(a_inst);
    yama::const_table b(b_inst);

    ASSERT_NE(b, a); // compare by inst ref

    b = a; // copy a

    ASSERT_EQ(b, a); // compare by inst ref
}

TEST(ConstTableTests, MoveAssign) {
    const auto info = make_info_1();
    yama::internal::type_instance a_inst(""_str, info);
    yama::internal::type_instance b_inst(""_str, info);

    yama::const_table a(a_inst);
    yama::const_table b(b_inst);

    ASSERT_NE(b, yama::const_table(a_inst)); // compare by inst ref

    b = std::move(a); // move a

    ASSERT_EQ(b, yama::const_table(a_inst)); // compare by inst ref
}

TEST(ConstTableTests, Complete) {
    const auto ti = make_type_inst();
    yama::type t(ti);

    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    ASSERT_FALSE(a.complete());

    a_inst.put<yama::float_const>(0, 3.21);

    ASSERT_FALSE(a.complete());

    a_inst.put<yama::int_const>(1, -3);

    ASSERT_FALSE(a.complete());

    a_inst.put<yama::primitive_type_const>(2, t);

    ASSERT_FALSE(a.complete());

    a_inst.put<yama::uint_const>(3, 13);

    ASSERT_TRUE(a.complete());
}

TEST(ConstTableTests, Size) {
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.size(), 4);
}

TEST(ConstTableTests, IsStub) {
    const auto ti = make_type_inst();
    yama::type t(ti);

    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_TRUE(a.is_stub(0));
    EXPECT_TRUE(a.is_stub(1));
    EXPECT_TRUE(a.is_stub(2));
    EXPECT_TRUE(a.is_stub(3));

    a_inst.put<yama::float_const>(0, 3.21);
    a_inst.put<yama::int_const>(1, -3);
    a_inst.put<yama::primitive_type_const>(2, t);
    a_inst.put<yama::uint_const>(3, 13);

    EXPECT_FALSE(a.is_stub(0));
    EXPECT_FALSE(a.is_stub(1));
    EXPECT_FALSE(a.is_stub(2));
    EXPECT_FALSE(a.is_stub(3));
}

TEST(ConstTableTests, IsStub_OutOfBounds) {
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_TRUE(a.is_stub(4)); // out-of-bounds == (pseudo-)stub
}

TEST(ConstTableTests, Get) {
    const auto ti = make_type_inst();
    yama::type t(ti);

    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    a_inst.put<yama::float_const>(0, 3.21);
    a_inst.put<yama::int_const>(1, -3);
    a_inst.put<yama::primitive_type_const>(2, t);
    a_inst.put<yama::uint_const>(3, 13);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.get<yama::float_const>(0), std::make_optional(3.21));
    EXPECT_EQ(a.get<yama::int_const>(1), std::make_optional(-3));
    EXPECT_EQ(a.get<yama::primitive_type_const>(2), std::make_optional(t));
    EXPECT_EQ(a.get<yama::uint_const>(3), std::make_optional(13u));
}

TEST(ConstTableTests, Get_OutOfBounds) {
    const auto ti = make_type_inst();
    yama::type t(ti);

    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    a_inst.put<yama::float_const>(0, 3.21);
    a_inst.put<yama::int_const>(1, -3);
    a_inst.put<yama::primitive_type_const>(2, t);
    a_inst.put<yama::uint_const>(3, 13);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.get<yama::int_const>(4), std::nullopt); // out-of-bounds
}

TEST(ConstTableTests, Get_WrongConstType) {
    const auto ti = make_type_inst();
    yama::type t(ti);

    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    a_inst.put<yama::float_const>(0, 3.21);
    a_inst.put<yama::int_const>(1, -3);
    a_inst.put<yama::primitive_type_const>(2, t);
    a_inst.put<yama::uint_const>(3, 13);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.get<yama::int_const>(0), std::nullopt); // wrong const_type
}

TEST(ConstTableTests, Get_Stub) {
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.get<yama::float_const>(0), std::nullopt); // stub
    EXPECT_EQ(a.get<yama::int_const>(1), std::nullopt); // stub
    EXPECT_EQ(a.get<yama::primitive_type_const>(2), std::nullopt); // stub
    EXPECT_EQ(a.get<yama::uint_const>(3), std::nullopt); // stub
}

TEST(ConstTableTests, ConstType) {
    // note that we're testing this w/ only stubs, as it should still work
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.const_type(0), std::make_optional(yama::float_const));
    EXPECT_EQ(a.const_type(1), std::make_optional(yama::int_const));
    EXPECT_EQ(a.const_type(2), std::make_optional(yama::primitive_type_const));
    EXPECT_EQ(a.const_type(3), std::make_optional(yama::uint_const));
}

TEST(ConstTableTests, ConstType_OutOfBounds) {
    // note that we're testing this w/ only stubs, as it should still work
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.const_type(4), std::nullopt); // out-of-bounds
}

TEST(ConstTableTests, Type) {
    const auto ti1 = make_type_inst();
    const auto ti2 = make_type_inst();
    yama::type t1(ti1);
    yama::type t2(ti2);

    const auto a_info = make_info_1();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);
    static_assert(yama::const_types == 7);
    a_inst.put<yama::int_const>(0, -4);
    a_inst.put<yama::uint_const>(1, 301);
    a_inst.put<yama::float_const>(2, 3.14159);
    a_inst.put<yama::bool_const>(3, true);
    a_inst.put<yama::char_const>(4, U'y');
    a_inst.put<yama::primitive_type_const>(5, t1);
    a_inst.put<yama::function_type_const>(6, t2);

    EXPECT_EQ(a.type(0), std::nullopt);
    EXPECT_EQ(a.type(1), std::nullopt);
    EXPECT_EQ(a.type(2), std::nullopt);
    EXPECT_EQ(a.type(3), std::nullopt);
    EXPECT_EQ(a.type(4), std::nullopt);
    EXPECT_EQ(a.type(5), std::make_optional(t1));
    EXPECT_EQ(a.type(6), std::make_optional(t2));
}

TEST(ConstTableTests, Type_OutOfBounds) {
    const auto a_info = make_info_2();
    yama::internal::type_instance a_inst(""_str, a_info);
    yama::const_table a(a_inst);

    EXPECT_EQ(a.type(4), std::nullopt); // out-of-bounds
}

TEST(ConstTableTests, Equality) {
    // use same info to ensure instances are structurally identical
    const auto info = make_info_2();
    yama::internal::type_instance a_inst(""_str, info);
    yama::internal::type_instance b_inst(""_str, info);
    yama::internal::type_instance c_inst(""_str, info);

    yama::const_table a1(a_inst);
    yama::const_table a2(a_inst);
    yama::const_table b(b_inst);
    yama::const_table c(c_inst);

    // equality

    EXPECT_TRUE(a1 == a1);
    EXPECT_TRUE(a1 == a2);
    EXPECT_FALSE(a1 == b);
    EXPECT_FALSE(a1 == c);

    EXPECT_TRUE(a2 == a2);
    EXPECT_FALSE(a2 == b);
    EXPECT_FALSE(a2 == c);

    EXPECT_TRUE(b == b);
    EXPECT_FALSE(b == c);

    EXPECT_TRUE(c == c);

    // inequality

    EXPECT_FALSE(a1 != a1);
    EXPECT_FALSE(a1 != a2);
    EXPECT_TRUE(a1 != b);
    EXPECT_TRUE(a1 != c);

    EXPECT_FALSE(a2 != a2);
    EXPECT_TRUE(a2 != b);
    EXPECT_TRUE(a2 != c);

    EXPECT_FALSE(b != b);
    EXPECT_TRUE(b != c);

    EXPECT_FALSE(c != c);
}

