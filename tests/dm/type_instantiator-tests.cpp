

#include <gtest/gtest.h>

#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/type_info.h>
#include <yama/dm/res_db.h>
#include <yama/dm/type_instantiator.h>
#include <yama/debug-impls/stderr_debug.h>


using namespace yama::string_literals;


class TypeInstantiatorTests : public testing::Test {
public:

    yama::dm::res_db<yama::res<yama::type_info>> type_info_db;
    yama::dm::res_db<yama::res<yama::dm::type_instance<std::allocator<void>>>> type_db;
    yama::dm::res_db<yama::res<yama::dm::type_instance<std::allocator<void>>>> type_batch_db;

    std::unique_ptr<yama::dm::type_instantiator<std::allocator<void>>> instant;


protected:

    void SetUp() override final {
        instant = std::make_unique<yama::dm::type_instantiator<std::allocator<void>>>(
            type_info_db, type_db, type_batch_db, 
            std::allocator<void>{},
            std::make_shared<yama::stderr_debug>());
    }

    void TearDown() override final {
        //
    }
};


// IMPORTANT:
//      below, the term 'indirectly referenced type' refers to a type which is
//      not specified in the constant table of the original type queried, but 
//      is nevertheless indirectly a part of its dep graph


TEST_F(TypeInstantiatorTests, Instantiate_EnsureWorksWithAllConstTypes) {
    static_assert(yama::const_types == 7);
    // dep graph:
    //      a <- Int -4         (not a dep)
    //        <- UInt 301       (not a dep)
    //        <- Float 3.14159  (not a dep)
    //        <- Bool true      (not a dep)
    //        <- Char U'y'      (not a dep)
    //        <- b0             (primitive)
    //        <- b1             (function)

    // this test is a catch-all for us testing that instantiate works w/ all
    // constant types *generally*

    // this test covers that instantiate properly handles non-type constants
    // in the constant table

    // this test covers that instantiate properly handles type constants in
    // a general case, w/ the below Instantiate_# tests then testing linking
    // behaviour more thoroughly, but presuming that this test passing means
    // that linking behaviour can be expected to *generalize* across all the
    // different type constant types

    static_assert(yama::const_types == 7);
    const auto a_consts =
        yama::const_table_info()
        .add_int(-4)
        .add_uint(301)
        .add_float(3.14159)
        .add_bool(true)
        .add_char(U'y')
        .add_primitive_type("b0"_str)
        .add_function_type("b1"_str, yama::make_callsig_info({}, 5)); // ie. fn() -> b0
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info b0_info{
        .fullname = "b0"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info b1_info{
        .fullname = "b1"_str,
        .consts = a_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({}, 5), // ie. fn() -> b0
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b0_info));
    type_info_db.push(yama::make_res<yama::type_info>(b1_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 3);

    EXPECT_EQ(type_db.size(), 3);

    const auto a = type_db.pull("a"_str);
    const auto b0 = type_db.pull("b0"_str);
    const auto b1 = type_db.pull("b1"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b0);
    ASSERT_TRUE(b1);

    // for this test we only really care that type 'a' instantiated correctly

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);

    // assert expected types

    const yama::type aa(**a);
    const yama::type bb0(**b0);
    const yama::type bb1(**b1);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), yama::const_types);

    static_assert(yama::const_types == 7);
    EXPECT_EQ(aa.consts().get<yama::int_const>(0), std::make_optional(-4));
    EXPECT_EQ(aa.consts().get<yama::uint_const>(1), std::make_optional(301));
    EXPECT_EQ(aa.consts().get<yama::float_const>(2), std::make_optional(3.14159));
    EXPECT_EQ(aa.consts().get<yama::bool_const>(3), std::make_optional(true));
    EXPECT_EQ(aa.consts().get<yama::char_const>(4), std::make_optional(U'y'));
    EXPECT_EQ(aa.consts().get<yama::primitive_type_const>(5), std::make_optional(bb0));
    EXPECT_EQ(aa.consts().get<yama::function_type_const>(6), std::make_optional(bb1));
}

