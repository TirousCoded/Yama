

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table.h>
#include <yama/core/type_info.h>
#include <yama/core/type.h>
#include <yama/core/domain.h>


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
        std::optional<yama::import_result> import(const yama::str& relative_path) override final {
            if (const auto it = mods.find(relative_path); it != mods.end()) {
                return yama::import_result(yama::res(it->second));
            }
            else return std::nullopt;
        }
    };
}


class TypeTests : public testing::Test {
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


// IMPORTANT: many of the basic query methods of yama::type are tested *in bulk* in
//            our per-kind tests


TEST_F(TypeTests, CopyCtor) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();
    yama::type b(a); // copy ctor

    ASSERT_EQ(a, b); // compare by ref
}

TEST_F(TypeTests, MoveCtor) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();
    yama::type b(std::move(a)); // move ctor

    ASSERT_EQ(b, dm->load("a:abc"_str).value()); // compare by ref
}

TEST_F(TypeTests, CopyAssign) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    auto def_constsinf =
        yama::const_table_info()
        .add_bool(true); // <- make b consts differ from a consts
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, std::move(abc_constsinf), yama::ptype::float0);
    mf.add_primitive("def"_str, std::move(def_constsinf), yama::ptype::bool0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();
    yama::type b = dm->load("a:def"_str).value();

    b = a; // copy assign

    ASSERT_EQ(a, b); // compare by ref
}

TEST_F(TypeTests, MoveAssign) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    auto def_constsinf =
        yama::const_table_info()
        .add_bool(true); // <- make b consts differ from a consts
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, std::move(abc_constsinf), yama::ptype::float0);
    mf.add_primitive("def"_str, std::move(def_constsinf), yama::ptype::bool0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();
    yama::type b = dm->load("a:def"_str).value();

    b = std::move(a); // move assign

    ASSERT_EQ(b, dm->load("a:abc"_str).value()); // compare by ref
}

TEST_F(TypeTests, Equality) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, yama::const_table_info(abc_constsinf), yama::ptype::float0);
    mf.add_primitive("def"_str, yama::const_table_info(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a0 = dm->load("a:abc"_str).value();
    yama::type a1 = dm->load("a:abc"_str).value();
    yama::type b = dm->load("a:def"_str).value();

    EXPECT_TRUE(a0 == a0);
    EXPECT_TRUE(a0 == a1);
    EXPECT_FALSE(a0 == b);

    EXPECT_TRUE(a1 == a0);
    EXPECT_TRUE(a1 == a1);
    EXPECT_FALSE(a1 == b);

    EXPECT_FALSE(b == a0);
    EXPECT_FALSE(b == a1);
    EXPECT_TRUE(b == b);

    EXPECT_FALSE(a0 != a0);
    EXPECT_FALSE(a0 != a1);
    EXPECT_TRUE(a0 != b);

    EXPECT_FALSE(a1 != a0);
    EXPECT_FALSE(a1 != a1);
    EXPECT_TRUE(a1 != b);

    EXPECT_TRUE(b != a0);
    EXPECT_TRUE(b != a1);
    EXPECT_FALSE(b != b);
}

static_assert(yama::kinds == 3);

TEST_F(TypeTests, PerKind_Primitive) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_int(10'000);
    yama::module_factory mf{};
    mf.add_primitive("abc"_str, std::move(abc_constsinf), yama::ptype::float0);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.callsig(), std::nullopt);
    EXPECT_EQ(a.call_fn(), std::nullopt);
    EXPECT_EQ(a.max_locals(), 0);
    EXPECT_EQ(a.consts(), yama::const_table(a));
}

TEST_F(TypeTests, PerKind_Function) {
    auto abc_call_fn = [](yama::context&) {};
    auto abc_callsiginf = yama::make_callsig({ 0, 1, 2 }, 1);
    auto abc_constsinf =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);
    yama::module_factory mf{};
    mf.add_function(
        "abc"_str,
        abc_constsinf,
        abc_callsiginf,
        17,
        abc_call_fn);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();

    std::optional<yama::callsig> expected_callsig = yama::callsig(abc_callsiginf, yama::const_table(a));
    std::optional<yama::call_fn> expected_call_fn = abc_call_fn;

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::function);
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), expected_callsig);
    EXPECT_EQ(a.call_fn(), expected_call_fn);
    EXPECT_EQ(a.max_locals(), 17);
    EXPECT_EQ(a.consts(), yama::const_table(a));
}

TEST_F(TypeTests, PerKind_Struct) {
    auto abc_constsinf =
        yama::const_table_info()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);
    yama::module_factory mf{};
    mf.add_struct(
        "abc"_str,
        abc_constsinf);

    parcel->push(""_str, std::move(mf.done()));

    yama::type a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::struct0);
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), std::nullopt);
    EXPECT_EQ(a.call_fn(), std::nullopt);
    EXPECT_EQ(a.max_locals(), 0);
    EXPECT_EQ(a.consts(), yama::const_table(a));
}

