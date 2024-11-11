

#include <gtest/gtest.h>

#include <yama/core/res.h>


// IMPORTANT:
//      these unit tests don't cover move semantic nuances like
//      self-assignment or reinit of move-from objects


// some structs to properly test base type conversion

struct A {};
struct B : public A {};


TEST(ResTests, SharedPtrCtor) {
    auto ptr = std::make_shared<B>();

    yama::res r1(ptr);
    yama::res<A> r2(ptr);

    EXPECT_EQ(r1.base(), ptr);
    EXPECT_EQ(r2.base(), ptr);
}

TEST(ResTests, SharedPtrCtorThrowsResErrorIfArgIsNullptr) {
    // remember, this test is for init via std::shared_ptr, and NOT a std::nullptr_t value!
    EXPECT_THROW(yama::res(std::shared_ptr<B>{}), yama::res_error);
    EXPECT_THROW(yama::res<A>(std::shared_ptr<B>{}), yama::res_error);
}

TEST(ResTests, CopyCtor) {
    auto ptr = std::make_shared<B>();

    yama::res original(ptr);

    yama::res r1(original);
    yama::res<A> r2(original);

    EXPECT_EQ(original.base(), ptr);

    EXPECT_EQ(r1.base(), ptr);
    EXPECT_EQ(r2.base(), ptr);
}

TEST(ResTests, MoveCtor) {
    auto ptr = std::make_shared<B>();

    yama::res original1(ptr);
    yama::res original2(ptr);

    yama::res r1(std::move(original1));
    yama::res<A> r2(std::move(original2));

    EXPECT_EQ(r1.base(), ptr);
    EXPECT_EQ(r2.base(), ptr);
}

TEST(ResTests, CopyAssign) {
    auto assigner_ptr = std::make_shared<B>();
    auto assignee_ptr = std::make_shared<B>();

    yama::res assigner(assigner_ptr);

    yama::res assignee_a(assignee_ptr);
    yama::res<A> assignee_b(assignee_ptr);

    assignee_a = assigner;
    assignee_b = assigner;

    EXPECT_EQ(assignee_a.base(), assigner_ptr);
    EXPECT_EQ(assignee_b.base(), assigner_ptr);
}

TEST(ResTests, MoveAssign) {
    auto assigner_ptr = std::make_shared<B>();
    auto assignee_ptr = std::make_shared<B>();

    yama::res assigner_a(assigner_ptr);
    yama::res assigner_b(assigner_ptr);

    yama::res assignee_a(assignee_ptr);
    yama::res<A> assignee_b(assignee_ptr);

    assignee_a = std::move(assigner_a);
    assignee_b = std::move(assigner_b);

    EXPECT_EQ(assignee_a.base(), assigner_ptr);
    EXPECT_EQ(assignee_b.base(), assigner_ptr);
}

TEST(ResTests, ImplicitConvertFromResToSharedPtr) {
    auto new_value = std::make_shared<B>();
    std::shared_ptr<B> target_a = std::make_shared<B>();
    std::shared_ptr<A> target_b = std::make_shared<B>();

    yama::res r(new_value);

    // implicit convert to shared_ptr

    target_a = r;
    target_b = r;

    // should replace target's old ptrs w/ new_value

    EXPECT_EQ(target_a, new_value);
    EXPECT_EQ(target_b, new_value);
}

TEST(ResTests, Base) {
    auto ptr = std::make_shared<int>(15);
    yama::res r(ptr);

    EXPECT_EQ(r.base(), ptr);
}

TEST(ResTests, Get) {
    auto ptr = std::make_shared<int>(15);
    yama::res r(ptr);

    EXPECT_EQ(r.get(), ptr.get());
}

TEST(ResTests, PointerLike) {
    auto ptr = std::make_shared<std::string>("abc");
    yama::res r(ptr);
    
    EXPECT_EQ(&(*r), ptr.get());

    EXPECT_EQ(r->length(), ptr->length());
}

TEST(ResTests, UseCount) {
    auto ptr = std::make_shared<int>(15);
    yama::res r(ptr);

    EXPECT_EQ(r.use_count(), 2);

    ptr = nullptr; // <- changes to underlying shared_ptr also affects res

    EXPECT_EQ(r.use_count(), 1);

    decltype(r) r0 = r;

    EXPECT_EQ(r.use_count(), 2);

    decltype(r) r1 = r;

    EXPECT_EQ(r.use_count(), 3);
}

