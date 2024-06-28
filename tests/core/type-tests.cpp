

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/type.h>
#include <yama/core/type-info-structs.h>


using namespace yama::string_literals;


template<typename T, size_t Extent>
static bool span_eq(std::span<T, Extent> a, std::span<T, Extent> b) noexcept {
    return
        a.size() == b.size() &&
        std::equal(a.begin(), a.end(), b.begin());
}


TEST(TypeTests, TypeInstanceCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.refs().size(), 3);
    EXPECT_FALSE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_FALSE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, CopyCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);

    yama::type b(a); // copy ctor

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.refs().size(), 3);
    EXPECT_FALSE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_FALSE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.refs().size(), 3);
    EXPECT_FALSE(b.refs()[0]);
    EXPECT_FALSE(b.refs()[1]);
    EXPECT_FALSE(b.refs()[2]);
    EXPECT_FALSE(b.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(b.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, MoveCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);

    yama::type b(std::move(a)); // move ctor

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.refs().size(), 3);
    EXPECT_FALSE(b.refs()[0]);
    EXPECT_FALSE(b.refs()[1]);
    EXPECT_FALSE(b.refs()[2]);
    EXPECT_FALSE(b.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(b.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, CopyAssign) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_data b_data(
        yama::primitive_info{
            "def"_str,
            {
                "aa"_str,
                "bb"_str,
                "cc"_str,
                "dd"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);
    yama::type_instance b_inst(b_data.fullname(), b_data);

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = a; // copy assign

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.refs().size(), 3);
    EXPECT_FALSE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_FALSE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.refs().size(), 3);
    EXPECT_FALSE(b.refs()[0]);
    EXPECT_FALSE(b.refs()[1]);
    EXPECT_FALSE(b.refs()[2]);
    EXPECT_FALSE(b.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(b.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, MoveAssign) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_data b_data(
        yama::primitive_info{
            "def"_str,
            {
                "aa"_str,
                "bb"_str,
                "cc"_str,
                "dd"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);
    yama::type_instance b_inst(b_data.fullname(), b_data);

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = std::move(a); // move assign

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.refs().size(), 3);
    EXPECT_FALSE(b.refs()[0]);
    EXPECT_FALSE(b.refs()[1]);
    EXPECT_FALSE(b.refs()[2]);
    EXPECT_FALSE(b.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(b.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, Complete_IncompleteType) {
    yama::type_data ref_data(
        yama::primitive_info{
            "ref"_str,
            {},
        });
    yama::type_instance ref_inst(ref_data.fullname(), ref_data);

    yama::type ref(ref_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    a_inst.put_ref(0, ref);
    //a_inst.put_ref(1, ref); <- incomplete
    a_inst.put_ref(2, ref);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
}

TEST(TypeTests, Complete_CompleteType) {
    yama::type_data ref_data(
        yama::primitive_info{
            "ref"_str,
            {},
        });
    yama::type_instance ref_inst(ref_data.fullname(), ref_data);

    yama::type ref(ref_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    a_inst.put_ref(0, ref);
    a_inst.put_ref(1, ref);
    a_inst.put_ref(2, ref);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Complete_CompleteType_ZeroRefSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {}, // <- zero refsyms
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Fullname) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {},
        });
    yama::type_instance a_inst(
        "def"_str, // <- fullname *differs* from a_data.fullname()
        a_data);

    yama::type a(a_inst);


    EXPECT_EQ(a.fullname(), "def"_str);
}

TEST(TypeTests, Refs) {
    yama::type_data ref_data(
        yama::primitive_info{
            "ref"_str,
            {},
        });
    yama::type_instance ref_inst(ref_data.fullname(), ref_data);

    yama::type ref(ref_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    a_inst.put_ref(0, ref);
    //a_inst.put_ref(1, ref); <- incomplete
    a_inst.put_ref(2, ref);

    yama::type a(a_inst);


    EXPECT_EQ(a.refs().size(), 3);

    EXPECT_TRUE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_TRUE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds

    if (a.refs()[0]) EXPECT_EQ(a.refs()[0].value(), ref);
    if (a.refs()[2]) EXPECT_EQ(a.refs()[2].value(), ref);

    EXPECT_TRUE(a.refs().ref(0));
    EXPECT_FALSE(a.refs().ref(1));
    EXPECT_TRUE(a.refs().ref(2));
    EXPECT_FALSE(a.refs().ref(3)); // <- fail due to out-of-bounds

    if (a.refs().ref(0)) EXPECT_EQ(a.refs().ref(0).value(), ref);
    if (a.refs().ref(2)) EXPECT_EQ(a.refs().ref(2).value(), ref);
}

TEST(TypeTests, Refs_ZeroRefSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {}, // <- zero refsyms
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);


    EXPECT_EQ(a.refs().size(), 0);

    EXPECT_FALSE(a.refs()[0]); // <- fail due to out-of-bounds
    EXPECT_FALSE(a.refs().ref(0)); // <- fail due to out-of-bounds
}

TEST(TypeTests, RefSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);


    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, RefSyms_ZeroRefSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {}, // <- zero refsyms
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);


    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>()));
}

