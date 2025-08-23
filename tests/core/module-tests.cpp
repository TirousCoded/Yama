

#include <gtest/gtest.h>

#include <unordered_set>

#include <yama/core/module.h>


using namespace yama::string_literals;


TEST(ModuleTests, Item_AllOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    EXPECT_TRUE(item.all_of<>());
    EXPECT_TRUE(item.all_of<yama::item_desc>());
    EXPECT_FALSE(item.all_of<yama::member_desc>());
    EXPECT_TRUE(item.all_of<yama::prim_desc>());
    const auto v0 = item.all_of<yama::item_desc, yama::prim_desc>();
    EXPECT_TRUE(v0);
    const auto v1 = item.all_of<yama::item_desc, yama::call_desc>();
    EXPECT_FALSE(v1);
    const auto v2 = item.all_of<yama::member_desc, yama::call_desc>();
    EXPECT_FALSE(v2);
}

TEST(ModuleTests, Item_AnyOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    EXPECT_FALSE(item.any_of<>());
    EXPECT_TRUE(item.any_of<yama::item_desc>());
    EXPECT_FALSE(item.any_of<yama::member_desc>());
    EXPECT_TRUE(item.any_of<yama::prim_desc>());
    const auto v0 = item.any_of<yama::item_desc, yama::prim_desc>();
    EXPECT_TRUE(v0);
    const auto v1 = item.any_of<yama::item_desc, yama::call_desc>();
    EXPECT_TRUE(v1);
    const auto v2 = item.any_of<yama::member_desc, yama::call_desc>();
    EXPECT_FALSE(v2);
}

TEST(ModuleTests, Item_NoneOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    EXPECT_TRUE(item.none_of<>());
    EXPECT_FALSE(item.none_of<yama::item_desc>());
    EXPECT_TRUE(item.none_of<yama::member_desc>());
    EXPECT_FALSE(item.none_of<yama::prim_desc>());
    const auto v0 = item.none_of<yama::item_desc, yama::prim_desc>();
    EXPECT_FALSE(v0);
    const auto v1 = item.none_of<yama::item_desc, yama::call_desc>();
    EXPECT_FALSE(v1);
    const auto v2 = item.none_of<yama::member_desc, yama::call_desc>();
    EXPECT_TRUE(v2);
}

TEST(ModuleTests, Item_TryGet) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    const auto [item_, prim] = m.get<yama::item_desc, yama::prim_desc>("A"_str);

    EXPECT_NO_THROW(std::tuple<> dummy = item.try_get<>());
    const auto [item0] = item.try_get<yama::item_desc>();
    const auto [bcode1] = item.try_get<yama::bcode_desc>();
    const auto [item2, prim2, bcode2] = item.try_get<yama::item_desc, yama::prim_desc, yama::bcode_desc>();

    EXPECT_EQ(item0, &item_);

    EXPECT_EQ(bcode1, nullptr);

    EXPECT_EQ(item2, &item_);
    EXPECT_EQ(prim2, &prim);
    EXPECT_EQ(bcode2, nullptr);
}

TEST(ModuleTests, Item_Get) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    const auto [item_, prim] = m.get<yama::item_desc, yama::prim_desc>("A"_str);

    EXPECT_NO_THROW(std::tuple<> dummy = item.get<>());
    const auto [item0] = item.get<yama::item_desc>();
    const auto [item1, prim1] = item.get<yama::item_desc, yama::prim_desc>();

    EXPECT_EQ(&item0, &item_);

    EXPECT_EQ(&item1, &item_);
    EXPECT_EQ(&prim1, &prim);
}

TEST(ModuleTests, Item_ID) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    EXPECT_EQ(item.id(), m.id("A"_str).value());
}

TEST(ModuleTests, Item_Name) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item item = m.get_item<>("A"_str).value();

    EXPECT_EQ(item.name(), "A"_str);
}

TEST(ModuleTests, Item_Equality) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_primitive("B"_str, yama::const_table{}, yama::ptype::int0));

    const yama::module::item a1 = m.get_item<>("A"_str).value();
    const yama::module::item a2 = m.get_item<>("A"_str).value();
    const yama::module::item b = m.get_item<>("B"_str).value();

    EXPECT_EQ(a1, a1);
    EXPECT_EQ(a1, a2);
    EXPECT_NE(a1, b);

    EXPECT_EQ(a2, a2);
    EXPECT_NE(a2, b);

    EXPECT_EQ(b, b);
}

TEST(ModuleTests, EmptyModule) {
    yama::module m{};

    EXPECT_EQ(m.count(), 0);
}

