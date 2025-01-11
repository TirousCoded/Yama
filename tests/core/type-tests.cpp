

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table.h>
#include <yama/core/type_info.h>
#include <yama/core/type.h>

// TODO: type_instance used to be in frontend, but now it isn't,
//       meaning our unit tests have backend code dependence,
//       which is undesirable
//
//       I'm thinking maybe pull type_instance back to the frontend
//       at some point
#include <yama/internals/type_instance.h>


using namespace yama::string_literals;


// just some data that appears in our tests over-and-over

static const auto consts0 =
    yama::const_table_info()
    .add_primitive_type("a"_str)
    .add_primitive_type("b"_str)
    .add_primitive_type("c"_str);
static const auto consts1 =
    yama::const_table_info()
    .add_primitive_type("aa"_str)
    .add_primitive_type("bb"_str)
    .add_primitive_type("cc"_str)
    .add_primitive_type("dd"_str);

static yama::callsig_info callsig_info0 = yama::make_callsig_info({ 0, 1, 2 }, 1);
static yama::callsig_info callsig_info1 = yama::make_callsig_info({ 2, 0, 1 }, 0);


TEST(TypeTests, TypeInstanceCtor) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));
    EXPECT_EQ(a.consts().size(), 3);
    EXPECT_TRUE(a.consts().is_stub(0));
    EXPECT_TRUE(a.consts().is_stub(1));
    EXPECT_TRUE(a.consts().is_stub(2));
}

TEST(TypeTests, CopyCtor) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    yama::type b(a); // copy ctor

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, MoveCtor) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    yama::type b(std::move(a)); // move ctor

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, CopyAssign) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::type_info b_info{
        .fullname = "def"_str,
        .consts = consts1,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
    yama::internal::type_instance b_inst(std::allocator<void>(), b_info.fullname, yama::make_res<yama::type_info>(b_info));

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = a; // copy assign

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, MoveAssign) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::type_info b_info{
        .fullname = "def"_str,
        .consts = consts1,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
    yama::internal::type_instance b_inst(std::allocator<void>(), b_info.fullname, yama::make_res<yama::type_info>(b_info));

    yama::type a(a_inst);
    yama::type b(b_inst);

    b = std::move(a); // move assign

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "abc"_str);
    EXPECT_EQ(b.kind(), yama::kind::primitive);
    EXPECT_EQ(b.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(b.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, Complete_IncompleteType) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put<yama::primitive_type_const>(0, link);
    //a_inst.put<yama::primitive_type_const>(1, link); <- incomplete
    a_inst.put<yama::primitive_type_const>(2, link);

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
}

TEST(TypeTests, Complete_CompleteType) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put<yama::primitive_type_const>(0, link);
    a_inst.put<yama::primitive_type_const>(1, link);
    a_inst.put<yama::primitive_type_const>(2, link);

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Complete_CompleteType_ZeroLinkSyms) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {}, // <- zero linksyms
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_TRUE(a.complete());
}

TEST(TypeTests, Fullname) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), 
        "def"_str, // <- fullname *differs* from a_info.fullname
        yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);


    EXPECT_EQ(a.fullname(), "def"_str);
}

TEST(TypeTests, PType) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::uint));
}

TEST(TypeTests, PType_NoPTypeForNonPrimitiveTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.ptype(), std::nullopt);
}

TEST(TypeTests, CallFn) {
    auto cf = [](yama::context&) {};

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = cf,
            .max_locals = 10,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.call_fn(), std::make_optional(cf));
}

TEST(TypeTests, CallFn_NoCallFnForNonCallableTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.call_fn(), std::nullopt);
}

TEST(TypeTests, MaxLocals) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::function_info{
            .callsig = callsig_info0,
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.max_locals(), 10);
}

TEST(TypeTests, MaxLocals_ZeroForNonCallableTypes) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::uint,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_EQ(a.max_locals(), 0);
}

TEST(TypeTests, Consts) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put<yama::primitive_type_const>(0, link);
    //a_inst.put<yama::primitive_type_const>(1, link); <- incomplete
    a_inst.put<yama::primitive_type_const>(2, link);

    yama::type a(a_inst);

    EXPECT_EQ(a.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, Equality) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));
    // b_inst is a clone of a_inst, and exists to ensure that yama::type objects
    // of a_inst are not equal to those of b_inst
    yama::internal::type_instance b_inst(std::allocator<void>(), "def"_str, a_inst);

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
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));
    EXPECT_EQ(a.consts().size(), 3);
    EXPECT_TRUE(a.consts().is_stub(0));
    EXPECT_TRUE(a.consts().is_stub(1));
    EXPECT_TRUE(a.consts().is_stub(2));
}

