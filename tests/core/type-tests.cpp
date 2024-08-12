

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
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, CopyCtor) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    yama::type b(a); // copy ctor

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, MoveCtor) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    yama::type b(std::move(a)); // move ctor

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, CopyAssign) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::type_info b_info{
        .fullname = "def"_str,
        .linksyms = linksyms1,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
    yama::dm::type_instance b_inst(std::allocator<void>(), b_info.fullname, yama::make_res<yama::type_info>(b_info));

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = a; // copy assign

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, MoveAssign) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::type_info b_info{
        .fullname = "def"_str,
        .linksyms = linksyms1,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
    yama::dm::type_instance b_inst(std::allocator<void>(), b_info.fullname, yama::make_res<yama::type_info>(b_info));

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = std::move(a); // move assign

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.links().size(), 3);
    EXPECT_FALSE(b.links()[0]);
    EXPECT_FALSE(b.links()[1]);
    EXPECT_FALSE(b.links()[2]);
    EXPECT_FALSE(b.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, Complete_IncompleteType) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put_link(0, link);
    //a_inst.put_link(1, link); <- incomplete
    a_inst.put_link(2, link);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
}

TEST(TypeTests, Complete_CompleteType) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put_link(0, link);
    a_inst.put_link(1, link);
    a_inst.put_link(2, link);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Complete_CompleteType_ZeroLinkSyms) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {}, // <- zero linksyms
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Fullname) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), 
        "def"_str, // <- fullname *differs* from a_info.fullname
        yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);


    EXPECT_EQ(a.fullname(), "def"_str);
}

TEST(TypeTests, PType) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::uint));
}

TEST(TypeTests, PType_NoPTypeForNonPrimitiveTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::nullopt);
}

TEST(TypeTests, CallFn) {
    auto cf = [](yama::context&, yama::links_view) {};

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = cf,
            .locals = 10,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.call_fn(), std::make_optional(cf));
}

TEST(TypeTests, CallFn_NoCallFnForNonCallableTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.call_fn(), std::nullopt);
}

TEST(TypeTests, MaxLocals) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.locals(), 10);
}

TEST(TypeTests, MaxLocals_ZeroForNonCallableTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.locals(), 0);
}

TEST(TypeTests, Links) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

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
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {}, // <- zero linksyms
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);


    EXPECT_EQ(a.links().size(), 0);

    EXPECT_FALSE(a.links()[0]); // <- fail due to out-of-bounds
    EXPECT_FALSE(a.links().link(0)); // <- fail due to out-of-bounds
}

TEST(TypeTests, Equality) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
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

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, TypeInstance_CloneCtor) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put_link(0, link);
    //a_inst.put_link(1, link);
    a_inst.put_link(2, link);

    yama::dm::type_instance b_inst(std::allocator<void>(), "def"_str, a_inst); // clone ctor

    yama::type a(a_inst);
    yama::type b(b_inst); // <- gotten from b_inst

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
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
    yama::type_info link_info{
        .fullname = "link"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

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

// per-kind tests

static_assert(yama::kinds == 2);

TEST(TypeTests, Primitive) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.callsig(), std::nullopt);
    EXPECT_EQ(a.call_fn(), std::nullopt);
    EXPECT_EQ(a.locals(), 0);
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

TEST(TypeTests, Function) {
    auto a_call_fn = [](yama::context&, yama::links_view) {};
    auto a_callsig = yama::make_callsig_info({ 0, 1, 2 }, 1);
    yama::type_info a_info{
        .fullname = "abc"_str,
        .linksyms = linksyms0,
        .info = yama::function_info{
            .callsig = a_callsig,
            .call_fn = a_call_fn,
            .locals = 17,
        },
    };
    yama::dm::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::function);
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(a_callsig, a.links())));
    EXPECT_EQ(a.call_fn(), std::make_optional(a_call_fn));
    EXPECT_EQ(a.locals(), 17);
    EXPECT_EQ(a.links().size(), 3);
    EXPECT_FALSE(a.links()[0]);
    EXPECT_FALSE(a.links()[1]);
    EXPECT_FALSE(a.links()[2]);
    EXPECT_FALSE(a.links()[3]); // <- fail due to out-of-bounds
}