TEST(ModuleTests, PopulatedModule) {
    static_assert(yama::kinds == 4);
    yama::module m{};

    yama::const_table A_consts, B_consts, A_C_consts, D_consts;
    A_consts.add_int(1);
    B_consts.add_int(2);
    A_C_consts.add_int(3);
    D_consts.add_int(4);

    auto B_callsig = yama::make_callsig({}, 1);
    auto A_C_callsig = yama::make_callsig({}, 2);
    
    size_t B_max_locals = 10;
    size_t A_C_max_locals = 11;

    auto B_call_fn = [](yama::context&) {};
    auto A_C_call_fn = [](yama::context&) {};

    EXPECT_TRUE(m.add_primitive("A"_str, A_consts, yama::ptype::int0));
    EXPECT_TRUE(m.add_function("B"_str, B_consts, B_callsig, B_max_locals, B_call_fn));
    EXPECT_TRUE(m.add_method("A"_str, "C"_str, A_C_consts, A_C_callsig, A_C_max_locals, A_C_call_fn));
    EXPECT_TRUE(m.add_struct("D"_str, D_consts));

    EXPECT_EQ(m.count(), yama::kinds);
    if (m.all_of<yama::item_desc, yama::owner_desc, yama::prim_desc>("A"_str)) {
        auto [item, owner, prim] = m.get<yama::item_desc, yama::owner_desc, yama::prim_desc>("A"_str);
        EXPECT_EQ(item.name, "A"_str);
        EXPECT_EQ(item.kind, yama::kind::primitive);
        EXPECT_EQ(item.consts, A_consts);

        EXPECT_EQ(owner.members.size(), 1);
        EXPECT_TRUE(owner.is_member(m.id("A::C"_str).value()));

        EXPECT_EQ(prim.ptype, yama::ptype::int0);
    }
    else ADD_FAILURE();
    if (m.all_of<yama::item_desc, yama::call_desc>("B"_str)) {
        auto [item, call] = m.get<yama::item_desc, yama::call_desc>("B"_str);
        EXPECT_EQ(item.name, "B"_str);
        EXPECT_EQ(item.kind, yama::kind::function);
        EXPECT_EQ(item.consts, B_consts);

        EXPECT_EQ(call.callsig, B_callsig);
        EXPECT_EQ(call.max_locals, B_max_locals);
        EXPECT_EQ(call.call_fn, B_call_fn);
    }
    else ADD_FAILURE();
    if (m.all_of<yama::item_desc, yama::member_desc, yama::call_desc>("A::C"_str)) {
        auto [item, memb, call] = m.get<yama::item_desc, yama::member_desc, yama::call_desc>("A::C"_str);
        EXPECT_EQ(item.name, "A::C"_str);
        EXPECT_EQ(item.kind, yama::kind::method);
        EXPECT_EQ(item.consts, A_C_consts);

        EXPECT_EQ(memb.owner, m.id("A"_str).value());

        EXPECT_EQ(call.callsig, A_C_callsig);
        EXPECT_EQ(call.max_locals, A_C_max_locals);
        EXPECT_EQ(call.call_fn, A_C_call_fn);
    }
    else ADD_FAILURE();
    if (m.all_of<yama::item_desc>("D"_str)) {
        auto [item] = m.get<yama::item_desc>("D"_str);
        EXPECT_EQ(item.name, "D"_str);
        EXPECT_EQ(item.kind, yama::kind::struct0);
        EXPECT_EQ(item.consts, D_consts);
    }
    else ADD_FAILURE();
}

TEST(ModuleTests, AddingMemberTypeBeforeItsOwnerType) {
    static_assert(yama::kinds == 4);
    yama::module m{};

    yama::const_table A_m_consts;
    A_m_consts.add_int(1);

    auto A_m_callsig = yama::make_callsig({}, 1);

    size_t A_m_max_locals = 10;

    auto A_m_call_fn = [](yama::context&) {};

    m.add_method("A"_str, "m"_str, A_m_consts, A_m_callsig, A_m_max_locals, A_m_call_fn);

    EXPECT_EQ(m.count(), 1);
    {
        EXPECT_TRUE(m.none_of<yama::member_desc>("A::m"_str));

        auto [item, call] = m.get<yama::item_desc, yama::call_desc>("A::m"_str);
        EXPECT_EQ(item.name, "A::m"_str);
        EXPECT_EQ(item.kind, yama::kind::method);
        EXPECT_EQ(item.consts, A_m_consts);

        EXPECT_EQ(call.callsig, A_m_callsig);
        EXPECT_EQ(call.max_locals, A_m_max_locals);
        EXPECT_EQ(call.call_fn, A_m_call_fn);
    }
}

