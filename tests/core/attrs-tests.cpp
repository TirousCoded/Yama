

#include <gtest/gtest.h>

#include <unordered_map>

#include <yama/core/attrs.h>


using ours_t = yama::attrs<std::string, int, float, char>;


TEST(AttrsTests, Supports) {
    static_assert(ours_t::supports<> == true);
    static_assert(ours_t::supports<int> == true);
    static_assert(ours_t::supports<float> == true);
    static_assert(ours_t::supports<char> == true);
    static_assert(ours_t::supports<int, float, char> == true); // multiple
    static_assert(ours_t::supports<bool> == false); // fail
    static_assert(ours_t::supports<std::string> == false); // fail
    static_assert(ours_t::supports<bool, int> == false); // fail
}

TEST(AttrsTests, Empty) {
    ours_t a{};

    EXPECT_EQ(a.total(), 0);
}

TEST(AttrsTests, Count) {
    ours_t a{};

    a.bind("A", 10, 'a');
    a.bind("B", 11, 'b', 1.72f);
    a.bind("C", 'c', 3.47f);
    a.bind("D", 12);

    size_t v{};
    EXPECT_EQ(a.count<>(), 0);
    EXPECT_EQ(a.count<int>(), 3);
    EXPECT_EQ(a.count<float>(), 2);
    EXPECT_EQ(a.count<char>(), 3);
    v = a.count<int, float>();
    EXPECT_EQ(v, 5);
    v = a.count<int, char>();
    EXPECT_EQ(v, 6);
    v = a.count<int, float, char>();
    EXPECT_EQ(v, 8);
    v = a.count<int, int, int, int>(); // count ints 4 times
    EXPECT_EQ(v, 12);
}

TEST(AttrsTests, CountFor) {
    ours_t a{};

    a.bind("A", 10, 'a');
    a.bind("B", 11, 'b', 1.72f);
    a.bind("C", 'c', 3.47f);
    a.bind("D", 12);

#define _HELPER(tempargs, A, B, C, D) \
EXPECT_EQ(a.count_for tempargs ("A"), (A)) << "A==" << A; \
EXPECT_EQ(a.count_for tempargs ("B"), (B)) << "B==" << B; \
EXPECT_EQ(a.count_for tempargs ("C"), (C)) << "C==" << C; \
EXPECT_EQ(a.count_for tempargs ("D"), (D)) << "D==" << D; \
EXPECT_EQ(a.count_for tempargs ("A", "D"), (A)+(D)) << "A+D==" << (A)+(D); \
EXPECT_EQ(a.count_for tempargs ("A", "B", "C", "D"), (A)+(B)+(C)+(D)) << "A+B+C+D==" << (A)+(B)+(C)+(D); \
/* count A 3 times */ \
EXPECT_EQ(a.count_for tempargs ("A", "B", "C", "D", "A", "A"), (A)*3+(B)+(C)+(D)) << "A*3+B+C+D==" << (A)*3+(B)+(C)+(D)

    _HELPER(<>, 0, 0, 0, 0);
    _HELPER(<int>, 1, 1, 0, 1);
    _HELPER(<float>, 0, 1, 1, 0);
    _HELPER(<char>, 1, 1, 1, 0);
#define _TEMPARGS <int, float>
    _HELPER(_TEMPARGS, 1, 2, 1, 1);
#undef _TEMPARGS
#define _TEMPARGS <int, char>
    _HELPER(_TEMPARGS, 2, 2, 1, 1);
#undef _TEMPARGS
#define _TEMPARGS <float, char>
    _HELPER(_TEMPARGS, 1, 2, 2, 0);
#undef _TEMPARGS
#define _TEMPARGS <int, float, char>
    _HELPER(_TEMPARGS, 2, 3, 2, 1);
#undef _TEMPARGS
#define _TEMPARGS <int, float, char, int, int> // count ints 3 times
    _HELPER(_TEMPARGS, 4, 5, 2, 3);
#undef _TEMPARGS

#undef _HELPER
}

TEST(AttrsTests, Total) {
    ours_t a{};

    a.bind("A", 10, 'a');
    a.bind("B", 11, 'b', 1.72f);
    a.bind("C", 'c', 3.47f);
    a.bind("D", 12);

    EXPECT_EQ(a.total(), 8);
}

