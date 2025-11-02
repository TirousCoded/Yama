

#include <gtest/gtest.h>
#include <yama++/Safe.h>


TEST(Safe, InitViaLValue) {
    int a = 10;
    ym::Safe<int> b(a);
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer) {
    int a = 10;
    ym::Safe<int> b(&a);
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer_Fail) {
    ASSERT_DEATH({
        ym::Safe<int>((int*)nullptr);
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
    ym::Safe<Derived> b((Base*)&a); // Convert Base* -> Derived*.
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, InitViaRawPointer_WithExplicitConvert_Fail) {
    ASSERT_DEATH({
        ym::Safe<Derived>((Base*)nullptr); // Convert Base* -> Derived*.
        },
        ".*");
}

TEST(Safe, InitViaExplicitConvertBetweenSafeTypes) {
    Derived a{};
    ym::Safe<Derived> b(ym::Safe<Base>((Base*)&a)); // Convert ym::Safe<Base> -> ym::Safe<Derived>.
    ASSERT_EQ(b.get(), &a);
}

TEST(Safe, CopyCtor) {
    int a = 10;
    ym::Safe<int> b(a);
    ym::Safe<int> c(b); // Copy ctor.
    ASSERT_EQ(c.get(), &a);
    ASSERT_EQ(b.get(), &a); // Still good.
}

TEST(Safe, MoveCtor) {
    int a = 10;
    ym::Safe<int> b(a);
    ym::Safe<int> c(std::move(b)); // Move ctor.
    ASSERT_EQ(c.get(), &a);
}

TEST(Safe, CopyAssign) {
    int a1 = 10;
    int a2 = 20;
    ym::Safe<int> other(a2);
    ym::Safe<int> b(a1);
    b = other; // Copy assign.
    ASSERT_EQ(b.get(), &a2);
    ASSERT_EQ(other.get(), &a2); // Still good.
}

TEST(Safe, MoveAssign) {
    int a1 = 10;
    int a2 = 20;
    ym::Safe<int> other(a2);
    ym::Safe<int> b(a1);
    b = std::move(other); // Move assign.
    ASSERT_EQ(b.get(), &a2);
}

// TODO: This 'Equality' test doesn't make a distinction between operator== and operator!=.

TEST(Safe, Equality) {
    int v1 = 10;
    int v2 = 10;
    ym::Safe<int> a1(v1);
    ym::Safe<int> a2(v1);
    ym::Safe<int> b(v2);
    ASSERT_EQ(a1, a1);
    ASSERT_EQ(a1, a2);
    ASSERT_NE(a1, b);
}

TEST(Safe, Get) {
    int v1 = 10;
    int v2 = 10;
    ym::Safe<int> a1(v1);
    ym::Safe<int> a2(v1);
    ym::Safe<int> b(v2);
    ASSERT_EQ(a1.get(), &v1);
    ASSERT_EQ(a2.get(), &v1);
    ASSERT_EQ(b.get(), &v2);
}

TEST(Safe, Value) {
    int v1 = 10;
    int v2 = 20;
    ym::Safe<int> a1(v1);
    ym::Safe<int> a2(v1);
    ym::Safe<int> b(v2);
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
    ym::Safe<A> a(v);
    // Assert that returned lvalue uses correct pointer.
    ASSERT_EQ(&(a->a), &(v.a));
    // Just for good measure, we'll also test deref.
    ASSERT_EQ(a->a, v.a);
}

TEST(Safe, DerefViaStar) {
    int v1 = 10;
    int v2 = 20;
    ym::Safe<int> a1(v1);
    ym::Safe<int> a2(v1);
    ym::Safe<int> b(v2);
    // Assert that returned lvalue uses correct pointer.
    ASSERT_EQ(&(*a1), &v1);
    ASSERT_EQ(&(*a2), &v1);
    ASSERT_EQ(&(*b), &v2);
    // Just for good measure, we'll also test deref.
    ASSERT_EQ(*a1, v1);
    ASSERT_EQ(*a2, v1);
    ASSERT_EQ(*b, v2);
}

TEST(Safe, ImplicitConvertToRawPointerOfSameElemType) {
    Derived v{};
    ym::Safe<Derived> a(v);
    Derived* b = a;
    ASSERT_EQ(b, &v);
}

TEST(Safe, ExplicitConvertToRawPointerOfDiffElemType) {
    Derived v{};
    ym::Safe<Base> a(v);
    Derived* b = (Derived*)a; // Convert ym::Safe<Base> -> Derived*.
    ASSERT_EQ(b, &v);
}

TEST(Safe, Into) {
    Derived v{};
    ym::Safe<Base> a(v);
    Derived* b = a.into<Derived>(); // Convert ym::Safe<Base> -> Derived*.
    ASSERT_EQ(b, &v);
}

TEST(Safe, DowncastInto) {
    Derived v{};
    ym::Safe<Base> a = v;
    // Downcasting from base type to derived type, which is a circumstance in which
    // the compiler might not know the runtime type.
    ym::Safe<Derived> b = a.downcastInto<Derived>();
    ASSERT_EQ(b, ym::Safe(v));
}

TEST(Safe, Hash) {
    int v = 10;
    // Hashing Safe(v) should yield equiv to hashing &v.
    EXPECT_EQ(ym::Safe(v).hash(), ym::hash(&v));
    EXPECT_EQ(std::hash<ym::Safe<int>>{}(ym::Safe(v)), ym::hash(&v));
}

// NOTE: Alongside ym::Safe, we'll also test ym::assertSafe.

TEST(Safe, AssertSafe) {
    int v = 10;
    ym::assertSafe(&v); // Succeeds
    ASSERT_DEATH({
        ym::assertSafe((int*)nullptr); // Fails
        },
        ".*");
}

