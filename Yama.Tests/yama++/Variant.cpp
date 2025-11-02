

#include <exception>
#include <gtest/gtest.h>
#include <yama++/print.h>
#include <yama++/Variant.h>


// TODO: Our tests don't cover std::initializer_list overloads.
// TODO: Our tests don't really distinguish between const and non-const overloads.


namespace {
    struct NonFormattableStruct {};

    struct ValuelessByExceptionHelper {
        ValuelessByExceptionHelper() = default;
        ValuelessByExceptionHelper(int) {
            throw std::runtime_error("");
        }
    };

    template<typename VariantT>
    VariantT mkVBE() {
        auto result = VariantT::template byType<ValuelessByExceptionHelper>();
        try {
            result.emplace<ValuelessByExceptionHelper>(int()); // int overload will throw.
        }
        catch (const std::runtime_error&) {
            //
        }
        EXPECT_TRUE(result.valuelessByException()) << "mkVBE is defective!";
        return result;
    }

    template<typename VariantT>
    void mkVBEToTestEmplaceThrowing() {
        auto result = VariantT::template byType<ValuelessByExceptionHelper>();
        result.emplace<ValuelessByExceptionHelper>(int()); // int overload will throw.
    }
}

template<>
struct std::hash<ValuelessByExceptionHelper> {
    inline size_t operator()(const ValuelessByExceptionHelper& x) const noexcept {
        return 51; // Some arbitrary number that isn't 0.
    }
};


TEST(Variant, InitViaStdVariant_Copy) {
    using _Variant = ym::Variant<int, float>;

    _Variant::Underlying a(std::in_place_type_t<float>{}, 10.4f);
    _Variant b(a); // Copy a.

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, InitViaStdVariant_Move) {
    using _Variant = ym::Variant<int, float>;

    _Variant::Underlying a(std::in_place_type_t<float>{}, 10.4f);
    _Variant b(std::move(a)); // Move a.

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, InitViaAltValue) {
    using _Variant = ym::Variant<int, float>;

    float a = 10.4f;
    _Variant b(a);

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, AltValueMoveAssign) {
    using _Variant = ym::Variant<int, float>;

    float a = 10.4f;
    _Variant b = _Variant::byType<int>(0);
    b = std::move(a);

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, CopyCtor) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = 10.4f;
    _Variant b(a); // Copy

    ASSERT_TRUE(a.is<float>());
    EXPECT_FLOAT_EQ(a.as<float>(), 10.4f);

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, MoveCtor) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = 10.4f;
    _Variant b(std::move(a)); // Move

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, CopyAssign) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = 10.4f;
    _Variant b = -3;
    b = a; // Copy

    ASSERT_TRUE(a.is<float>());
    EXPECT_FLOAT_EQ(a.as<float>(), 10.4f);

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, MoveAssign) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = 10.4f;
    _Variant b = -3;
    b = std::move(a); // Move

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 10.4f);
}

TEST(Variant, InitViaByType) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = _Variant::byType<float>(10.4f);

    ASSERT_TRUE(a.is<float>());
    EXPECT_FLOAT_EQ(a.as<float>(), 10.4f);
}

TEST(Variant, InitViaByIndex) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = _Variant::byIndex<1>(10.4f);

    ASSERT_TRUE(a.is<float>());
    EXPECT_FLOAT_EQ(a.as<float>(), 10.4f);
}

TEST(Variant, Equality) {
    using _Variant = ym::Variant<int, float>;

    _Variant a = -3;
    _Variant b = -3;
    _Variant c = 12;
    _Variant d = 3.14159f;

    EXPECT_EQ(a, a);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);

    EXPECT_EQ(b, a);
    EXPECT_EQ(b, b);
    EXPECT_NE(b, c);
    EXPECT_NE(b, d);

    EXPECT_NE(c, a);
    EXPECT_NE(c, b);
    EXPECT_EQ(c, c);
    EXPECT_NE(c, d);

    EXPECT_NE(d, a);
    EXPECT_NE(d, b);
    EXPECT_NE(d, c);
    EXPECT_EQ(d, d);
}

TEST(Variant, ImplicitConvertToUnderlyingStdVariant) {
    using _Variant = ym::Variant<int, std::string>;

    _Variant a = -3;
    _Variant b = 12;
    _Variant c = std::string("hello");

    _Variant::Underlying aa = a;
    _Variant::Underlying bb = b;
    _Variant::Underlying cc = c;

    EXPECT_EQ(aa, _Variant::Underlying(std::in_place_type_t<int>{}, -3));
    EXPECT_EQ(bb, _Variant::Underlying(std::in_place_type_t<int>{}, 12));
    EXPECT_EQ(cc, _Variant::Underlying(std::in_place_type_t<std::string>{}, "hello"));
}

TEST(Variant, Index) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    EXPECT_EQ(a.index(), 0);
    EXPECT_EQ(b.index(), 1);
    EXPECT_EQ(c.index(), std::variant_npos);
}

TEST(Variant, ValuelessByException) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    EXPECT_FALSE(a.valuelessByException());
    EXPECT_FALSE(b.valuelessByException());
    EXPECT_TRUE(c.valuelessByException());
}

TEST(Variant, Is) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    EXPECT_TRUE(a.is<int>());
    EXPECT_FALSE(a.is<std::string>());
    EXPECT_FALSE(a.is<ValuelessByExceptionHelper>());

    EXPECT_FALSE(b.is<int>());
    EXPECT_TRUE(b.is<std::string>());
    EXPECT_FALSE(b.is<ValuelessByExceptionHelper>());

    EXPECT_FALSE(c.is<int>());
    EXPECT_FALSE(c.is<std::string>());
    EXPECT_FALSE(c.is<ValuelessByExceptionHelper>());
}

