

#include <gtest/gtest.h>
#include <yama++/Safe.h>


using namespace ym;


TEST(Safe, InitViaLValue) {
    int a = 10;
    Safe<int> b(a);
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer) {
    int a = 10;
    Safe<int> b(&a);
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer_Fail) {
    ASSERT_DEATH({
        Safe<int>((int*)nullptr);
        },
        ".*");
}

namespace {
    class Base {
    public:
        Base() = default;
        virtual ~Base() noexcept = default; // Ensure Base has vtable.
    };
    class Derived : public Base {
    public:
        Derived() = default;
    };
}

TEST(Safe, InitViaRawPointer_WithExplicitConvert) {
    Derived a{};
    Safe<Derived> b((Base*)&a); // Convert Base* -> Derived*.
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer_WithExplicitConvert_Fail) {
    ASSERT_DEATH({
        Safe<Derived>((Base*)nullptr); // Convert Base* -> Derived*.
        },
        ".*");
}

TEST(Safe, InitViaExplicitConvertBetweenSafeTypes) {
    Derived a{};
    Safe<Derived> b(Safe<Base>((Base*)&a)); // Convert Safe<Base> -> Safe<Derived>.
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, CopyCtor) {
    int a = 10;
    Safe<int> b(a);
    Safe<int> c(b); // Copy ctor.
    ASSERT_EQ(c.get(), &a);
    ASSERT_EQ(b.get(), &a); // Still good.
}

TEST(Safe, MoveCtor) {
    int a = 10;
    Safe<int> b(a);
    Safe<int> c(std::move(b)); // Move ctor.
    ASSERT_EQ(c.get(), &a);
}

TEST(Safe, CopyAssign) {
    int a1 = 10;
    int a2 = 20;
    Safe<int> other(a2);
    Safe<int> b(a1);
    b = other; // Copy assign.
    ASSERT_EQ(b.get(), &a2);
    ASSERT_EQ(other.get(), &a2); // Still good.
}

TEST(Safe, MoveAssign) {
    int a1 = 10;
    int a2 = 20;
    Safe<int> other(a2);
    Safe<int> b(a1);
    b = std::move(other); // Move assign.
    ASSERT_EQ(b.get(), &a2);
}

// TODO: This 'Equality' test doesn't make a distinction between operator== and operator!=.

TEST(Safe, Equality) {
    int v1 = 10;
    int v2 = 10;
    Safe<int> a1(v1);
    Safe<int> a2(v1);
    Safe<int> b(v2);
    ASSERT_EQ(a1, a1);
    ASSERT_EQ(a1, a2);
    ASSERT_NE(a1, b);
}

TEST(Safe, Get) {
    int v1 = 10;
    int v2 = 10;
    Safe<int> a1(v1);
    Safe<int> a2(v1);
    Safe<int> b(v2);
    ASSERT_EQ(a1.get(), &v1);
    ASSERT_EQ(a2.get(), &v1);
    ASSERT_EQ(b.get(), &v2);
}

TEST(Safe, Value) {
    int v1 = 10;
    int v2 = 20;
    Safe<int> a1(v1);
    Safe<int> a2(v1);
    Safe<int> b(v2);
    // Assert that returned lvalue uses correct pointer.
    ASSERT_EQ(&(a1.value()), &v1);
    ASSERT_EQ(&(a2.value()), &v1);
    ASSERT_EQ(&(b.value()), &v2);
    // Just for good measure, we'll also test deref.
    ASSERT_EQ(a1.value(), v1);
    ASSERT_EQ(a2.value(), v1);
    ASSERT_EQ(b.value(), v2);
}

namespace {
    struct A {
        int a;
    };
}

TEST(Safe, DerefViaArrow) {
    A v{ 10 };
    Safe<A> a(v);
    // Assert that returned lvalue uses correct pointer.
    ASSERT_EQ(&(a->a), &(v.a));
    // Just for good measure, we'll also test deref.
    ASSERT_EQ(a->a, v.a);
}

TEST(Safe, DerefViaStar) {
    int v1 = 10;
    int v2 = 20;
    Safe<int> a1(v1);
    Safe<int> a2(v1);
    Safe<int> b(v2);
    // Assert that returned lvalue uses correct pointer.
    ASSERT_EQ(&(*a1), &v1);
    ASSERT_EQ(&(*a2), &v1);
    ASSERT_EQ(&(*b), &v2);
    // Just for good measure, we'll also test deref.
    ASSERT_EQ(*a1, v1);
    ASSERT_EQ(*a2, v1);
    ASSERT_EQ(*b, v2);
}

