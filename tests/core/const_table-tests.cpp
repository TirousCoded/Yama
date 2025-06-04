

#include <gtest/gtest.h>

#include <yama/core/parcel.h>
#include <yama/core/domain.h>
#include <yama/core/const_table.h>
#include <yama/core/type.h>


using namespace yama::string_literals;


namespace {
    class test_parcel final : public yama::parcel {
    public:
        std::optional<yama::parcel_metadata> md;
        std::unordered_map<yama::str, yama::res<yama::module_info>> mods;


        test_parcel() = default;


        void push(const yama::str& relative_path, yama::module_info&& m) {
            mods.insert({ relative_path, yama::make_res<yama::module_info>(std::forward<yama::module_info>(m)) });
        }


        const yama::parcel_metadata& metadata() override final {
            if (!md) md = yama::parcel_metadata{ "self"_str, { "yama"_str } };
            return *md;
        }
        std::optional<yama::import_result> import(const yama::str & relative_path) override final {
            if (const auto it = mods.find(relative_path); it != mods.end()) {
                return yama::import_result(yama::res(it->second));
            }
            else return std::nullopt;
        }
    };
}


class ConstTableTests : public testing::Test {
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


TEST_F(ConstTableTests, CopyCtor) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a_type = dm->load("a:abc"_str).value();

    yama::const_table a = a_type;
    yama::const_table b(a); // copy a

    ASSERT_EQ(b, a); // compare by ref
}

TEST_F(ConstTableTests, MoveCtor) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a_type = dm->load("a:abc"_str).value();

    yama::const_table a = a_type;
    yama::const_table b(std::move(a)); // move a

    ASSERT_EQ(b, yama::const_table(a_type)); // compare by ref
}

TEST_F(ConstTableTests, CopyAssign) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    auto def_constsinf =
        yama::const_table_info()
        .add_int(10'000); // <- structurally identical
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);
    mf.add_primitive_type("def"_str, std::move(def_constsinf), yama::ptype::bool0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a_type = dm->load("a:abc"_str).value();
    yama::type b_type = dm->load("a:def"_str).value();

    yama::const_table a = a_type;
    yama::const_table b = b_type;

    ASSERT_NE(b, a); // compare by ref

    b = a; // copy a

    ASSERT_EQ(b, a); // compare by ref
}

TEST_F(ConstTableTests, MoveAssign) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    auto def_constsinf =
        yama::const_table_info()
        .add_int(10'000); // <- structurally identical
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);
    mf.add_primitive_type("def"_str, std::move(def_constsinf), yama::ptype::bool0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a_type = dm->load("a:abc"_str).value();
    yama::type b_type = dm->load("a:def"_str).value();

    yama::const_table a = a_type;
    yama::const_table b = b_type;

    ASSERT_NE(b, yama::const_table(a_type)); // compare by ref

    b = std::move(a); // move a

    ASSERT_EQ(b, yama::const_table(a_type)); // compare by ref
}

TEST_F(ConstTableTests, Size) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000)
        .add_int(10'000)
        .add_int(10'000)
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.size(), 4);
}

TEST_F(ConstTableTests, IsStub) {
    // TODO: test stubs which aren't from an out-of-bounds

    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_FALSE(a.is_stub(0));
    EXPECT_FALSE(a.is_stub(1));
    EXPECT_FALSE(a.is_stub(2));
    EXPECT_FALSE(a.is_stub(3));

    EXPECT_TRUE(a.is_stub(4)); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableTests, IsStub_OutOfBounds) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_TRUE(a.is_stub(4)); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableTests, Get) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::float_const>(0), std::make_optional(3.21));
    EXPECT_EQ(a.get<yama::int_const>(1), std::make_optional(-3));
    EXPECT_EQ(a.get<yama::primitive_type_const>(2), dm->load("yama:Int"_str));
    EXPECT_EQ(a.get<yama::uint_const>(3), std::make_optional(13u));
}

TEST_F(ConstTableTests, Get_OutOfBounds) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::int_const>(4), std::nullopt); // out-of-bounds == (pseudo-)stub
}

TEST_F(ConstTableTests, Get_WrongConstType) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.get<yama::int_const>(0), std::nullopt); // wrong const_type
}

TEST_F(ConstTableTests, Get_Stub) {
    // TODO: stub
}

TEST_F(ConstTableTests, ConstType) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.const_type(0), std::make_optional(yama::float_const));
    EXPECT_EQ(a.const_type(1), std::make_optional(yama::int_const));
    EXPECT_EQ(a.const_type(2), std::make_optional(yama::primitive_type_const));
    EXPECT_EQ(a.const_type(3), std::make_optional(yama::uint_const));
}

TEST_F(ConstTableTests, ConstType_OutOfBounds) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_float(3.21)
        .add_int(-3)
        .add_primitive_type("yama:Int"_str)
        .add_uint(13);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.const_type(4), std::nullopt); // out-of-bounds
}

TEST_F(ConstTableTests, Type) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(-3)
        .add_uint(13)
        .add_float(3.21)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.type(0), std::nullopt);
    EXPECT_EQ(a.type(1), std::nullopt);
    EXPECT_EQ(a.type(2), std::nullopt);
    EXPECT_EQ(a.type(3), std::nullopt);
    EXPECT_EQ(a.type(4), std::nullopt);
    EXPECT_EQ(a.type(5), dm->load("yama:Int"_str));
    EXPECT_EQ(a.type(6), dm->load("yama:Float"_str));
}

TEST_F(ConstTableTests, Type_OutOfBounds) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(-3)
        .add_uint(13)
        .add_float(3.21)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str);
    yama::module_factory mf{};
    mf.add_primitive_type("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::const_table a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.type(7), std::nullopt); // out-of-bounds
}

TEST_F(ConstTableTests, Equality) {
    yama::const_table a1 = dm->int_type();
    yama::const_table a2 = dm->int_type();
    yama::const_table b = dm->uint_type();
    yama::const_table c = dm->float_type();

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

