

#include <gtest/gtest.h>

#include <yama/core/bcode.h>


using namespace yama::string_literals;


// TODO: copy/move ctor/assign have not been unit tested yet


TEST(BCodeTests, DefaultInit) {
    yama::bc::code a{};

    EXPECT_EQ(a.count(), 0);
}

TEST(BCodeTests, Count) {
    yama::bc::code a{};

    EXPECT_EQ(a.count(), 0);
    
    a.add_noop();

    EXPECT_EQ(a.count(), 1);
    
    a.add_noop();

    EXPECT_EQ(a.count(), 2);
    
    a.add_noop();

    EXPECT_EQ(a.count(), 3);
}

TEST(BCodeTests, Get) {
    yama::bc::code a{};

    a
        .add_noop()
        .add_ret(0)
        .add_copy(3, 4, true)
        .add_copy(0, 1);

    ASSERT_EQ(a.count(), 4);

    const auto get_0_a = a.get(0);
    const auto get_0_b = a[0];
    const auto get_1_a = a.get(1);
    const auto get_1_b = a[1];
    const auto get_2_a = a.get(2);
    const auto get_2_b = a[2];
    const auto get_3_a = a.get(3);
    const auto get_3_b = a[3];

    EXPECT_EQ(get_0_a.opc, yama::bc::opcode::noop);
    EXPECT_EQ(get_0_b.opc, yama::bc::opcode::noop);
    EXPECT_EQ(get_1_a.opc, yama::bc::opcode::ret);
    EXPECT_EQ(get_1_b.opc, yama::bc::opcode::ret);
    EXPECT_EQ(get_1_a.A, 0);
    EXPECT_EQ(get_1_b.A, 0);
    EXPECT_EQ(get_2_a.opc, yama::bc::opcode::copy);
    EXPECT_EQ(get_2_b.opc, yama::bc::opcode::copy);
    EXPECT_EQ(get_2_a.A, 3);
    EXPECT_EQ(get_2_b.A, 3);
    EXPECT_EQ(get_2_a.B, 4);
    EXPECT_EQ(get_2_b.B, 4);
    EXPECT_EQ(get_3_a.opc, yama::bc::opcode::copy);
    EXPECT_EQ(get_3_b.opc, yama::bc::opcode::copy);
    EXPECT_EQ(get_3_a.A, 0);
    EXPECT_EQ(get_3_b.A, 0);
    EXPECT_EQ(get_3_a.B, 1);
    EXPECT_EQ(get_3_b.B, 1);
}

TEST(BCodeTests, ReinitFlag) {
    yama::bc::code a{};

    a
        .add_noop()
        .add_ret(0)
        .add_copy(3, 4, true)
        .add_copy(0, 1);

    ASSERT_EQ(a.count(), 4);

    EXPECT_FALSE(a.reinit_flag(0));
    EXPECT_FALSE(a.reinit_flag(1));
    EXPECT_TRUE(a.reinit_flag(2));
    EXPECT_FALSE(a.reinit_flag(3));
}

TEST(BCodeTests, Construction) {
    yama::bc::code a{};

    static_assert(yama::bc::opcodes == 11);

    a
        .add_noop()
        .add_load_none(10)
        .add_load_none(10, true)
        .add_load_const(10, 11)
        .add_load_const(10, 11, true)
        .add_load_arg(10, 11)
        .add_load_arg(10, 11, true)
        .add_copy(10, 11)
        .add_copy(10, 11, true)
        .add_call(10, 11, 12)
        .add_call(10, 11, 12, true)
        .add_call_nr(10, 11)
        .add_ret(10)
        .add_jump(-6)
        .add_jump_true(10, -6)
        .add_jump_false(10, -6);

    ASSERT_EQ(a.count(), 16);

    std::cerr << a.fmt_disassembly() << "\n";

    size_t i = 0;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::noop);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_none);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_none);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_TRUE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_const);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_const);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_TRUE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_arg);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::load_arg);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_TRUE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::copy);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::copy);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_TRUE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::call);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_EQ(a[i].C, 12);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::call);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_EQ(a[i].C, 12);
    EXPECT_TRUE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::call_nr);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].B, 11);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::ret);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::jump);
    EXPECT_EQ(a[i].sBx, -6);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::jump_true);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].sBx, -6);
    EXPECT_FALSE(a.reinit_flag(i));

    i++;

    EXPECT_EQ(a[i].opc, yama::bc::opcode::jump_false);
    EXPECT_EQ(a[i].A, 10);
    EXPECT_EQ(a[i].sBx, -6);
    EXPECT_FALSE(a.reinit_flag(i));
}

TEST(BCodeTests, Syms_Empty) {
    yama::bc::syms a{};

    EXPECT_FALSE(a.fetch(0));
    EXPECT_FALSE(a.fetch(1));
    EXPECT_FALSE(a.fetch(2));
    EXPECT_FALSE(a.fetch(3));

    EXPECT_FALSE(a[0]);
    EXPECT_FALSE(a[1]);
    EXPECT_FALSE(a[2]);
    EXPECT_FALSE(a[3]);
}

TEST(BCodeTests, Syms_NonEmpty) {
    const auto a =
        yama::bc::syms()
        .add(1, "abc"_str, 10, 14)
        .add(3, "def"_str, 1, 1);

    EXPECT_FALSE(a.fetch(0));
    EXPECT_TRUE(a.fetch(1));
    EXPECT_FALSE(a.fetch(2));
    EXPECT_TRUE(a.fetch(3));

    EXPECT_FALSE(a[0]);
    EXPECT_TRUE(a[1]);
    EXPECT_FALSE(a[2]);
    EXPECT_TRUE(a[3]);

    const yama::bc::sym other_a{ .index = 1, .origin = "abc"_str, .ch = 10, .ln = 14 };
    const yama::bc::sym other_b{ .index = 3, .origin = "def"_str, .ch = 1, .ln = 1 };

    if (const auto v = a.fetch(1)) EXPECT_EQ(*v, other_a);
    if (const auto v = a.fetch(3)) EXPECT_EQ(*v, other_b);

    if (const auto v = a[1]) EXPECT_EQ(*v, other_a);
    if (const auto v = a[3]) EXPECT_EQ(*v, other_b);
}

TEST(BCodeTests, Syms_AddOverwritesExisting) {
    const auto a =
        yama::bc::syms()
        .add(0, "abc"_str, 10, 14)
        .add(0, "def"_str, 1, 1);

    ASSERT_TRUE(a[0]);

    const yama::bc::sym other{ .index = 0, .origin = "def"_str, .ch = 1, .ln = 1 };

    EXPECT_EQ(a[0].value(), other);
}

