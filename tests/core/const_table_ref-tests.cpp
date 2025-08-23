

#include <gtest/gtest.h>

#include <yama/core/module.h>
#include <yama/core/parcel.h>
#include <yama/core/domain.h>
#include <yama/core/const_table_ref.h>
#include <yama/core/item_ref.h>

#include "../utils/module_helper.h"


using namespace yama::string_literals;


namespace {
    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::unordered_map<yama::str, yama::res<yama::module>> mods;


        test_parcel() = default;


        void push(const yama::str& relative_path, yama::module m) {
            mods.insert({ relative_path, yama::make_res<yama::module>(std::move(m)) });
        }


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (const auto it = mods.find(relative_path); it != mods.end()) {
                return yama::import_result(yama::res(it->second));
            }
            else return std::nullopt;
        }
    };
}


class ConstTableRefTests : public testing::Test {
public:
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<test_parcel> parcel;


protected:
    void SetUp() override {
        dbg = std::make_shared<yama::stderr_debug>();
        dm = std::make_shared<yama::domain>(dbg);
        parcel = std::make_shared<test_parcel>();

        yama::install_batch ib{};
        ib
            .install("a"_str, yama::res(parcel))
            .map_dep("a"_str, "yama"_str, "yama"_str);
        dm->install(std::move(ib));
    }
    void TearDown() override {
        //
    }
};


TEST_F(ConstTableRefTests, CopyCtor) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::item_ref a_type = dm->load("a:abc"_str).value();

    yama::const_table_ref a = a_type;
    yama::const_table_ref b(a); // copy a

    ASSERT_EQ(b, a); // compare by ref
}

TEST_F(ConstTableRefTests, MoveCtor) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::item_ref a_type = dm->load("a:abc"_str).value();

    yama::const_table_ref a = a_type;
    yama::const_table_ref b(std::move(a)); // move a

    ASSERT_EQ(b, yama::const_table_ref(a_type)); // compare by ref
}

TEST_F(ConstTableRefTests, CopyAssign) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    auto def_consts =
        yama::const_table()
        .add_int(10'000); // <- structurally identical
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);
    mh.add_primitive("def"_str, def_consts, yama::ptype::bool0);

    parcel->push(""_str, mh.result());

    yama::item_ref a_type = dm->load("a:abc"_str).value();
    yama::item_ref b_type = dm->load("a:def"_str).value();

    yama::const_table_ref a = a_type;
    yama::const_table_ref b = b_type;

    ASSERT_NE(b, a); // compare by ref

    b = a; // copy a

    ASSERT_EQ(b, a); // compare by ref
}

TEST_F(ConstTableRefTests, MoveAssign) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    auto def_consts =
        yama::const_table()
        .add_int(10'000); // <- structurally identical
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);
    mh.add_primitive("def"_str, def_consts, yama::ptype::bool0);

    parcel->push(""_str, mh.result());

    yama::item_ref a_type = dm->load("a:abc"_str).value();
    yama::item_ref b_type = dm->load("a:def"_str).value();

    yama::const_table_ref a = a_type;
    yama::const_table_ref b = b_type;

    ASSERT_NE(b, yama::const_table_ref(a_type)); // compare by ref

    b = std::move(a); // move a

    ASSERT_EQ(b, yama::const_table_ref(a_type)); // compare by ref
}

TEST_F(ConstTableRefTests, Size) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000)
        .add_int(10'000)
        .add_int(10'000)
        .add_int(10'000);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.size(), 4);
}

TEST_F(ConstTableRefTests, IsStub) {
    // TODO: test stubs which aren't from an out-of-bounds

    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_FALSE(a.is_stub(0));
    EXPECT_FALSE(a.is_stub(1));
    EXPECT_FALSE(a.is_stub(2));
    EXPECT_FALSE(a.is_stub(3));

    EXPECT_TRUE(a.is_stub(4)); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableRefTests, IsStub_OutOfBounds) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_TRUE(a.is_stub(4)); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableRefTests, Get) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::float_const>(0), std::make_optional(3.21));
    EXPECT_EQ(a.get<yama::int_const>(1), std::make_optional(-3));
    EXPECT_EQ(a.get<yama::primitive_type_const>(2), dm->load("yama:Int"_str));
    EXPECT_EQ(a.get<yama::uint_const>(3), std::make_optional(13u));
}

TEST_F(ConstTableRefTests, Get_OutOfBounds) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::int_const>(4), std::nullopt); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableRefTests, Get_WrongConstType) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::int_const>(0), std::nullopt); // wrong const_type
}

TEST_F(ConstTableRefTests, Get_Stub) {
    // TODO: stub
}

TEST_F(ConstTableRefTests, ConstType) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.const_type(0), std::make_optional(yama::float_const));
    EXPECT_EQ(a.const_type(1), std::make_optional(yama::int_const));
    EXPECT_EQ(a.const_type(2), std::make_optional(yama::primitive_type_const));
    EXPECT_EQ(a.const_type(3), std::make_optional(yama::uint_const));
}

TEST_F(ConstTableRefTests, ConstType_OutOfBounds) {
    auto abc_consts =
        yama::const_table()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.const_type(4), std::nullopt); // out-of-bounds
}

TEST_F(ConstTableRefTests, Type) {
    auto abc_consts =
        yama::const_table()
        .add_int(-3)
        .add_uint(13)
        .add_float(3.21)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.type(0), std::nullopt);
    EXPECT_EQ(a.type(1), std::nullopt);
    EXPECT_EQ(a.type(2), std::nullopt);
    EXPECT_EQ(a.type(3), std::nullopt);
    EXPECT_EQ(a.type(4), std::nullopt);
    EXPECT_EQ(a.type(5), dm->load("yama:Int"_str));
    EXPECT_EQ(a.type(6), dm->load("yama:Float"_str));
}

TEST_F(ConstTableRefTests, Type_OutOfBounds) {
    auto abc_consts =
        yama::const_table()
        .add_int(-3)
        .add_uint(13)
        .add_float(3.21)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str);
    module_helper mh{};
    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    parcel->push(""_str, mh.result());

    yama::const_table_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.type(7), std::nullopt); // out-of-bounds
}

TEST_F(ConstTableRefTests, Equality) {
    yama::const_table_ref a1 = dm->int_type();
    yama::const_table_ref a2 = dm->int_type();
    yama::const_table_ref b = dm->uint_type();
    yama::const_table_ref c = dm->float_type();

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