TEST(ModuleTests, Fail_NameAlreadyUsed) {
    static_assert(yama::kinds == 4);
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    EXPECT_FALSE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_FALSE(m.add_function("A"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_FALSE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    EXPECT_FALSE(m.add_struct("A"_str, yama::const_table{}));

    EXPECT_EQ(m.count(), 2);
}

TEST(ModuleTests, Count) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "a"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "b"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "c"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_primitive("B"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_primitive("C"_str, yama::const_table{}, yama::ptype::int0));

    EXPECT_EQ(m.count(), 6);
}

TEST(ModuleTests, CountWith) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "a"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "b"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "c"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_struct("B"_str, yama::const_table{}));
    ASSERT_TRUE(m.add_primitive("C"_str, yama::const_table{}, yama::ptype::int0));

    EXPECT_EQ(m.count_with<yama::item_desc>(), 6);
    EXPECT_EQ(m.count_with<yama::owner_desc>(), 1);
    EXPECT_EQ(m.count_with<yama::member_desc>(), 3);
    EXPECT_EQ(m.count_with<yama::prim_desc>(), 2);
    EXPECT_EQ(m.count_with<yama::call_desc>(), 3);
    EXPECT_EQ(m.count_with<yama::bcode_desc>(), 0);
    {
        size_t s = m.count_with<yama::item_desc, yama::owner_desc>();
        EXPECT_EQ(s, 1);
    }
    {
        size_t s = m.count_with<yama::call_desc, yama::member_desc>();
        EXPECT_EQ(s, 3);
    }
    {
        size_t s = m.count_with<yama::prim_desc, yama::member_desc, yama::item_desc>();
        EXPECT_EQ(s, 0);
    }
}

