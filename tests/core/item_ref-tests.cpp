

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig_ref.h>
#include <yama/core/const_table.h>
#include <yama/core/item_ref.h>
#include <yama/core/domain.h>

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


class ItemRefTests : public testing::Test {
public:
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<test_parcel> parcel;


    module_helper mh;

    void setup() {
        parcel->push(""_str, mh.result());
    }


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


// IMPORTANT: many of the basic query methods of yama::item_ref are tested *in bulk* in
//            our per-kind tests


TEST_F(ItemRefTests, CopyCtor) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);

    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();
    yama::item_ref b(a); // copy ctor

    ASSERT_EQ(a, b); // compare by ref
}

TEST_F(ItemRefTests, MoveCtor) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);

    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();
    yama::item_ref b(std::move(a)); // move ctor

    ASSERT_EQ(b, dm->load("a:abc"_str).value()); // compare by ref
}

TEST_F(ItemRefTests, CopyAssign) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    auto def_consts =
        yama::const_table()
        .add_bool(true); // <- make b consts differ from a consts

    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);
    mh.add_primitive("def"_str, def_consts, yama::ptype::bool0);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();
    yama::item_ref b = dm->load("a:def"_str).value();

    b = a; // copy assign

    ASSERT_EQ(a, b); // compare by ref
}

TEST_F(ItemRefTests, MoveAssign) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);
    auto def_consts =
        yama::const_table()
        .add_bool(true); // <- make b consts differ from a consts

    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);
    mh.add_primitive("def"_str, def_consts, yama::ptype::bool0);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();
    yama::item_ref b = dm->load("a:def"_str).value();

    b = std::move(a); // move assign

    ASSERT_EQ(b, dm->load("a:abc"_str).value()); // compare by ref
}

TEST_F(ItemRefTests, Equality) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);

    mh.add_primitive("abc"_str, yama::const_table(abc_consts), yama::ptype::float0);
    mh.add_primitive("def"_str, yama::const_table(abc_consts), yama::ptype::float0);

    setup();

    yama::item_ref a0 = dm->load("a:abc"_str).value();
    yama::item_ref a1 = dm->load("a:abc"_str).value();
    yama::item_ref b = dm->load("a:def"_str).value();

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

static_assert(yama::kinds == 4);

TEST_F(ItemRefTests, Primitive) {
    auto abc_consts =
        yama::const_table()
        .add_int(10'000);

    mh.add_primitive("abc"_str, abc_consts, yama::ptype::float0);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.consts(), yama::const_table_ref(a));
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.callsig(), std::nullopt);
    //EXPECT_EQ(a.call_fn(), std::nullopt);
    //EXPECT_EQ(a.max_locals(), 0);
}

TEST_F(ItemRefTests, Function) {
    auto abc_call_fn = [](yama::context&) {};
    auto abc_callsig = yama::make_callsig({ 0, 1, 2 }, 1);
    auto abc_consts =
        yama::const_table()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);

    mh.add_function(
        "abc"_str,
        abc_consts,
        abc_callsig,
        17,
        abc_call_fn);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();

    std::optional<yama::callsig_ref> expected_callsig = yama::callsig_ref(abc_callsig, yama::const_table_ref(a));
    std::optional<yama::call_fn> expected_call_fn = abc_call_fn;

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::function);
    EXPECT_EQ(a.consts(), yama::const_table_ref(a));
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), expected_callsig);
    //EXPECT_EQ(a.call_fn(), expected_call_fn);
    //EXPECT_EQ(a.max_locals(), 17);
}

TEST_F(ItemRefTests, Method) {
    auto abc_m_call_fn = [](yama::context&) {};
    auto abc_m_callsig = yama::make_callsig({ 0, 1, 2 }, 1);
    auto abc_m_consts =
        yama::const_table()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);

    mh.add_struct(
        "abc"_str,
        {});
    mh.add_method(
        "abc"_str, "m"_str,
        abc_m_consts,
        abc_m_callsig,
        17,
        abc_m_call_fn);

    setup();

    yama::item_ref abc = dm->load("a:abc"_str).value();
    yama::item_ref abc_m = dm->load("a:abc::m"_str).value();

    std::optional<yama::callsig_ref> expected_callsig = yama::callsig_ref(abc_m_callsig, yama::const_table_ref(abc_m));
    std::optional<yama::call_fn> expected_call_fn = abc_m_call_fn;

    EXPECT_EQ(abc_m.fullname(), "a:abc::m"_str);
    EXPECT_EQ(abc_m.kind(), yama::kind::method);
    EXPECT_EQ(abc_m.consts(), yama::const_table_ref(abc_m));
    EXPECT_EQ(abc_m.ptype(), std::nullopt);
    EXPECT_EQ(abc_m.callsig(), expected_callsig);
    //EXPECT_EQ(abc_m.call_fn(), expected_call_fn);
    //EXPECT_EQ(abc_m.max_locals(), 17);
}

TEST_F(ItemRefTests, Struct) {
    auto abc_consts =
        yama::const_table()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);

    mh.add_struct(
        "abc"_str,
        abc_consts);

    setup();

    yama::item_ref a = dm->load("a:abc"_str).value();

    EXPECT_EQ(a.fullname(), "a:abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::struct0);
    EXPECT_EQ(a.consts(), yama::const_table_ref(a));
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), std::nullopt);
    //EXPECT_EQ(a.call_fn(), std::nullopt);
    //EXPECT_EQ(a.max_locals(), 0);
}

