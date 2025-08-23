

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/parcel.h>
#include <yama/core/domain.h>
#include <yama/core/callsig_ref.h>
#include <yama/core/const_table.h>
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
        std::optional<yama::import_result> import(const yama::str & relative_path) override final {
            if (const auto it = mods.find(relative_path); it != mods.end()) {
                return yama::import_result(yama::res(it->second));
            }
            else return std::nullopt;
        }
    };
}


class CallSigRefTests : public testing::Test {
public:
    std::shared_ptr<yama::debug> dbg;
    std::shared_ptr<yama::domain> dm;
    std::shared_ptr<test_parcel> parcel;


    void push_module_with_f();
    void push_module_with_a1_a2_b_c_and_d();


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

void CallSigRefTests::push_module_with_f() {
    // fn(Int, Float, Char) -> Float
    auto f_consts =
        yama::const_table()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str);
    auto f_callsig = yama::make_callsig({ 0, 1, 2 }, 1);

    module_helper mh{};
    mh.add_function("f"_str, f_consts, f_callsig, 10, yama::noop_call_fn);

    parcel->push(""_str, mh.result());
}

void CallSigRefTests::push_module_with_a1_a2_b_c_and_d() {
    // a1 and a2 will be structurally the same, but will refer to two distinct callsigs
    // (w/ this being to test compare by value)

    // b will differ by having different number of params
    // c will differ by having different param types
    // d will differ by having different return types

    // a1, a2       : fn(Int, Float, Char) -> Float
    // b            : fn(Int, Float, Char, UInt) -> Float
    // c            : fn(Int, UInt, Char) -> Float
    // d            : fn(Int, Float, Char) -> UInt

    auto consts =
        yama::const_table()
        .add_primitive_type("yama:Int"_str)
        .add_primitive_type("yama:Float"_str)
        .add_primitive_type("yama:Char"_str)
        .add_primitive_type("yama:UInt"_str);

    auto a_callsig = yama::make_callsig({ 0, 1, 2 }, 1);
    auto b_callsig = yama::make_callsig({ 0, 1, 2, 3 }, 1);
    auto c_callsig = yama::make_callsig({ 0, 3, 2 }, 1);
    auto d_callsig = yama::make_callsig({ 0, 1, 2 }, 3);

    module_helper mh{};
    mh.add_function("a1"_str, consts, a_callsig, 10, yama::noop_call_fn);
    mh.add_function("a2"_str, consts, a_callsig, 10, yama::noop_call_fn);
    mh.add_function("b"_str, consts, b_callsig, 10, yama::noop_call_fn);
    mh.add_function("c"_str, consts, c_callsig, 10, yama::noop_call_fn);
    mh.add_function("d"_str, consts, d_callsig, 10, yama::noop_call_fn);

    parcel->push(""_str, mh.result());
}


TEST_F(CallSigRefTests, Params) {
    push_module_with_f();
    auto f = dm->load("a:f"_str).value().callsig();

    ASSERT_TRUE(f);
    ASSERT_EQ(f->params(), 3);
}

TEST_F(CallSigRefTests, ParamType) {
    push_module_with_f();
    auto f = dm->load("a:f"_str).value().callsig();

    ASSERT_TRUE(f);
    ASSERT_EQ(f->params(), 3);

    EXPECT_EQ(f->param_type(0), std::make_optional(dm->int_type()));
    EXPECT_EQ(f->param_type(1), std::make_optional(dm->float_type()));
    EXPECT_EQ(f->param_type(2), std::make_optional(dm->char_type()));
}

TEST_F(CallSigRefTests, ParamType_IndexOutOfBounds) {
    push_module_with_f();
    auto f = dm->load("a:f"_str).value().callsig();

    ASSERT_TRUE(f);
    ASSERT_EQ(f->params(), 3);

    EXPECT_EQ(f->param_type(4), std::nullopt);
}

TEST_F(CallSigRefTests, ParamType_RefsStubConst) {
    // TODO: stub
}

TEST_F(CallSigRefTests, ReturnType) {
    push_module_with_f();
    auto f = dm->load("a:f"_str).value().callsig();

    ASSERT_TRUE(f);
    ASSERT_EQ(f->return_type(), std::make_optional(dm->float_type()));
}

TEST_F(CallSigRefTests, ReturnType_RefsStubConst) {
    // TODO: stub
}

TEST_F(CallSigRefTests, Equality) {
    push_module_with_a1_a2_b_c_and_d();
    auto a1 = dm->load("a:a1"_str).value().callsig();
    auto a2 = dm->load("a:a2"_str).value().callsig();
    auto b = dm->load("a:b"_str).value().callsig();
    auto c = dm->load("a:c"_str).value().callsig();
    auto d = dm->load("a:d"_str).value().callsig();

    ASSERT_TRUE(a1);
    ASSERT_TRUE(a2);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);

    EXPECT_EQ(a1, a1);
    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, b);
    EXPECT_NE(a1, c);
    EXPECT_NE(a1, d);

    EXPECT_EQ(a2, a1);
    EXPECT_EQ(a2, a2);
    EXPECT_NE(a2, b);
    EXPECT_NE(a2, c);
    EXPECT_NE(a2, d);

    EXPECT_NE(b, a1);
    EXPECT_NE(b, a2);
    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);
    EXPECT_NE(b, d);

    EXPECT_NE(c, a1);
    EXPECT_NE(c, a2);
    EXPECT_NE(c, b);
    EXPECT_EQ(c, c);
    EXPECT_NE(c, d);

    EXPECT_NE(d, a1);
    EXPECT_NE(d, a2);
    EXPECT_NE(d, b);
    EXPECT_NE(d, c);
    EXPECT_EQ(d, d);
}

TEST_F(CallSigRefTests, Fmt) {
    push_module_with_f();
    auto f = dm->load("a:f"_str).value().callsig();

    ASSERT_TRUE(f);

    std::string expected = "fn(yama:Int, yama:Float, yama:Char) -> yama:Float";
    std::string actual = f->fmt();

    yama::println("-- expected = {}", expected);
    yama::println("-- actual   = {}", actual);

    EXPECT_EQ(expected, actual);
}

