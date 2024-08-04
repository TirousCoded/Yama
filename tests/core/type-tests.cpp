

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/type_info.h>
#include <yama/core/type.h>
#include <yama/dm/type_instance.h>


using namespace yama::string_literals;


// just some data that appears in our tests over-and-over

static std::vector<yama::linksym> linksyms0{
    yama::make_linksym("a"_str, yama::kind::primitive),
    yama::make_linksym("b"_str, yama::kind::primitive),
    yama::make_linksym("c"_str, yama::kind::primitive),
};

static std::vector<yama::linksym> linksyms1{
    yama::make_linksym("aa"_str, yama::kind::primitive),
    yama::make_linksym("bb"_str, yama::kind::primitive),
    yama::make_linksym("cc"_str, yama::kind::primitive),
    yama::make_linksym("dd"_str, yama::kind::primitive),
};

static yama::callsig_info callsig_info0 = yama::make_callsig_info({ 0, 1, 2 }, 1);

static yama::callsig_info callsig_info1 = yama::make_callsig_info({ 2, 0, 1 }, 0);


TEST(TypeTests, TypeInstanceCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(callsig_info0, a.links())));
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, CopyCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    yama::type b(a); // copy ctor

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(callsig_info0, a.links())));
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.callsig(), std::make_optional(yama::callsig(callsig_info0, b.links())));
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, MoveCtor) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    yama::type b(std::move(a)); // move ctor

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.callsig(), std::make_optional(yama::callsig(callsig_info0, b.links())));
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, CopyAssign) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::type_data b_data(
        yama::primitive_info{
            "def"_str,
            std::make_optional(callsig_info1),
            linksyms1,
            yama::ptype::bool0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);
    yama::dm::type_instance b_inst(std::allocator<void>(), b_data.fullname(), b_data);

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = a; // copy assign

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(callsig_info0, a.links())));
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.callsig(), std::make_optional(yama::callsig(callsig_info0, b.links())));
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, MoveAssign) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::type_data b_data(
        yama::primitive_info{
            "def"_str,
            std::make_optional(callsig_info1),
            linksyms1,
            yama::ptype::bool0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);
    yama::dm::type_instance b_inst(std::allocator<void>(), b_data.fullname(), b_data);

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = std::move(a); // move assign

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.callsig(), std::make_optional(yama::callsig(callsig_info0, b.links())));
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, Complete_IncompleteType) {
    yama::type_data link_data(
        yama::primitive_info{
            "link"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance link_inst(std::allocator<void>(), link_data.fullname(), link_data);

    yama::type link(link_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    a_inst.put_link(0, link);
    //a_inst.put_link(1, link); <- incomplete
    a_inst.put_link(2, link);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
}

TEST(TypeTests, Complete_CompleteType) {
    yama::type_data link_data(
        yama::primitive_info{
            "link"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance link_inst(std::allocator<void>(), link_data.fullname(), link_data);

    yama::type link(link_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    a_inst.put_link(0, link);
    a_inst.put_link(1, link);
    a_inst.put_link(2, link);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Complete_CompleteType_ZeroLinkSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {}, // <- zero linksyms
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Fullname) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), 
        "def"_str, // <- fullname *differs* from a_data.fullname()
        a_data);

    yama::type a(a_inst);


    EXPECT_EQ(a.fullname(), "def"_str);
}

TEST(TypeTests, PType) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::uint,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::uint));
}

TEST(TypeTests, PType_NoPTypeForNonPrimitiveTypes) {
    yama::type_data a_data(
        yama::function_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::noop_call_fn,
            10,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::nullopt);
}

TEST(TypeTests, MaxLocals) {
    yama::type_data a_data(
        yama::function_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::noop_call_fn,
            10,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_EQ(a.max_locals(), 10);
}

TEST(TypeTests, MaxLocals_ZeroForNonFunctionTypes) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::uint,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_EQ(a.max_locals(), 0);
}