TEST(Safe, ArrayAccess) {
    int arr[10]{};
    for (size_t i = 0; i < 10; i++) {
        EXPECT_EQ(&Safe(arr[0])[i], &arr[i]) << "i == " << i;
    }
}

TEST(Safe, ImplicitConvertToRawPointerOfSameElemType) {
    Derived v{};
    Safe<Derived> a(v);
    Derived* b = a;
    ASSERT_EQ(b, &v);
}

TEST(Safe, ExplicitConvertToRawPointerOfDiffElemType) {
    Derived v{};
    Safe<Base> a(v);
    Derived* b = (Derived*)a; // Convert Safe<Base> -> Derived*.
    ASSERT_EQ(b, &v);
}

TEST(Safe, Into) {
    Derived v{};
    Safe<Base> a(v);
    Derived* b = a.into<Derived>(); // Convert Safe<Base> -> Derived*.
    ASSERT_EQ(b, &v);
}

TEST(Safe, DowncastInto) {
    Derived v{};
    Safe<Base> a = v;
    // Downcasting from base type to derived type, which is a circumstance in which
    // the compiler might not know the runtime type.
    Safe<Derived> b = a.downcastInto<Derived>();
    ASSERT_EQ(b, Safe(v));
}

TEST(Safe, Hash) {
    int v = 10;
    // Hashing Safe(v) should yield equiv to hashing &v.
    EXPECT_EQ(Safe(v).hash(), hash(&v));
    EXPECT_EQ(std::hash<Safe<int>>{}(Safe(v)), hash(&v));
}

TEST(Safe, Addition) {
    int arr[10]{};
    EXPECT_EQ(Safe(arr[0]) + 0, Safe(arr[0]));
    EXPECT_EQ(Safe(arr[0]) + 3, Safe(arr[3]));
    EXPECT_EQ(Safe(arr[0]) + 3 + 3 + 3, Safe(arr[9]));
}

TEST(Safe, Subtraction) {
    int arr[10]{};
    EXPECT_EQ(Safe(arr[9]) - 0, Safe(arr[9]));
    EXPECT_EQ(Safe(arr[9]) - 3, Safe(arr[6]));
    EXPECT_EQ(Safe(arr[9]) - 3 - 3 - 3, Safe(arr[0]));
}

TEST(Safe, AddAssign) {
    int arr[10]{};
    auto a = Safe(arr[0]);
    auto& b = a += 5;
    EXPECT_EQ(a, Safe(arr[5]));
    EXPECT_EQ(&a, &b);
}

TEST(Safe, SubtractAssign) {
    int arr[10]{};
    auto a = Safe(arr[9]);
    auto& b = a -= 5;
    EXPECT_EQ(a, Safe(arr[4]));
    EXPECT_EQ(&a, &b);
}

TEST(Safe, PreIncr) {
    int arr[10]{};
    auto a = Safe(arr[0]);
    auto& b = ++a;
    EXPECT_EQ(a, Safe(arr[1]));
    EXPECT_EQ(&a, &b);
}

TEST(Safe, PreDecr) {
    int arr[10]{};
    auto a = Safe(arr[9]);
    auto& b = --a;
    EXPECT_EQ(a, Safe(arr[8]));
    EXPECT_EQ(&a, &b);
}

TEST(Safe, PostIncr) {
    int arr[10]{};
    auto a = Safe(arr[0]);
    auto b = a++;
    EXPECT_EQ(a, Safe(arr[1]));
    EXPECT_EQ(b, Safe(arr[0]));
}

TEST(Safe, PostDecr) {
    int arr[10]{};
    auto a = Safe(arr[9]);
    auto b = a--;
    EXPECT_EQ(a, Safe(arr[8]));
    EXPECT_EQ(b, Safe(arr[9]));
}

// NOTE: Alongside Safe, we'll also test assertSafe.

TEST(Safe, AssertSafe) {
    int v = 10;
    assertSafe(&v); // Succeeds
    ASSERT_DEATH({
        assertSafe((int*)nullptr); // Fails
        },
        ".*");
}