TEST(TypeTests, Equality) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {},
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);
    // b_inst is a clone of a_inst, and exists to ensure that yama::type objects
    // of a_inst are not equal to those of b_inst
    yama::type_instance b_inst("def"_str, a_inst);

    yama::type a0(a_inst);
    yama::type a1(a_inst);
    yama::type b(b_inst);


    EXPECT_TRUE(a0 == a0);
    EXPECT_TRUE(a0 == a1);
    EXPECT_FALSE(a0 == b);

    EXPECT_TRUE(a1 == a0);
    EXPECT_TRUE(a1 == a1);
    EXPECT_FALSE(a1 == b);

    EXPECT_FALSE(b == a0);
    EXPECT_FALSE(b == a1);
    EXPECT_TRUE(b == b);

    EXPECT_FALSE(a0 != a0);
    EXPECT_FALSE(a0 != a1);
    EXPECT_TRUE(a0 != b);

    EXPECT_FALSE(a1 != a0);
    EXPECT_FALSE(a1 != a1);
    EXPECT_TRUE(a1 != b);

    EXPECT_TRUE(b != a0);
    EXPECT_TRUE(b != a1);
    EXPECT_FALSE(b != b);
}

TEST(TypeTests, TypeInstance_RegularCtor) {
    // this is literally just a copy of TypeInstanceCtor, but I
    // have this here to test specifically the yama::type_instance
    // side of things, for *completeness*

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.refs().size(), 3);
    EXPECT_FALSE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_FALSE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds
    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));
}

TEST(TypeTests, TypeInstance_CloneCtor) {
    yama::type_data ref_data(
        yama::primitive_info{
            "ref"_str,
            {},
        });
    yama::type_instance ref_inst(ref_data.fullname(), ref_data);

    yama::type ref(ref_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            {
                "a"_str,
                "b"_str,
                "c"_str,
            },
        });
    yama::type_instance a_inst(a_data.fullname(), a_data);

    a_inst.put_ref(0, ref);
    //a_inst.put_ref(1, ref);
    a_inst.put_ref(2, ref);

    yama::type_instance b_inst("def"_str, a_inst); // clone ctor

    yama::type a(a_inst);
    yama::type b(b_inst); // <- gotten from b_inst

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.refs().size(), 3);
    EXPECT_TRUE(a.refs()[0]);
    EXPECT_FALSE(a.refs()[1]);
    EXPECT_TRUE(a.refs()[2]);
    EXPECT_FALSE(a.refs()[3]); // <- fail due to out-of-bounds
    if (a.refs()[0]) EXPECT_EQ(a.refs()[0].value(), ref);
    if (a.refs()[2]) EXPECT_EQ(a.refs()[2].value(), ref);
    EXPECT_TRUE(span_eq(a.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "def"_str); // <- clone uses new fullname
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.refs().size(), 3);
    EXPECT_TRUE(b.refs()[0]);
    EXPECT_FALSE(b.refs()[1]);
    EXPECT_TRUE(b.refs()[2]);
    EXPECT_FALSE(b.refs()[3]); // <- fail due to out-of-bounds
    if (b.refs()[0]) EXPECT_EQ(b.refs()[0].value(), ref);
    if (b.refs()[2]) EXPECT_EQ(b.refs()[2].value(), ref);
    EXPECT_TRUE(span_eq(b.refsyms(), std::span<const yama::str>({ "a"_str, "b"_str, "c"_str })));

    // equality tests already cover this, but just for completeness...

    EXPECT_NE(a, b);
}