TEST_F(TypeInstantiatorTests, Instantiate_WithNoDeps) {
    // dep graph:
    //      a

    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 1);

    EXPECT_EQ(type_db.size(), 1);

    const auto a = type_db.pull("a"_str);

    ASSERT_TRUE(a);

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);

    // assert expected types

    const yama::type aa(**a);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_WithDeps) {
    // dep graph:
    //      a -> b
    //        -> c

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 3);

    EXPECT_EQ(type_db.size(), 3);

    const auto a = type_db.pull("a"_str);
    const auto b = type_db.pull("b"_str);
    const auto c = type_db.pull("c"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);
    EXPECT_EQ(b->get()->fullname(), "b"_str);
    EXPECT_EQ(c->get()->fullname(), "c"_str);

    // assert expected types

    const yama::type aa(**a);
    const yama::type bb(**b);
    const yama::type cc(**c);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 0);

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_WithDeps_WithIndirectlyReferencedTypes) {
    // dep graph:
    //      a -> b -> d
    //             -> e
    //        -> c -> f

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("d"_str)
        .add_primitive_type("e"_str);
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto c_consts =
        yama::const_table_info()
        .add_primitive_type("f"_str);
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = c_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info d_info{
        .fullname = "d"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info e_info{
        .fullname = "e"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));
    type_info_db.push(yama::make_res<yama::type_info>(d_info));
    type_info_db.push(yama::make_res<yama::type_info>(e_info));
    type_info_db.push(yama::make_res<yama::type_info>(f_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 6);

    EXPECT_EQ(type_db.size(), 6);

    const auto a = type_db.pull("a"_str);
    const auto b = type_db.pull("b"_str);
    const auto c = type_db.pull("c"_str);
    const auto d = type_db.pull("d"_str);
    const auto e = type_db.pull("e"_str);
    const auto f = type_db.pull("f"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);
    ASSERT_TRUE(e);
    ASSERT_TRUE(f);

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);
    EXPECT_EQ(b->get()->fullname(), "b"_str);
    EXPECT_EQ(c->get()->fullname(), "c"_str);
    EXPECT_EQ(d->get()->fullname(), "d"_str);
    EXPECT_EQ(e->get()->fullname(), "e"_str);
    EXPECT_EQ(f->get()->fullname(), "f"_str);

    // assert expected types

    const yama::type aa(**a);
    const yama::type bb(**b);
    const yama::type cc(**c);
    const yama::type dd(**d);
    const yama::type ee(**e);
    const yama::type ff(**f);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 2);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(bb.consts().type(1), std::make_optional(ee));

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 1);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(ff));

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);

    EXPECT_TRUE(ee.complete());
    EXPECT_EQ(ee.fullname(), "e"_str);
    EXPECT_EQ(ee.kind(), yama::kind::primitive);
    EXPECT_EQ(ee.consts().size(), 0);

    EXPECT_TRUE(ff.complete());
    EXPECT_EQ(ff.fullname(), "f"_str);
    EXPECT_EQ(ff.kind(), yama::kind::primitive);
    EXPECT_EQ(ff.consts().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_WithDeps_TypeReferencedMultipleTimesInAcyclicDepGraph) {
    // dep graph:
    //      a -> b -> d
    //        -> c -> d
    //             -> d     <- test w/ multiple refs to 'd' on same 'c' node in graph
    //             -> d

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("d"_str);
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto c_consts =
        yama::const_table_info()
        .add_primitive_type("d"_str)
        .add_primitive_type("d"_str)
        .add_primitive_type("d"_str);
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = c_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info d_info{
        .fullname = "d"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));
    type_info_db.push(yama::make_res<yama::type_info>(d_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 4);

    EXPECT_EQ(type_db.size(), 4);

    const auto a = type_db.pull("a"_str);
    const auto b = type_db.pull("b"_str);
    const auto c = type_db.pull("c"_str);
    const auto d = type_db.pull("d"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_TRUE(c);
    ASSERT_TRUE(d);

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);
    EXPECT_EQ(b->get()->fullname(), "b"_str);
    EXPECT_EQ(c->get()->fullname(), "c"_str);
    EXPECT_EQ(d->get()->fullname(), "d"_str);

    // assert expected types

    const yama::type aa(**a);
    const yama::type bb(**b);
    const yama::type cc(**c);
    const yama::type dd(**d);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(cc));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(dd));

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.consts().size(), 3);
    EXPECT_EQ(cc.consts().type(0), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(1), std::make_optional(dd));
    EXPECT_EQ(cc.consts().type(2), std::make_optional(dd));

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.consts().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_WithDeps_DepGraphCycle) {
    // dep graph:
    //      a -> b -> a     (back ref)
    //        -> a          (back ref)

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("a"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str);
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));

    const size_t result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 2);

    EXPECT_EQ(type_db.size(), 2);

    const auto a = type_db.pull("a"_str);
    const auto b = type_db.pull("b"_str);

    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    // assert expected type_instance fullnames

    EXPECT_EQ(a->get()->fullname(), "a"_str);
    EXPECT_EQ(b->get()->fullname(), "b"_str);

    // assert expected types

    const yama::type aa(**a);
    const yama::type bb(**b);

    EXPECT_TRUE(aa.complete());
    EXPECT_EQ(aa.fullname(), "a"_str);
    EXPECT_EQ(aa.kind(), yama::kind::primitive);
    EXPECT_EQ(aa.consts().size(), 2);
    EXPECT_EQ(aa.consts().type(0), std::make_optional(bb));
    EXPECT_EQ(aa.consts().type(1), std::make_optional(aa));

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.consts().size(), 1);
    EXPECT_EQ(bb.consts().type(0), std::make_optional(aa));
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToOriginalTypeAlreadyInstantiated) {
    // dep graph:
    //      a

    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));

    // instantiate 'a' up-front, then ensure instantiating 'a' again fails
    // as it's already been instantiated

    ASSERT_TRUE(instant->instantiate("a"_str));

    EXPECT_EQ(type_db.size(), 1);
    ASSERT_TRUE(type_db.exists("a"_str));

    const auto result = instant->instantiate("a"_str);

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 1);
    EXPECT_TRUE(type_db.exists("a"_str));
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToOriginalTypeNotFound) {
    // dep graph:
    //      a           <- error is that no type_data 'a' provided

    //type_data_db.push(yama::type_data(yama::primitive_info{ "a"_str }));
    
    const auto result = instant->instantiate("a"_str); // <- will fail due to no 'a'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToReferencedTypeNotFound) {
    // dep graph:
    //      a -> b      <- error is that no type_data 'b' provided
    //        -> c

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    //yama::type_info b_info{
    //    .fullname = "b"_str,
    //    .consts = {},
    //    .info = yama::primitive_info{
    //        .ptype = yama::ptype::bool0,
    //    },
    //};
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    //type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));

    const auto result = instant->instantiate("a"_str); // <- will fail due to no 'b'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToIndirectlyReferencedTypeNotFound) {
    // dep graph:
    //      a -> b -> d     <- error is that no type_data 'd' provided
    //             -> e
    //        -> c -> f

    const auto a_consts =
        yama::const_table_info()
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("d"_str)
        .add_primitive_type("e"_str);
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto c_consts =
        yama::const_table_info()
        .add_primitive_type("f"_str);
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = c_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    //yama::type_info d_info{
    //    .fullname = "d"_str,
    //    .consts = {},
    //    .info = yama::primitive_info{
    //        .ptype = yama::ptype::bool0,
    //    },
    //};
    yama::type_info e_info{
        .fullname = "e"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info f_info{
        .fullname = "f"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));
    //type_info_db.push(yama::make_res<yama::type_info>(d_info));
    type_info_db.push(yama::make_res<yama::type_info>(e_info));
    type_info_db.push(yama::make_res<yama::type_info>(f_info));

    const auto result = instant->instantiate("a"_str); // <- will fail due to no 'd'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToKindMismatch) {
    // dep graph:
    //      a -> b -> c

    // a's b symbol's kind is primitive
    // b's kind is function

    const auto a_consts =
        yama::const_table_info()
        // kind is intended to mismatch w/ the indirectly referenced type b
        .add_primitive_type("b"_str);
    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("c"_str);
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));

    const auto result = instant->instantiate("a"_str); // <- will fail due to kind mismatch between 'a' and 'b'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