TEST(ModuleTests, Exists) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    EXPECT_TRUE(m.exists("A"_str));
    EXPECT_TRUE(m.exists("A::m"_str));
    EXPECT_TRUE(m.exists(m.id("A"_str).value()));
    EXPECT_TRUE(m.exists(m.id("A::m"_str).value()));

    EXPECT_FALSE(m.exists("missing"_str));
    EXPECT_FALSE(m.exists(10'000)); // some invalid ID
}

TEST(ModuleTests, AllOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    {
        EXPECT_TRUE(m.all_of<>("A"_str));
        EXPECT_TRUE(m.all_of<yama::item_desc>("A"_str));
        EXPECT_FALSE(m.all_of<yama::member_desc>("A"_str));
        EXPECT_TRUE(m.all_of<yama::prim_desc>("A"_str));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>("A"_str);
        EXPECT_TRUE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>("A"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>("A"_str);
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_TRUE(m.all_of<>(m.id("A"_str).value()));
        EXPECT_TRUE(m.all_of<yama::item_desc>(m.id("A"_str).value()));
        EXPECT_FALSE(m.all_of<yama::member_desc>(m.id("A"_str).value()));
        EXPECT_TRUE(m.all_of<yama::prim_desc>(m.id("A"_str).value()));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>(m.id("A"_str).value());
        EXPECT_TRUE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_FALSE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_TRUE(m.all_of<>("A::m"_str));
        EXPECT_TRUE(m.all_of<yama::item_desc>("A::m"_str));
        EXPECT_TRUE(m.all_of<yama::member_desc>("A::m"_str));
        EXPECT_FALSE(m.all_of<yama::prim_desc>("A::m"_str));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>("A::m"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>("A::m"_str);
        EXPECT_TRUE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>("A::m"_str);
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_TRUE(m.all_of<>(m.id("A::m"_str).value()));
        EXPECT_TRUE(m.all_of<yama::item_desc>(m.id("A::m"_str).value()));
        EXPECT_TRUE(m.all_of<yama::member_desc>(m.id("A::m"_str).value()));
        EXPECT_FALSE(m.all_of<yama::prim_desc>(m.id("A::m"_str).value()));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>(m.id("A::m"_str).value());
        EXPECT_FALSE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_TRUE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_FALSE(m.all_of<>("missing"_str));
        EXPECT_FALSE(m.all_of<yama::item_desc>("missing"_str));
        EXPECT_FALSE(m.all_of<yama::member_desc>("missing"_str));
        EXPECT_FALSE(m.all_of<yama::prim_desc>("missing"_str));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>("missing"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v2);
    }
    {
        yama::lid_t invalid_id = 10'000;
        ASSERT_FALSE(m.exists(invalid_id));

        EXPECT_FALSE(m.all_of<>(invalid_id));
        EXPECT_FALSE(m.all_of<yama::item_desc>(invalid_id));
        EXPECT_FALSE(m.all_of<yama::member_desc>(invalid_id));
        EXPECT_FALSE(m.all_of<yama::prim_desc>(invalid_id));
        const auto v0 = m.all_of<yama::item_desc, yama::prim_desc>(invalid_id);
        EXPECT_FALSE(v0);
        const auto v1 = m.all_of<yama::item_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v1);
        const auto v2 = m.all_of<yama::member_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v2);
    }
}

TEST(ModuleTests, AnyOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    {
        EXPECT_FALSE(m.any_of<>("A"_str));
        EXPECT_TRUE(m.any_of<yama::item_desc>("A"_str));
        EXPECT_FALSE(m.any_of<yama::member_desc>("A"_str));
        EXPECT_TRUE(m.any_of<yama::prim_desc>("A"_str));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>("A"_str);
        EXPECT_TRUE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>("A"_str);
        EXPECT_TRUE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>("A"_str);
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_FALSE(m.any_of<>(m.id("A"_str).value()));
        EXPECT_TRUE(m.any_of<yama::item_desc>(m.id("A"_str).value()));
        EXPECT_FALSE(m.any_of<yama::member_desc>(m.id("A"_str).value()));
        EXPECT_TRUE(m.any_of<yama::prim_desc>(m.id("A"_str).value()));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>(m.id("A"_str).value());
        EXPECT_TRUE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_TRUE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_FALSE(m.any_of<>("A::m"_str));
        EXPECT_TRUE(m.any_of<yama::item_desc>("A::m"_str));
        EXPECT_TRUE(m.any_of<yama::member_desc>("A::m"_str));
        EXPECT_FALSE(m.any_of<yama::prim_desc>("A::m"_str));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>("A::m"_str);
        EXPECT_TRUE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>("A::m"_str);
        EXPECT_TRUE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>("A::m"_str);
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_FALSE(m.any_of<>(m.id("A::m"_str).value()));
        EXPECT_TRUE(m.any_of<yama::item_desc>(m.id("A::m"_str).value()));
        EXPECT_TRUE(m.any_of<yama::member_desc>(m.id("A::m"_str).value()));
        EXPECT_FALSE(m.any_of<yama::prim_desc>(m.id("A::m"_str).value()));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>(m.id("A::m"_str).value());
        EXPECT_TRUE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_TRUE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_FALSE(m.any_of<>("missing"_str));
        EXPECT_FALSE(m.any_of<yama::item_desc>("missing"_str));
        EXPECT_FALSE(m.any_of<yama::member_desc>("missing"_str));
        EXPECT_FALSE(m.any_of<yama::prim_desc>("missing"_str));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>("missing"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v2);
    }
    {
        yama::lid_t invalid_id = 10'000;
        ASSERT_FALSE(m.exists(invalid_id));

        EXPECT_FALSE(m.any_of<>(invalid_id));
        EXPECT_FALSE(m.any_of<yama::item_desc>(invalid_id));
        EXPECT_FALSE(m.any_of<yama::member_desc>(invalid_id));
        EXPECT_FALSE(m.any_of<yama::prim_desc>(invalid_id));
        const auto v0 = m.any_of<yama::item_desc, yama::prim_desc>(invalid_id);
        EXPECT_FALSE(v0);
        const auto v1 = m.any_of<yama::item_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v1);
        const auto v2 = m.any_of<yama::member_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v2);
    }
}

