

#include <gtest/gtest.h>

#include <yama/core/module_info.h>


using namespace yama::string_literals;


TEST(ModuleFactoryTests, DefaultInit) {
    yama::module_factory f{};

    auto m = f.done();

    ASSERT_EQ(m.size(), 0);
}

TEST(ModuleFactoryTests, DoneResetsFactory) {
    yama::module_factory f{};

    // populate then reset f
    f.add_primitive("A"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive("B"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive("C"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive("D"_str, yama::const_table_info{}, yama::ptype::int0);
    (void)f.done();

    auto m = f.done();

    ASSERT_EQ(m.size(), 0);
}

TEST(ModuleFactoryTests, PopulatedModule) {
    yama::module_factory f{};

    static_assert(yama::kinds == 4);

    // primitive type

    yama::const_table_info A_consts{};
    A_consts
        .add_int(31);

    f.add_primitive("A"_str, A_consts, yama::ptype::int0);

    // function/method type (native fn)

    yama::const_table_info B_consts{};
    B_consts
        .add_int(31)
        .add_primitive_type("Int"_str);

    auto B_callsig = yama::make_callsig({ 1, 1 }, 1);

    size_t B_max_locals = 13;

    auto B_call_fn = [](yama::context&) {};

    f.add_function("B"_str, B_consts, B_callsig, B_max_locals, B_call_fn);
    f.add_method("Bm"_str, B_consts, B_callsig, B_max_locals, B_call_fn);

    // function/method type (bcode)

    yama::const_table_info C_consts{};
    C_consts
        .add_int(31)
        .add_primitive_type("Int"_str);

    auto C_callsig = yama::make_callsig({ 1, 1 }, 1);

    size_t C_max_locals = 10;

    yama::bc::code C_code =
        yama::bc::code_writer()
        .add_noop()
        .add_noop()
        .add_noop()
        .done()
        .value();

    yama::bc::syms C_syms{};
    C_syms
        .add(0, "origin"_str, 10, 13)
        .add(1, "origin"_str, 5, 14)
        .add(2, "origin"_str, 15, 15);

    f.add_function("C"_str, C_consts, C_callsig, C_max_locals, C_code, C_syms);
    f.add_method("Cm"_str, C_consts, C_callsig, C_max_locals, C_code, C_syms);

    // struct type

    yama::const_table_info D_consts{};
    D_consts
        .add_int(31);

    f.add_struct("D"_str, D_consts);

    auto m = f.done();

    ASSERT_EQ(m.size(), 6);

    ASSERT_TRUE(m.contains("A"_str));
    ASSERT_TRUE(m.contains("B"_str));
    ASSERT_TRUE(m.contains("Bm"_str));
    ASSERT_TRUE(m.contains("C"_str));
    ASSERT_TRUE(m.contains("Cm"_str));
    ASSERT_TRUE(m.contains("D"_str));

    EXPECT_EQ(m["A"_str], yama::make_primitive("A"_str, A_consts, yama::ptype::int0));
    EXPECT_EQ(m["B"_str], yama::make_function("B"_str, B_consts, B_callsig, B_max_locals, B_call_fn));
    EXPECT_EQ(m["Bm"_str], yama::make_method("Bm"_str, B_consts, B_callsig, B_max_locals, B_call_fn));
    EXPECT_EQ(m["C"_str], yama::make_function("C"_str, C_consts, C_callsig, C_max_locals, C_code, C_syms));
    EXPECT_EQ(m["Cm"_str], yama::make_method("Cm"_str, C_consts, C_callsig, C_max_locals, C_code, C_syms));
    EXPECT_EQ(m["D"_str], yama::make_struct("D"_str, D_consts));
}