// TODO: maybe decompose below test into a few tests for each of the following cases:
//          1) differ by param count
//          2) differ by param type indices (same const table)
//          2) differ by return type indices (same const table)
//          3) differ by const table (but have otherwise identical callsigs)

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToCallSigMismatch) {
    // dep graph:
    //      a -> b -> c

    // a's b function type ref symbol's callsig is 'fn(b) -> b'
    // b's callsig is 'fn(c) -> c'

    // TODO: while clever, after re-reading this unit test code, I think we'd
    //       do well to rewrite this and other unit tests herein to be a lot
    //       easier to read, as right now it's a bit *confusing* to me

    // IMPORTANT:
    //      I actually really like the below *cleverness*, as it actually
    //      caught me during the initial impl, w/ me not otherwise noticing
    //      the subtle error in my code w/out it pointing it out to me

    // the thing that makes this test a bit *clever* is that the two
    // callsigs are actually identical in terms of their index numbers,
    // taking the form 'fn( *0* ) -> *0*', and the thing that makes them
    // different is the INTERPRETATION of these indices, as each of the
    // two respective callsigs' indices refer to different constant tables

    // put another way, type 'a' will use clever_callsig_info_for_test w/
    // a_consts, while 'b' will use clever_callsig_info_for_test w/
    // b_consts, w/ the different #_consts used by each causing
    // the above bout of *cleverness*
    const auto clever_callsig_info_for_test = yama::make_callsig_info({ 0 }, 0);

    const auto a_consts =
        yama::const_table_info()
        // callsig is intended to mismatch w/ the indirectly referenced type b
        .add_function_type("b"_str, clever_callsig_info_for_test);
    const auto b_consts =
        yama::const_table_info()
        .add_primitive_type("c"_str); // <- c exists for b's constant indices to differ from a's

    yama::type_info a_info{
        .fullname = "a"_str,
        .consts = a_consts,
        .info = yama::function_info{
            .callsig = clever_callsig_info_for_test,
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .consts = b_consts,
        .info = yama::function_info{
            .callsig = clever_callsig_info_for_test,
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    type_info_db.push(yama::make_res<yama::type_info>(a_info));
    type_info_db.push(yama::make_res<yama::type_info>(b_info));
    type_info_db.push(yama::make_res<yama::type_info>(c_info));

    const auto result = instant->instantiate("a"_str); // <- will fail due to callsig mismatch between 'a' and 'b'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