TEST(TypeTests, Links) {
    yama::type_data link_data(
        yama::primitive_info{
            "link"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance link_inst(std::allocator<void>(), link_data.fullname(), link_data);

    yama::type link(link_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    a_inst.put_link(0, link);
    //a_inst.put_link(1, link); <- incomplete
    a_inst.put_link(2, link);

    yama::type a(a_inst);


    EXPECT_EQ(a.links().size(), 3);

    EXPECT_TRUE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_TRUE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds

    if (a.links()[0]) EXPECT_EQ(a.links()[0].value(), link);
    if (a.links()[2]) EXPECT_EQ(a.links()[2].value(), link);

    EXPECT_TRUE(a.links().link(0));
    EXPECT_FALSE(a.links().link(1));
    EXPECT_TRUE(a.links().link(2));
    EXPECT_FALSE(a.links().link(3)); // <- fail due to out-of-bounds

    if (a.links().link(0)) EXPECT_EQ(a.links().link(0).value(), link);
    if (a.links().link(2)) EXPECT_EQ(a.links().link(2).value(), link);
}

TEST(TypeTests, Links_ZeroLinkSyms) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {}, // <- zero linksyms
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);


    EXPECT_EQ(a.links().size(), 0);

    EXPECT_FALSE(a.links()[0]); // <- fail due to out-of-bounds
    EXPECT_FALSE(a.links().link(0)); // <- fail due to out-of-bounds
}

TEST(TypeTests, Equality) {
    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);
    // b_inst is a clone of a_inst, and exists to ensure that yama::type objects
    // of a_inst are not equal to those of b_inst
    yama::dm::type_instance b_inst(std::allocator<void>(), "def"_str, a_inst);

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
    // have this here to test specifically the yama::dm::type_instance
    // side of things, for *completeness*

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(callsig_info0, a.links())));
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, TypeInstance_CloneCtor) {
    yama::type_data link_data(
        yama::primitive_info{
            "link"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance link_inst(std::allocator<void>(), link_data.fullname(), link_data);

    yama::type link(link_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    a_inst.put_link(0, link);
    //a_inst.put_link(1, link);
    a_inst.put_link(2, link);

    yama::dm::type_instance b_inst(std::allocator<void>(), "def"_str, a_inst); // clone ctor

    yama::type a(a_inst);
    yama::type b(b_inst); // <- gotten from b_inst

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(callsig_info0, a.links())));
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_TRUE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_TRUE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
    if (a.links()[0]) EXPECT_EQ(a.links()[0].value(), link);
    if (a.links()[2]) EXPECT_EQ(a.links()[2].value(), link);

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "def"_str); // <- clone uses new fullname
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.callsig(), std::make_optional(yama::callsig(callsig_info0, b.links())));
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_TRUE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_TRUE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
    if (b.links()[0]) EXPECT_EQ(b.links()[0].value(), link);
    if (b.links()[2]) EXPECT_EQ(b.links()[2].value(), link);

    // equality tests already cover this, but just for completeness...

    EXPECT_NE(a, b);
}

TEST(TypeTests, TypeInstance_MutationsToTypeInstanceAreVisibleToType) {
    yama::type_data link_data(
        yama::primitive_info{
            "link"_str,
            std::make_optional(callsig_info0),
            {},
            yama::ptype::float0,
        });
    yama::dm::type_instance link_inst(std::allocator<void>(), link_data.fullname(), link_data);

    yama::type link(link_inst);

    yama::type_data a_data(
        yama::primitive_info{
            "abc"_str,
            std::make_optional(callsig_info0),
            linksyms0,
            yama::ptype::float0,
        });
    yama::dm::type_instance a_inst(std::allocator<void>(), a_data.fullname(), a_data);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);

    a_inst.put_link(0, link);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_TRUE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    if (a.links()[0]) EXPECT_EQ(a.links()[0].value(), link);
    
    a_inst.put_link(2, link);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_TRUE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_TRUE(a.links()[2]);
    if (a.links()[0]) EXPECT_EQ(a.links()[0].value(), link);
    if (a.links()[2]) EXPECT_EQ(a.links()[2].value(), link);
    
    a_inst.put_link(1, link);

    EXPECT_TRUE(a.complete());
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_TRUE(a.links()[0]);
    EXPECT_TRUE(a.links()[1]);
    EXPECT_TRUE(a.links()[2]);
    if (a.links()[0]) EXPECT_EQ(a.links()[0].value(), link);
    if (a.links()[1]) EXPECT_EQ(a.links()[1].value(), link);
    if (a.links()[2]) EXPECT_EQ(a.links()[2].value(), link);
}