TEST(ModuleTests, NoneOf) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "m"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    {
        EXPECT_TRUE(m.none_of<>("A"_str));
        EXPECT_FALSE(m.none_of<yama::item_desc>("A"_str));
        EXPECT_TRUE(m.none_of<yama::member_desc>("A"_str));
        EXPECT_FALSE(m.none_of<yama::prim_desc>("A"_str));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>("A"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>("A"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>("A"_str);
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_TRUE(m.none_of<>(m.id("A"_str).value()));
        EXPECT_FALSE(m.none_of<yama::item_desc>(m.id("A"_str).value()));
        EXPECT_TRUE(m.none_of<yama::member_desc>(m.id("A"_str).value()));
        EXPECT_FALSE(m.none_of<yama::prim_desc>(m.id("A"_str).value()));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>(m.id("A"_str).value());
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>(m.id("A"_str).value());
        EXPECT_TRUE(v2);
    }
    {
        EXPECT_TRUE(m.none_of<>("A::m"_str));
        EXPECT_FALSE(m.none_of<yama::item_desc>("A::m"_str));
        EXPECT_FALSE(m.none_of<yama::member_desc>("A::m"_str));
        EXPECT_TRUE(m.none_of<yama::prim_desc>("A::m"_str));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>("A::m"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>("A::m"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>("A::m"_str);
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_TRUE(m.none_of<>(m.id("A::m"_str).value()));
        EXPECT_FALSE(m.none_of<yama::item_desc>(m.id("A::m"_str).value()));
        EXPECT_FALSE(m.none_of<yama::member_desc>(m.id("A::m"_str).value()));
        EXPECT_TRUE(m.none_of<yama::prim_desc>(m.id("A::m"_str).value()));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>(m.id("A::m"_str).value());
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>(m.id("A::m"_str).value());
        EXPECT_FALSE(v2);
    }
    {
        EXPECT_FALSE(m.none_of<>("missing"_str));
        EXPECT_FALSE(m.none_of<yama::item_desc>("missing"_str));
        EXPECT_FALSE(m.none_of<yama::member_desc>("missing"_str));
        EXPECT_FALSE(m.none_of<yama::prim_desc>("missing"_str));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>("missing"_str);
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>("missing"_str);
        EXPECT_FALSE(v2);
    }
    {
        yama::lid_t invalid_id = 10'000;
        ASSERT_FALSE(m.exists(invalid_id));

        EXPECT_FALSE(m.none_of<>(invalid_id));
        EXPECT_FALSE(m.none_of<yama::item_desc>(invalid_id));
        EXPECT_FALSE(m.none_of<yama::member_desc>(invalid_id));
        EXPECT_FALSE(m.none_of<yama::prim_desc>(invalid_id));
        const auto v0 = m.none_of<yama::item_desc, yama::prim_desc>(invalid_id);
        EXPECT_FALSE(v0);
        const auto v1 = m.none_of<yama::item_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v1);
        const auto v2 = m.none_of<yama::member_desc, yama::call_desc>(invalid_id);
        EXPECT_FALSE(v2);
    }
}

TEST(ModuleTests, TryGet) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const auto id = m.id("A"_str).value();
    const auto [item, prim] = m.get<yama::item_desc, yama::prim_desc>("A"_str);

    {
        EXPECT_NO_THROW(std::tuple<> dummy = m.try_get<>("A"_str));
        const auto [item0] = m.try_get<yama::item_desc>("A"_str);
        const auto [bcode1] = m.try_get<yama::bcode_desc>("A"_str);
        const auto [item2, prim2, bcode2] = m.try_get<yama::item_desc, yama::prim_desc, yama::bcode_desc>("A"_str);

        EXPECT_EQ(item0, &item);
        
        EXPECT_EQ(bcode1, nullptr);

        EXPECT_EQ(item2, &item);
        EXPECT_EQ(prim2, &prim);
        EXPECT_EQ(bcode2, nullptr);
    }
    {
        EXPECT_NO_THROW(std::tuple<> dummy = m.try_get<>(id));
        const auto [item0] = m.try_get<yama::item_desc>(id);
        const auto [bcode1] = m.try_get<yama::bcode_desc>(id);
        const auto [item2, prim2, bcode2] = m.try_get<yama::item_desc, yama::prim_desc, yama::bcode_desc>(id);

        EXPECT_EQ(item0, &item);

        EXPECT_EQ(bcode1, nullptr);

        EXPECT_EQ(item2, &item);
        EXPECT_EQ(prim2, &prim);
        EXPECT_EQ(bcode2, nullptr);
    }
    {
        EXPECT_NO_THROW(std::tuple<> dummy = m.try_get<>("missing"_str));
        const auto [item0] = m.try_get<yama::item_desc>("missing"_str);
        const auto [bcode1] = m.try_get<yama::bcode_desc>("missing"_str);
        const auto [item2, prim2, bcode2] = m.try_get<yama::item_desc, yama::prim_desc, yama::bcode_desc>("missing"_str);

        EXPECT_EQ(item0, nullptr);

        EXPECT_EQ(bcode1, nullptr);

        EXPECT_EQ(item2, nullptr);
        EXPECT_EQ(prim2, nullptr);
        EXPECT_EQ(bcode2, nullptr);
    }
    {
        yama::lid_t invalid_id = 10'000;
        ASSERT_FALSE(m.exists(invalid_id));

        EXPECT_NO_THROW(std::tuple<> dummy = m.try_get<>(invalid_id));
        const auto [item0] = m.try_get<yama::item_desc>(invalid_id);
        const auto [bcode1] = m.try_get<yama::bcode_desc>(invalid_id);
        const auto [item2, prim2, bcode2] = m.try_get<yama::item_desc, yama::prim_desc, yama::bcode_desc>(invalid_id);

        EXPECT_EQ(item0, nullptr);

        EXPECT_EQ(bcode1, nullptr);

        EXPECT_EQ(item2, nullptr);
        EXPECT_EQ(prim2, nullptr);
        EXPECT_EQ(bcode2, nullptr);
    }
}

TEST(ModuleTests, Get) {
    yama::module m{};

    yama::const_table A_consts{};
    auto A_ptype = yama::ptype::int0;

    ASSERT_TRUE(m.add_primitive("A"_str, A_consts, A_ptype));

    const auto id = m.id("A"_str).value();

    {
        EXPECT_NO_THROW(std::tuple<> dummy = m.get<>("A"_str));
        const auto [item, prim] = m.get<yama::item_desc, yama::prim_desc>("A"_str);
        EXPECT_EQ(item.name, "A"_str);
        EXPECT_EQ(item.kind, yama::kind::primitive);
        EXPECT_EQ(item.consts, A_consts);

        EXPECT_EQ(prim.ptype, A_ptype);
    }
    {
        EXPECT_NO_THROW(std::tuple<> dummy = m.get<>(id));
        const auto [item, prim] = m.get<yama::item_desc, yama::prim_desc>(id);
        EXPECT_EQ(item.name, "A"_str);
        EXPECT_EQ(item.kind, yama::kind::primitive);
        EXPECT_EQ(item.consts, A_consts);

        EXPECT_EQ(prim.ptype, A_ptype);
    }
    {
        yama::lid_t invalid_id = 10'000;
        ASSERT_FALSE(m.exists(invalid_id));

        EXPECT_THROW(m.get<>("missing"_str), std::invalid_argument);
        EXPECT_THROW(m.get<yama::item_desc>("missing"_str), std::invalid_argument);

        EXPECT_THROW(m.get<>(invalid_id), std::invalid_argument);
        EXPECT_THROW(m.get<yama::item_desc>(invalid_id), std::invalid_argument);
    }
    {
        EXPECT_THROW(m.get<yama::member_desc>("A"_str), std::out_of_range);
        EXPECT_THROW(m.get<yama::member_desc>(id), std::out_of_range);
    }
}

TEST(ModuleTests, GetItem) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_method("A"_str, "a"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "b"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_method("A"_str, "c"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_primitive("B"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_primitive("C"_str, yama::const_table{}, yama::ptype::int0));

    const auto A_id = m.id("A"_str).value();
    const auto A_b_id = m.id("A::b"_str).value();

    {
        auto A_0 = m.get_item<>("A"_str);
        auto A_1 = m.get_item<yama::item_desc, yama::prim_desc>("A"_str);
        auto A_2 = m.get_item<yama::prim_desc, yama::bcode_desc>("A"_str);
        ASSERT_TRUE(A_0);
        ASSERT_TRUE(A_1);
        ASSERT_TRUE(A_2);

        EXPECT_EQ(A_0->id(), A_id);
        EXPECT_EQ(A_1->id(), A_id);
        EXPECT_EQ(A_2->id(), A_id);
    }
    {
        auto A_0 = m.get_item<>(A_id);
        auto A_1 = m.get_item<yama::item_desc, yama::prim_desc>(A_id);
        auto A_2 = m.get_item<yama::prim_desc, yama::bcode_desc>(A_id);
        ASSERT_TRUE(A_0);
        ASSERT_TRUE(A_1);
        ASSERT_TRUE(A_2);

        EXPECT_EQ(A_0->id(), A_id);
        EXPECT_EQ(A_1->id(), A_id);
        EXPECT_EQ(A_2->id(), A_id);
    }
    {
        auto A_b_0 = m.get_item<>("A::b"_str);
        auto A_b_1 = m.get_item<yama::item_desc, yama::prim_desc>("A::b"_str);
        auto A_b_2 = m.get_item<yama::prim_desc, yama::bcode_desc>("A::b"_str);
        ASSERT_TRUE(A_b_0);
        ASSERT_TRUE(A_b_1);
        ASSERT_TRUE(A_b_2);

        EXPECT_EQ(A_b_0->id(), A_b_id);
        EXPECT_EQ(A_b_1->id(), A_b_id);
        EXPECT_EQ(A_b_2->id(), A_b_id);
    }
    {
        auto A_b_0 = m.get_item<>(A_b_id);
        auto A_b_1 = m.get_item<yama::item_desc, yama::prim_desc>(A_b_id);
        auto A_b_2 = m.get_item<yama::prim_desc, yama::bcode_desc>(A_b_id);
        ASSERT_TRUE(A_b_0);
        ASSERT_TRUE(A_b_1);
        ASSERT_TRUE(A_b_2);

        EXPECT_EQ(A_b_0->id(), A_b_id);
        EXPECT_EQ(A_b_1->id(), A_b_id);
        EXPECT_EQ(A_b_2->id(), A_b_id);
    }
    {
        EXPECT_FALSE(m.get_item<>("missing"_str));
        auto v = m.get_item<yama::item_desc, yama::prim_desc>("missing"_str);
        EXPECT_FALSE(v);
    }
    {
        yama::lid_t invalid_id = 10'000;
        EXPECT_FALSE(m.get_item<>(invalid_id));
        auto v = m.get_item<yama::item_desc, yama::prim_desc>(invalid_id);
        EXPECT_FALSE(v);
    }
}

TEST(ModuleTests, View) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_struct("B"_str, yama::const_table{}));
    ASSERT_TRUE(m.add_primitive("C"_str, yama::const_table{}, yama::ptype::float0));
    ASSERT_TRUE(m.add_primitive("D"_str, yama::const_table{}, yama::ptype::char0));

    struct Result final {
        std::unordered_set<yama::lid_t> ids; // Need this for IDs iterated over, but w/ no descriptors.
        std::unordered_map<yama::lid_t, const yama::item_desc*> items;
        std::unordered_map<yama::lid_t, const yama::prim_desc*> prims;
        bool operator==(const Result&) const noexcept = default;
        void add(yama::lid_t id) { ids.insert(id); }
        void add(yama::lid_t id, const yama::item_desc& x) { add(id); items[id] = &x; }
        void add(yama::lid_t id, const yama::prim_desc& x) { add(id); prims[id] = &x; }
        void add(yama::lid_t id, const yama::item_desc& a, const yama::prim_desc& b) { add(id, a); add(id, b); }
        std::string fmt() const {
            std::string result{};
            result += "Result:\n";
            result += std::format("    ids ({}):\n", ids.size());
            for (const auto& id : ids) {
                result += std::format("        {}\n", id);
            }
            result += std::format("    items ({}):\n", items.size());
            for (const auto& [id, item_ptr] : items) {
                result += std::format("        {} => {}\n", id, (void*)item_ptr);
            }
            result += std::format("    prims ({}):\n", prims.size());
            for (const auto& [id, prim_ptr] : prims) {
                result += std::format("        {} => {}\n", id, (void*)prim_ptr);
            }
            return result;
        }
    };

    auto expected_result = [&m](bool include_item, bool include_prim, std::vector<yama::str> names) -> Result {
        Result result{};
        for (const auto& name : names) {
            const auto id = m.id(name).value();
            result.add(id);
            const auto [item, prim] = m.try_get<yama::item_desc, yama::prim_desc>(id);
            if (item && include_item) result.add(id, *item);
            if (prim && include_prim) result.add(id, *prim);
        }
        return result;
    };

    {
        Result expected = expected_result(false, false, { "A"_str, "B"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto [id] : m.view<>()) {
            actual.add(id);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(true, false, { "A"_str, "B"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto [id, item] : m.view<yama::item_desc>()) {
            actual.add(id, item);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(false, true, { "A"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto [id, prim] : m.view<yama::prim_desc>()) {
            actual.add(id, prim);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(true, true, { "A"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto [id, item, prim] : m.view<yama::item_desc, yama::prim_desc>()) {
            actual.add(id, item, prim);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        // failure
        for (const auto [id, item, memb, prim] : m.view<yama::item_desc, yama::member_desc, yama::prim_desc>()) {
            ADD_FAILURE() << "didn't fail!";
            break;
        }
    }
    {
        size_t n = 0;
        // Multiple of same descriptor type + arbitrary order.
        for (const auto [id, item1, prim1, item2, prim2, item3] : m.view<yama::item_desc, yama::prim_desc, yama::item_desc, yama::prim_desc, yama::item_desc>()) {
            n++;
            EXPECT_EQ(&item1, &item2);
            EXPECT_EQ(&item1, &item3);

            EXPECT_EQ(&prim1, &prim2);
        }
        EXPECT_EQ(n, 3);
    }
}

TEST(ModuleTests, ViewItems) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));
    ASSERT_TRUE(m.add_struct("B"_str, yama::const_table{}));
    ASSERT_TRUE(m.add_primitive("C"_str, yama::const_table{}, yama::ptype::float0));
    ASSERT_TRUE(m.add_primitive("D"_str, yama::const_table{}, yama::ptype::char0));

    struct Result final {
        std::unordered_set<yama::lid_t> ids; // Need this for IDs iterated over, but w/ no descriptors.
        bool operator==(const Result&) const noexcept = default;
        void add(yama::module::item x) { ids.insert(x.id()); }
        std::string fmt() const {
            std::string result{};
            result += "Result:\n";
            result += std::format("    ids ({}):\n", ids.size());
            for (const auto& id : ids) {
                result += std::format("        {}\n", id);
            }
            return result;
        }
    };

    auto expected_result = [&m](bool include_item, bool include_prim, std::vector<yama::str> names) -> Result {
        Result result{};
        for (const auto& name : names) {
            result.add(m.get_item<>(name).value());
        }
        return result;
    };

    {
        Result expected = expected_result(false, false, { "A"_str, "B"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto item : m.view_items<>()) {
            actual.add(item);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(true, false, { "A"_str, "B"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto item : m.view_items<yama::item_desc>()) {
            actual.add(item);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(false, true, { "A"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto item : m.view_items<yama::prim_desc>()) {
            actual.add(item);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        Result expected = expected_result(true, true, { "A"_str, "C"_str, "D"_str });
        Result actual{};
        for (const auto item : m.view_items<yama::item_desc, yama::prim_desc>()) {
            actual.add(item);
        }
        EXPECT_EQ(actual, expected) << "\n(expected) " << expected.fmt() << "\n(actual) " << actual.fmt();
    }
    {
        // failure
        for (const auto item : m.view_items<yama::item_desc, yama::member_desc, yama::prim_desc>()) {
            ADD_FAILURE() << "didn't fail!";
            break;
        }
    }
}

TEST(ModuleTests, Name) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const auto id = m.id("A"_str).value();

    const auto result = m.name(id);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(*result, "A"_str);
}

TEST(ModuleTests, ID) {
    yama::module m{};

    ASSERT_TRUE(m.add_primitive("A"_str, yama::const_table{}, yama::ptype::int0));

    const auto result = m.id("A"_str);
    ASSERT_TRUE(result.has_value());
}

TEST(ModuleTests, BindBCode) {
    yama::module m{};

    const auto bcode =
        yama::bc::code_writer()
        .add_noop()
        .add_noop()
        .add_noop()
        .add_noop()
        .done()
        .value();
    yama::bc::syms bsyms{};
    bsyms.add(0, ""_str, 10, 10);
    bsyms.add(1, ""_str, 10, 11);
    bsyms.add(2, ""_str, 10, 12);
    bsyms.add(3, ""_str, 10, 13);

    const auto bcode2 =
        yama::bc::code_writer()
        .done()
        .value();
    yama::bc::syms bsyms2{};

    ASSERT_TRUE(m.add_function("A"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));
    ASSERT_TRUE(m.add_function("B"_str, yama::const_table{}, yama::make_callsig({}, 0), 10, [](yama::context&) {}));

    {
        // test name overload via A
        const auto name = "A"_str;
        EXPECT_TRUE(m.none_of<yama::bcode_desc>(name));
        EXPECT_TRUE(m.bind_bcode(name, bcode, bsyms));
        if (m.all_of<yama::bcode_desc>(name)) {
            const auto [bc] = m.get<yama::bcode_desc>(name);
            EXPECT_EQ(bc.bcode, bcode);
            EXPECT_EQ(bc.bsyms, bsyms);
        }
        EXPECT_FALSE(m.bind_bcode(name, bcode2, bsyms2));
        if (m.all_of<yama::bcode_desc>(name)) {
            const auto [bc] = m.get<yama::bcode_desc>(name);
            EXPECT_EQ(bc.bcode, bcode);
            EXPECT_EQ(bc.bsyms, bsyms);
        }
    }
    {
        // test ID overload via B
        const auto id = m.id("B"_str).value();
        EXPECT_TRUE(m.none_of<yama::bcode_desc>(id));
        EXPECT_TRUE(m.bind_bcode(id, bcode, bsyms));
        if (m.all_of<yama::bcode_desc>(id)) {
            const auto [bc] = m.get<yama::bcode_desc>(id);
            EXPECT_EQ(bc.bcode, bcode);
            EXPECT_EQ(bc.bsyms, bsyms);
        }
        EXPECT_FALSE(m.bind_bcode(id, bcode2, bsyms2));
        if (m.all_of<yama::bcode_desc>(id)) {
            const auto [bc] = m.get<yama::bcode_desc>(id);
            EXPECT_EQ(bc.bcode, bcode);
            EXPECT_EQ(bc.bsyms, bsyms);
        }
    }
    {
        EXPECT_FALSE(m.bind_bcode("missing"_str, bcode, bsyms));
    }
    {
        constexpr auto invalid_id = 10'000;
        EXPECT_FALSE(m.bind_bcode(invalid_id, bcode, bsyms));
    }
}