TEST(AttrsTests, TotalFor) {
    ours_t a{};

    a.bind("A", 10, 'a');
    a.bind("B", 11, 'b', 1.72f);
    a.bind("C", 'c', 3.47f);
    a.bind("D", 12);

    const size_t A_total = 2, B_total = 3, C_total = 2, D_total = 1;
    EXPECT_EQ(a.total_for("A"), A_total);
    EXPECT_EQ(a.total_for("B"), B_total);
    EXPECT_EQ(a.total_for("C"), C_total);
    EXPECT_EQ(a.total_for("D"), D_total);
    EXPECT_EQ(a.total_for("A", "D"), A_total + D_total);
    EXPECT_EQ(a.total_for("A", "B", "C", "D"), A_total + B_total + C_total + D_total);
    // count A 3 times
    EXPECT_EQ(a.total_for("A", "B", "C", "D", "A", "A"), A_total * 3 + B_total + C_total + D_total);
}

TEST(AttrsTests, Have) {
    ours_t a{};

    a.bind("A", 10, 'a');
    a.bind("B", 11, 'b', 1.72f);
    a.bind("C", 'c', 3.47f);
    a.bind("D", 12);

#define _HELPER(tempargs, A, B, C, D) \
EXPECT_EQ(a.have tempargs ("A"), (A)) << "A==" << A; \
EXPECT_EQ(a.have tempargs ("B"), (B)) << "B==" << B; \
EXPECT_EQ(a.have tempargs ("C"), (C)) << "C==" << C; \
EXPECT_EQ(a.have tempargs ("D"), (D)) << "D==" << D; \
EXPECT_EQ(a.have tempargs ("A", "D"), (A)&&(D)) << "A && D==" << ((A)&&(D)); \
EXPECT_EQ(a.have tempargs ("A", "B", "C", "D"), (A)&&(B)&&(C)&&(D)) << "A && B && C && D==" << ((A)&&(B)&&(C)&&(D)); \
EXPECT_EQ(a.have tempargs ("A", "B", "C", "D", "A", "A"), (A)&&(B)&&(C)&&(D)) << "A && B && C && D && A && A==" << ((A)&&(B)&&(C)&&(D))

    _HELPER(<>,         true,   true,   true,   true);
    _HELPER(<int>,      true,   true,   false,  true);
    _HELPER(<float>,    false,  true,   true,   false);
    _HELPER(<char>,     true,   true,   true,   false);
#define _TEMPARGS <int, float>
    _HELPER(_TEMPARGS,  false,  true,   false,  false);
#undef _TEMPARGS
#define _TEMPARGS <int, char>
    _HELPER(_TEMPARGS,  true,   true,   false,  false);
#undef _TEMPARGS
#define _TEMPARGS <float, char>
    _HELPER(_TEMPARGS,  false,  true,   true,   false);
#undef _TEMPARGS
#define _TEMPARGS <int, float, char>
    _HELPER(_TEMPARGS,  false,  true,   false,  false);
#undef _TEMPARGS
#define _TEMPARGS <int, float, char, int, int> // check ints 3 times
    _HELPER(_TEMPARGS,  false,  true,   false,  false);
#undef _TEMPARGS

#undef _HELPER
}

