

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
    f.add_primitive_type("A"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive_type("B"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive_type("C"_str, yama::const_table_info{}, yama::ptype::int0);
    f.add_primitive_type("D"_str, yama::const_table_info{}, yama::ptype::int0);
    (void)f.done();

    auto m = f.done();

    ASSERT_EQ(m.size(), 0);
}

TEST(ModuleFactoryTests, PopulatedModule) {
    yama::module_factory f{};

    static_assert(yama::kinds == 2);

    // primitive type

    yama::const_table_info A_consts{};
    A_consts
        .add_int(31);

    f.add_primitive_type(
        "A"_str,
        decltype(A_consts)(A_consts),
        yama::ptype::int0);

    // function type (native fn)

    yama::const_table_info B_consts{};
    B_consts
        .add_int(31)
        .add_primitive_type("Int"_str);

    auto B_callsig = yama::make_callsig_info({ 1, 1 }, 1);

    size_t B_max_locals = 13;

    auto B_call_fn = [](yama::context&) {};

    f.add_function_type(
        "B"_str,
        decltype(B_consts)(B_consts),
        decltype(B_callsig)(B_callsig),
        B_max_locals,
        B_call_fn);

    // function type (bcode)

    yama::const_table_info C_consts{};
    C_consts
        .add_int(31)
        .add_primitive_type("Int"_str);

    auto C_callsig = yama::make_callsig_info({ 1, 1 }, 1);

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

    f.add_function_type(
        "C"_str,
        decltype(C_consts)(C_consts),
        decltype(C_callsig)(C_callsig),
        C_max_locals,
        decltype(C_code)(C_code),
        decltype(C_syms)(C_syms));

    auto m = f.done();

    ASSERT_EQ(m.size(), 3);

    ASSERT_TRUE(m.contains("A"_str));
    ASSERT_TRUE(m.contains("B"_str));
    ASSERT_TRUE(m.contains("C"_str));

    yama::type_info A_expected{
        .fullname = "A"_str,
        .consts = A_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::int0,
        },
    };
    yama::type_info B_expected{
        .fullname = "B"_str,
        .consts = B_consts,
        .info = yama::function_info{
            .callsig = B_callsig,
            .call_fn = B_call_fn,
            .max_locals = B_max_locals,
            .bcode = yama::bc::code{},
            .bcodesyms = yama::bc::syms{},
        },
    };
    yama::type_info C_expected{
        .fullname = "C"_str,
        .consts = C_consts,
        .info = yama::function_info{
            .callsig = C_callsig,
            .call_fn = yama::bcode_call_fn,
            .max_locals = C_max_locals,
            .bcode = C_code,
            .bcodesyms = C_syms,
        },
    };

    EXPECT_EQ(m["A"_str], A_expected);
    EXPECT_EQ(m["B"_str], B_expected);
    EXPECT_EQ(m["C"_str], C_expected);
}