TEST(TypeTests, TypeInstance_CloneCtor) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    a_inst.put<yama::primitive_type_const>(0, link);
    //a_inst.put<yama::primitive_type_const>(1, link);
    a_inst.put<yama::primitive_type_const>(2, link);

    yama::internal::type_instance b_inst(std::allocator<void>(), "def"_str, a_inst); // clone ctor

    yama::type a(a_inst);
    yama::type b(b_inst); // <- gotten from b_inst

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), std::make_optional(yama::ptype::float0));
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));

    EXPECT_FALSE(b.complete());
    EXPECT_EQ(b.fullname(), "def"_str); // <- clone uses new fullname
    EXPECT_EQ(b.kind(), a.kind());
    EXPECT_EQ(b.ptype(), a.ptype());
    
    EXPECT_EQ(b.consts().size(), a.consts().size());

    EXPECT_EQ(b.consts().type(0), std::make_optional(link));
    EXPECT_EQ(a.consts().type(0), std::make_optional(link));
    
    EXPECT_TRUE(b.consts().is_stub(1));
    EXPECT_TRUE(a.consts().is_stub(1));
    
    EXPECT_EQ(b.consts().type(2), std::make_optional(link));
    EXPECT_EQ(a.consts().type(2), std::make_optional(link));

    // equality tests already cover this, but just for completeness...

    EXPECT_NE(a, b);
}

TEST(TypeTests, TypeInstance_MutationsToTypeInstanceAreVisibleToTypeAndConstTable) {
    yama::type_info link_info{
        .fullname = "link"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance link_inst(std::allocator<void>(), link_info.fullname, yama::make_res<yama::type_info>(link_info));

    yama::type link(link_inst);

    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type t(a_inst);           // yama::type can see changes
    yama::const_table ct(a_inst);   // yama::const_table can see changes

    EXPECT_FALSE(t.complete());
    EXPECT_FALSE(t.consts().complete());
    EXPECT_FALSE(ct.complete());
    EXPECT_EQ(t.consts(), yama::const_table(a_inst));
    EXPECT_EQ(ct, yama::const_table(a_inst));

    a_inst.put<yama::primitive_type_const>(0, link);

    EXPECT_FALSE(t.complete());
    EXPECT_FALSE(t.consts().complete());
    EXPECT_FALSE(ct.complete());
    EXPECT_EQ(t.consts(), yama::const_table(a_inst));
    EXPECT_EQ(ct, yama::const_table(a_inst));
    
    a_inst.put<yama::primitive_type_const>(2, link);

    EXPECT_FALSE(t.complete());
    EXPECT_FALSE(t.consts().complete());
    EXPECT_FALSE(ct.complete());
    EXPECT_EQ(t.consts(), yama::const_table(a_inst));
    EXPECT_EQ(ct, yama::const_table(a_inst));
    
    a_inst.put<yama::primitive_type_const>(1, link);

    EXPECT_TRUE(t.complete());
    EXPECT_TRUE(t.consts().complete());
    EXPECT_TRUE(ct.complete());
    EXPECT_EQ(t.consts(), yama::const_table(a_inst));
    EXPECT_EQ(ct, yama::const_table(a_inst));
}

// per-kind tests

static_assert(yama::kinds == 2);

TEST(TypeTests, Primitive) {
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::primitive_info{
            .ptype = yama::ptype::float0,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::primitive);
    EXPECT_EQ(a.ptype(), yama::ptype::float0);
    EXPECT_EQ(a.callsig(), std::nullopt);
    EXPECT_EQ(a.call_fn(), std::nullopt);
    EXPECT_EQ(a.max_locals(), 0);
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));
}

TEST(TypeTests, Function) {
    auto a_call_fn = [](yama::context&) {};
    auto a_callsig = yama::make_callsig_info({ 0, 1, 2 }, 1);
    yama::type_info a_info{
        .fullname = "abc"_str,
        .consts = consts0,
        .info = yama::function_info{
            .callsig = a_callsig,
            .call_fn = a_call_fn,
            .max_locals = 17,
        },
    };
    yama::internal::type_instance a_inst(std::allocator<void>(), a_info.fullname, yama::make_res<yama::type_info>(a_info));

    yama::type a(a_inst);

    EXPECT_FALSE(a.complete());
    EXPECT_EQ(a.fullname(), "abc"_str);
    EXPECT_EQ(a.kind(), yama::kind::function);
    EXPECT_EQ(a.ptype(), std::nullopt);
    EXPECT_EQ(a.callsig(), std::make_optional(yama::callsig(a_callsig, a.consts())));
    EXPECT_EQ(a.call_fn(), std::make_optional(a_call_fn));
    EXPECT_EQ(a.max_locals(), 17);
    EXPECT_EQ(a.consts(), yama::const_table(a_inst));
}