TEST(Variant, As) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    // By type.
    EXPECT_EQ(a.as<int>(), 100);
    EXPECT_THROW(a.as<std::string>(), std::bad_variant_access);
    EXPECT_THROW(a.as<ValuelessByExceptionHelper>(), std::bad_variant_access);

    EXPECT_THROW(b.as<int>(), std::bad_variant_access);
    EXPECT_EQ(b.as<std::string>(), "hello");
    EXPECT_THROW(b.as<ValuelessByExceptionHelper>(), std::bad_variant_access);

    EXPECT_THROW(c.as<int>(), std::bad_variant_access);
    EXPECT_THROW(c.as<std::string>(), std::bad_variant_access);
    EXPECT_THROW(c.as<ValuelessByExceptionHelper>(), std::bad_variant_access);

    // By index.
    EXPECT_EQ(a.as<0>(), 100);
    EXPECT_THROW(a.as<1>(), std::bad_variant_access);
    EXPECT_THROW(a.as<2>(), std::bad_variant_access);

    EXPECT_THROW(b.as<0>(), std::bad_variant_access);
    EXPECT_EQ(b.as<1>(), "hello");
    EXPECT_THROW(b.as<2>(), std::bad_variant_access);

    EXPECT_THROW(c.as<0>(), std::bad_variant_access);
    EXPECT_THROW(c.as<1>(), std::bad_variant_access);
    EXPECT_THROW(c.as<2>(), std::bad_variant_access);
}

TEST(Variant, TryAs) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    // By type.
    EXPECT_EQ(a.tryAs<int>(), &a.as<int>());
    EXPECT_EQ(a.tryAs<std::string>(), nullptr);
    EXPECT_EQ(a.tryAs<ValuelessByExceptionHelper>(), nullptr);

    EXPECT_EQ(b.tryAs<int>(), nullptr);
    EXPECT_EQ(b.tryAs<std::string>(), &b.as<std::string>());
    EXPECT_EQ(b.tryAs<ValuelessByExceptionHelper>(), nullptr);

    EXPECT_EQ(c.tryAs<int>(), nullptr);
    EXPECT_EQ(c.tryAs<std::string>(), nullptr);
    EXPECT_EQ(c.tryAs<ValuelessByExceptionHelper>(), nullptr);

    // By index.
    EXPECT_EQ(a.tryAs<0>(), &a.as<int>());
    EXPECT_EQ(a.tryAs<1>(), nullptr);
    EXPECT_EQ(a.tryAs<2>(), nullptr);

    EXPECT_EQ(b.tryAs<0>(), nullptr);
    EXPECT_EQ(b.tryAs<1>(), &b.as<std::string>());
    EXPECT_EQ(b.tryAs<2>(), nullptr);

    EXPECT_EQ(c.tryAs<0>(), nullptr);
    EXPECT_EQ(c.tryAs<1>(), nullptr);
    EXPECT_EQ(c.tryAs<2>(), nullptr);
}

TEST(Variant, Hash) {
    using _Variant = ym::Variant<int, std::string, ValuelessByExceptionHelper>;

    auto a = _Variant::byType<int>(100);
    auto b = _Variant::byType<std::string>("hello");
    auto c = mkVBE<_Variant>();

    // Method
    EXPECT_EQ(a.hash(), ym::hash(_Variant::Underlying(a)));
    EXPECT_EQ(b.hash(), ym::hash(_Variant::Underlying(b)));
    EXPECT_EQ(c.hash(), ym::hash(_Variant::Underlying(c))); // Valueless-By-Exception

    // std::hash
    EXPECT_EQ(std::hash<_Variant>{}(a), ym::hash(_Variant::Underlying(a)));
    EXPECT_EQ(std::hash<_Variant>{}(b), ym::hash(_Variant::Underlying(b)));
    EXPECT_EQ(std::hash<_Variant>{}(c), ym::hash(_Variant::Underlying(c))); // Valueless-By-Exception
}

TEST(Variant, Fmt) {
    using _Variant = ym::Variant<int, std::string, NonFormattableStruct, ValuelessByExceptionHelper>;

    // Method
    EXPECT_EQ(_Variant::byType<int>(100).fmt(), "100");
    EXPECT_EQ(_Variant::byType<std::string>("hello").fmt(), "hello");
    EXPECT_EQ(_Variant::byType<NonFormattableStruct>().fmt(), "n/a"); // Non-Formattable
    EXPECT_EQ(mkVBE<_Variant>().fmt(), "n/a"); // Valueless-By-Exception

    // std::format
    EXPECT_EQ(std::format("{}", _Variant::byType<int>(100)), "100");
    EXPECT_EQ(std::format("{}", _Variant::byType<std::string>("hello")), "hello");
    EXPECT_EQ(std::format("{}", _Variant::byType<NonFormattableStruct>()), "n/a"); // Non-Formattable
    EXPECT_EQ(std::format("{}", mkVBE<_Variant>()), "n/a"); // Valueless-By-Exception
}

TEST(Variant, Emplace) {
    using _Variant = ym::Variant<int, float, ValuelessByExceptionHelper>;

    _Variant a = -3;
    _Variant b = -3;

    a.emplace<float>(3.14159f);
    b.emplace<1>(3.14159f);

    ASSERT_TRUE(a.is<float>());
    EXPECT_FLOAT_EQ(a.as<float>(), 3.14159f);

    ASSERT_TRUE(b.is<float>());
    EXPECT_FLOAT_EQ(b.as<float>(), 3.14159f);

    // Test that emplace throws correctly when generating valueless-by-exception variant objects.
    EXPECT_THROW(mkVBEToTestEmplaceThrowing<_Variant>(), std::runtime_error);
}