TEST(AttrsTests, View) {
    ours_t a{};

    a.bind("A", 3.14159f, 'e', 11);
    a.bind("B", 2);

    const ours_t& a_const = a;

    {
        // shouldn't throw
        std::tuple<> ra = a.view<>("A");
        std::tuple<> rb = a_const.view<>("A");
        std::tuple<> rc = a.view<>("doesn't-exist-at-all-in-a");
        std::tuple<> rd = a_const.view<>("doesn't-exist-at-all-in-a");
    }
    {
        // should throw
        EXPECT_THROW(a.view<int>("missing"), std::out_of_range);
        EXPECT_THROW(a_const.view<int>("missing"), std::out_of_range);
        // B has int but not float
#define _TEMPARGS <int, float>
        EXPECT_THROW(a.view _TEMPARGS ("B"), std::out_of_range);
        EXPECT_THROW(a_const.view _TEMPARGS ("B"), std::out_of_range);
#undef _TEMPARGS
    }
    {
        auto [i, f, c] = a.view<int, float, char>("A");
        EXPECT_EQ(i, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        static_assert(std::is_same_v<int&, decltype(i)>);
    }
    {
        auto [i, f, c] = a_const.view<int, float, char>("A");
        EXPECT_EQ(i, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        static_assert(std::is_same_v<const int&, decltype(i)>);
    }
    {
        // out of order
        auto [f, c, i] = a.view<float, char, int>("A");
        EXPECT_EQ(i, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        static_assert(std::is_same_v<int&, decltype(i)>);
    }
    {
        // out of order
        auto [f, c, i] = a_const.view<float, char, int>("A");
        EXPECT_EQ(i, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        static_assert(std::is_same_v<const int&, decltype(i)>);
    }
    {
        // multiple
        auto [ia, f, c, ib, ic] = a.view<int, float, char, int, int>("A");
        EXPECT_EQ(ia, 11);
        EXPECT_EQ(ib, 11);
        EXPECT_EQ(ic, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        // ia, ib and ic should all share a memory address
        EXPECT_EQ(&ia, &ib);
        EXPECT_EQ(&ia, &ic);
        EXPECT_EQ(&ib, &ic);
        static_assert(std::is_same_v<int&, decltype(ia)>);
    }
    {
        // multiple
        auto [ia, f, c, ib, ic] = a_const.view<int, float, char, int, int>("A");
        EXPECT_EQ(ia, 11);
        EXPECT_EQ(ib, 11);
        EXPECT_EQ(ic, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
        // ia, ib and ic should all share a memory address
        EXPECT_EQ(&ia, &ib);
        EXPECT_EQ(&ia, &ic);
        EXPECT_EQ(&ib, &ic);
        static_assert(std::is_same_v<const int&, decltype(ia)>);
    }
    {
        // lvalue mutability
        auto [ia, fa] = a.view<int, float>("A");
        auto [ib, fb] = a.view<int, float>("A");
        EXPECT_EQ(ia, ib);
        EXPECT_FLOAT_EQ(fa, fb);
        ia = 4;
        fa = 0.14f;
        EXPECT_EQ(ia, ib);
        EXPECT_FLOAT_EQ(fa, fb);
    }
}

TEST(AttrsTests, IteratorsAndTraversers) {
    ours_t a{};

    a.bind("A", 1, 0.01f, 'a');
    a.bind("B", 2, 0.02f, 'b');
    a.bind("C", 3, 0.03f, 'c');
    a.bind("D", 4, 0.04f, 'd');

    const auto& a_const = a;

    std::unordered_map<std::string, char> actual{}, expected{};

    auto prepare = [&]() {
        actual = {};
        expected = {
            { "A", 'a' },
            { "B", 'b' },
            { "C", 'c' },
            { "D", 'd' },
        };
        };
    auto test = [&](std::string_view msg) {
        EXPECT_EQ(actual, expected) << std::string(msg);
        };

    {
        prepare();
        for (auto it = a.begin<char>(); it != a.end<char>(); std::advance(it, 1)) {
            actual.insert(*it);
        }
        test("iterators #1");
    }
    {
        prepare();
        for (auto it = a_const.cbegin<char>(); it != a_const.cend<char>(); std::advance(it, 1)) {
            actual.insert(*it);
        }
        test("iterators #2");
    }
    {
        prepare();
        for (auto it = a_const.begin<char>(); it != a_const.end<char>(); std::advance(it, 1)) {
            actual.insert(*it);
        }
        test("iterators #3");
    }
    {
        prepare();
        for (const auto& [key, value] : a.traverse<char>()) {
            actual.insert({ key, value });
        }
        test("traverser #1");
    }
    {
        prepare();
        for (const auto& [key, value] : a_const.ctraverse<char>()) {
            actual.insert({ key, value });
        }
        test("traverser #2");
    }
    {
        prepare();
        for (const auto& [key, value] : a_const.traverse<char>()) {
            actual.insert({ key, value });
        }
        test("traverser #3");
    }
}

TEST(AttrsTests, Bind) {
    ours_t a{};

    EXPECT_EQ(a.total(), 0);
    EXPECT_EQ(a.have<int>("A"), false);
    EXPECT_EQ(a.have<float>("A"), false);
    EXPECT_EQ(a.have<char>("A"), false);

    a.bind("A");

    EXPECT_EQ(a.total(), 0);
    EXPECT_EQ(a.have<int>("A"), false);
    EXPECT_EQ(a.have<float>("A"), false);
    EXPECT_EQ(a.have<char>("A"), false);

    a.bind("A", 'c', 10);

    EXPECT_EQ(a.total(), 2);
    EXPECT_EQ(a.have<int>("A"), true);
    EXPECT_EQ(a.have<float>("A"), false);
    EXPECT_EQ(a.have<char>("A"), true);
    if (a.have<int, char>("A")) {
        auto [i, c] = a.view<int, char>("A");
        EXPECT_EQ(i, 10);
        EXPECT_EQ(c, 'c');
    }
    else FAIL();

    a.bind("A", 3.14159f, 'e', 11);

    EXPECT_EQ(a.total(), 3);
    EXPECT_EQ(a.have<int>("A"), true);
    EXPECT_EQ(a.have<float>("A"), true);
    EXPECT_EQ(a.have<char>("A"), true);
    if (a.have<int, char>("A")) {
        auto [i, f, c] = a.view<int, float, char>("A");
        EXPECT_EQ(i, 11);
        EXPECT_FLOAT_EQ(f, 3.14159f);
        EXPECT_EQ(c, 'e');
    }
    else FAIL();
}

TEST(AttrsTests, Unbind) {
    ours_t a{};

    a.bind("A", 1, 0.01f, 'a');
    a.bind("B", 2, 0.02f, 'b');
    a.bind("C", 3, 0.03f, 'c');
    a.bind("D", 4, 0.04f, 'd');

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind<>();

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind<>("A");

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind<>("A", "B", "C", "D");

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind<int>("A", "D");

    ASSERT_EQ(a.have<int>("B", "C"), true);
    ASSERT_EQ(a.have<int>("A", "D"), false);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind<float, char>("A", "B");

    ASSERT_EQ(a.have<int>("B", "C"), true);
    ASSERT_EQ(a.have<int>("A", "D"), false);
    ASSERT_EQ(a.have<float>("C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B"), false);
    ASSERT_EQ(a.have<char>("C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B"), false);

    a.unbind<int, float, char>("A", "C"); // fails quietly for A

    ASSERT_EQ(a.have<int>("B"), true);
    ASSERT_EQ(a.have<int>("A", "C", "D"), false);
    ASSERT_EQ(a.have<float>("D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C"), false);
    ASSERT_EQ(a.have<char>("D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C"), false);

    a.unbind<int, float, char>(); // fails quietly

    ASSERT_EQ(a.have<int>("B"), true);
    ASSERT_EQ(a.have<int>("A", "C", "D"), false);
    ASSERT_EQ(a.have<float>("D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C"), false);
    ASSERT_EQ(a.have<char>("D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C"), false);
}

TEST(AttrsTests, UnbindAll) {
    ours_t a{};

    a.bind("A", 1, 0.01f, 'a');
    a.bind("B", 2, 0.02f, 'b');
    a.bind("C", 3, 0.03f, 'c');
    a.bind("D", 4, 0.04f, 'd');

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind_all();

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.unbind_all("A", "C");

    ASSERT_EQ(a.have<int>("B", "D"), true);
    ASSERT_EQ(a.have<int>("A", "C"), false);
    ASSERT_EQ(a.have<float>("B", "D"), true);
    ASSERT_EQ(a.have<float>("A", "C"), false);
    ASSERT_EQ(a.have<char>("B", "D"), true);
    ASSERT_EQ(a.have<char>("A", "C"), false);

    a.unbind_all("A", "B"); // fails quietly for A

    ASSERT_EQ(a.have<int>("D"), true);
    ASSERT_EQ(a.have<int>("A", "B", "C"), false);
    ASSERT_EQ(a.have<float>("D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C"), false);
    ASSERT_EQ(a.have<char>("D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C"), false);
}

TEST(AttrsTests, Reset) {
    ours_t a{};

    a.bind("A", 1, 0.01f, 'a');
    a.bind("B", 2, 0.02f, 'b');
    a.bind("C", 3, 0.03f, 'c');
    a.bind("D", 4, 0.04f, 'd');

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.reset<>();

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.reset<int, float>();

    ASSERT_EQ(a.have<int>(), true);
    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<float>(), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.reset<int, char>(); // fails quietly for int

    ASSERT_EQ(a.have<int>(), true);
    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<float>(), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<char>(), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), false);
}

TEST(AttrsTests, ResetAll) {
    ours_t a{};

    a.bind("A", 1, 0.01f, 'a');
    a.bind("B", 2, 0.02f, 'b');
    a.bind("C", 3, 0.03f, 'c');
    a.bind("D", 4, 0.04f, 'd');

    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), true);

    a.reset_all();

    ASSERT_EQ(a.have<int>(), true);
    ASSERT_EQ(a.have<int>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<float>(), true);
    ASSERT_EQ(a.have<float>("A", "B", "C", "D"), false);
    ASSERT_EQ(a.have<char>(), true);
    ASSERT_EQ(a.have<char>("A", "B", "C", "D"), false);
}