TEST(ResTests, Equality) {
    auto ptra = std::make_shared<B>();
    auto ptrb = std::make_shared<B>(); // ptra and ptrb share same value, but differ in address
    yama::res ra(ptra);
    yama::res rb(ptrb);
    yama::res<A> ra_alt = ra;
    yama::res<A> rb_alt = rb;

    // == yama::res

    EXPECT_TRUE(ra == ra);
    EXPECT_FALSE(ra == rb);
    EXPECT_TRUE(ra == ra_alt);
    EXPECT_FALSE(ra == rb_alt);
    
    EXPECT_FALSE(rb == ra);
    EXPECT_TRUE(rb == rb);
    EXPECT_FALSE(rb == ra_alt);
    EXPECT_TRUE(rb == rb_alt);

    EXPECT_TRUE(ra_alt == ra);
    EXPECT_FALSE(ra_alt == rb);
    EXPECT_TRUE(ra_alt == ra_alt);
    EXPECT_FALSE(ra_alt == rb_alt);

    EXPECT_FALSE(rb_alt == ra);
    EXPECT_TRUE(rb_alt == rb);
    EXPECT_FALSE(rb_alt == ra_alt);
    EXPECT_TRUE(rb_alt == rb_alt);
    
    // == std::shared_ptr

    EXPECT_TRUE(ra == ptra);
    EXPECT_FALSE(ra == ptrb);
    
    EXPECT_FALSE(rb == ptra);
    EXPECT_TRUE(rb == ptrb);

    EXPECT_TRUE(ra_alt == ptra);
    EXPECT_FALSE(ra_alt == ptrb);

    EXPECT_FALSE(rb_alt == ptra);
    EXPECT_TRUE(rb_alt == ptrb);
    
    // std::shared_ptr ==

    EXPECT_TRUE(ptra == ra);
    EXPECT_FALSE(ptrb == ra);
    
    EXPECT_FALSE(ptra == rb);
    EXPECT_TRUE(ptrb == rb);

    EXPECT_TRUE(ptra == ra_alt);
    EXPECT_FALSE(ptrb == ra_alt);

    EXPECT_FALSE(ptra == rb_alt);
    EXPECT_TRUE(ptrb == rb_alt);

    // != yama::res

    EXPECT_FALSE(ra != ra);
    EXPECT_TRUE(ra != rb);
    EXPECT_FALSE(ra != ra_alt);
    EXPECT_TRUE(ra != rb_alt);
    
    EXPECT_TRUE(rb != ra);
    EXPECT_FALSE(rb != rb);
    EXPECT_TRUE(rb != ra_alt);
    EXPECT_FALSE(rb != rb_alt);

    EXPECT_FALSE(ra_alt != ra);
    EXPECT_TRUE(ra_alt != rb);
    EXPECT_FALSE(ra_alt != ra_alt);
    EXPECT_TRUE(ra_alt != rb_alt);

    EXPECT_TRUE(rb_alt != ra);
    EXPECT_FALSE(rb_alt != rb);
    EXPECT_TRUE(rb_alt != ra_alt);
    EXPECT_FALSE(rb_alt != rb_alt);

    // != std::shared_ptr

    EXPECT_FALSE(ra != ptra);
    EXPECT_TRUE(ra != ptrb);
    
    EXPECT_TRUE(rb != ptra);
    EXPECT_FALSE(rb != ptrb);

    EXPECT_FALSE(ra_alt != ptra);
    EXPECT_TRUE(ra_alt != ptrb);

    EXPECT_TRUE(rb_alt != ptra);
    EXPECT_FALSE(rb_alt != ptrb);

    // std::shared_ptr !=

    EXPECT_FALSE(ptra != ra);
    EXPECT_TRUE(ptrb != ra);
    
    EXPECT_TRUE(ptra != rb);
    EXPECT_FALSE(ptrb != rb);

    EXPECT_FALSE(ptra != ra_alt);
    EXPECT_TRUE(ptrb != ra_alt);

    EXPECT_TRUE(ptra != rb_alt);
    EXPECT_FALSE(ptrb != rb_alt);
}

TEST(ResTests, EqualityWithNullptr) {
    yama::res r(std::make_shared<int>(15));

    EXPECT_FALSE(r == nullptr);

    EXPECT_TRUE(r != nullptr);
}

TEST(ResTests, MakeRes) {
    auto r = yama::make_res<std::string>("abc");

    EXPECT_EQ(*r, std::string("abc"));
}

