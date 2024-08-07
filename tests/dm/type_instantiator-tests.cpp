

#include <gtest/gtest.h>

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
//      below, the term 'indirectly linked type' refers to a type which is
//      not specified in the link symbol table of the original type queried,
//      but is nevertheless indirectly queried as part of the original query,
//      due to it indirectly being depended upon


TEST_F(TypeInstantiatorTests, Instantiate_NoLinks) {
    // link graph:
    //      a

    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = {},
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
    EXPECT_EQ(aa.links().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_Links) {
    // link graph:
    //      a -> b
    //        -> c

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = {},
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
    EXPECT_EQ(aa.links().size(), 2);

    if (aa.links()[0]) EXPECT_EQ(*(aa.links()[0]), bb);
    if (aa.links()[1]) EXPECT_EQ(*(aa.links()[1]), cc);

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.links().size(), 0);

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.links().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_Links_WithIndirectlyLinkedTypes) {
    // link graph:
    //      a -> b -> d
    //             -> e
    //        -> c -> f

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("d"_str, yama::kind::primitive),
        yama::make_linksym("e"_str, yama::kind::primitive),
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> c_linksyms{
        yama::make_linksym("f"_str, yama::kind::primitive),
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = c_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info d_info{
        .fullname = "d"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info e_info{
        .fullname = "e"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info f_info{
        .fullname = "f"_str,
        .linksyms = {},
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
    EXPECT_EQ(aa.links().size(), 2);

    if (aa.links()[0]) EXPECT_EQ(*(aa.links()[0]), bb);
    if (aa.links()[1]) EXPECT_EQ(*(aa.links()[1]), cc);

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.links().size(), 2);

    if (bb.links()[0]) EXPECT_EQ(*(bb.links()[0]), dd);
    if (bb.links()[1]) EXPECT_EQ(*(bb.links()[1]), ee);

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.links().size(), 1);

    if (cc.links()[0]) EXPECT_EQ(*(cc.links()[0]), ff);

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.links().size(), 0);

    EXPECT_TRUE(ee.complete());
    EXPECT_EQ(ee.fullname(), "e"_str);
    EXPECT_EQ(ee.kind(), yama::kind::primitive);
    EXPECT_EQ(ee.links().size(), 0);

    EXPECT_TRUE(ff.complete());
    EXPECT_EQ(ff.fullname(), "f"_str);
    EXPECT_EQ(ff.kind(), yama::kind::primitive);
    EXPECT_EQ(ff.links().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_Links_TypeLinkedMultipleTimesInAcyclicLinkGraph) {
    // link graph:
    //      a -> b -> d
    //        -> c -> d
    //             -> d     <- test w/ multiple links to 'd' on same 'c' node in graph
    //             -> d

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("d"_str, yama::kind::primitive),
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> c_linksyms{
        yama::make_linksym("d"_str, yama::kind::primitive),
        yama::make_linksym("d"_str, yama::kind::primitive),
        yama::make_linksym("d"_str, yama::kind::primitive),
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = c_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info d_info{
        .fullname = "d"_str,
        .linksyms = {},
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
    EXPECT_EQ(aa.links().size(), 2);

    if (aa.links()[0]) EXPECT_EQ(*(aa.links()[0]), bb);
    if (aa.links()[1]) EXPECT_EQ(*(aa.links()[1]), cc);

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.links().size(), 1);

    if (bb.links()[0]) EXPECT_EQ(*(bb.links()[0]), dd);

    EXPECT_TRUE(cc.complete());
    EXPECT_EQ(cc.fullname(), "c"_str);
    EXPECT_EQ(cc.kind(), yama::kind::primitive);
    EXPECT_EQ(cc.links().size(), 3);

    if (cc.links()[0]) EXPECT_EQ(*(cc.links()[0]), dd);
    if (cc.links()[1]) EXPECT_EQ(*(cc.links()[1]), dd);
    if (cc.links()[2]) EXPECT_EQ(*(cc.links()[2]), dd);

    EXPECT_TRUE(dd.complete());
    EXPECT_EQ(dd.fullname(), "d"_str);
    EXPECT_EQ(dd.kind(), yama::kind::primitive);
    EXPECT_EQ(dd.links().size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_Links_LinkGraphCycle) {
    // link graph:
    //      a -> b -> a (back ref)
    //        -> a (back ref)

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("a"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
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
    EXPECT_EQ(aa.links().size(), 2);

    if (aa.links()[0]) EXPECT_EQ(*(aa.links()[0]), bb);
    if (aa.links()[1]) EXPECT_EQ(*(aa.links()[1]), aa);

    EXPECT_TRUE(bb.complete());
    EXPECT_EQ(bb.fullname(), "b"_str);
    EXPECT_EQ(bb.kind(), yama::kind::primitive);
    EXPECT_EQ(bb.links().size(), 1);

    if (bb.links()[0]) EXPECT_EQ(*(bb.links()[0]), aa);
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToOriginalTypeAlreayInstantiated) {
    // link graph:
    //      a

    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = {},
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
    // link graph:
    //      a   <- error is that no type_data 'a' provided

    //type_data_db.push(yama::type_data(yama::primitive_info{ "a"_str }));
    
    const auto result = instant->instantiate("a"_str); // <- will fail due to no 'a'

    EXPECT_EQ(result, 0);

    EXPECT_EQ(type_db.size(), 0);
}

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToLinkedTypeNotFound) {
    // link graph:
    //      a -> b    <- error is that no type_data 'b' provided
    //        -> c

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    //yama::type_info b_info{
    //    .fullname = "b"_str,
    //    .linksyms = {},
    //    .info = yama::primitive_info{
    //        .ptype = yama::ptype::bool0,
    //    },
    //};
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = {},
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

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToIndirectlyLinkedTypeNotFound) {
    // link graph:
    //      a -> b -> d     <- error is that no type_data 'd' provided
    //             -> e
    //        -> c -> f

    static const std::vector<yama::linksym> a_linksyms{
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("d"_str, yama::kind::primitive),
        yama::make_linksym("e"_str, yama::kind::primitive),
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> c_linksyms{
        yama::make_linksym("f"_str, yama::kind::primitive),
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = c_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    //yama::type_info d_info{
    //    .fullname = "d"_str,
    //    .linksyms = {},
    //    .info = yama::primitive_info{
    //        .ptype = yama::ptype::bool0,
    //    },
    //};
    yama::type_info e_info{
        .fullname = "e"_str,
        .linksyms = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    yama::type_info f_info{
        .fullname = "f"_str,
        .linksyms = {},
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
    // link graph:
    //      a -> b -> c

    // a's b link symbol's kind is primitive
    // b's kind is function

    static const std::vector<yama::linksym> a_linksyms{
        // kind is intended to mismatch w/ the indirectly linked type b
        yama::make_linksym("b"_str, yama::kind::primitive),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0 }, 0),
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = {},
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

TEST_F(TypeInstantiatorTests, Instantiate_FailDueToCallSigMismatch) {
    // link graph:
    //      a -> b -> c

    // a's b link symbol's callsig is 'fn(b) -> b'
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
    // two respective callsigs' indices refer to different link tables

    // put another way, type 'a' will use clever_callsig_info_for_test w/
    // a_linksyms, while 'b' will use clever_callsig_info_for_test w/
    // b_linksyms, w/ the different #_linksyms used by each causing
    // the above bout of *cleverness*
    const auto clever_callsig_info_for_test = yama::make_callsig_info({ 0 }, 0);

    static const std::vector<yama::linksym> a_linksyms{
        // callsig is intended to mismatch w/ the indirectly linked type b
        yama::make_linksym("b"_str, yama::kind::function, clever_callsig_info_for_test),
    };
    yama::type_info a_info{
        .fullname = "a"_str,
        .linksyms = a_linksyms,
        .info = yama::function_info{
            .callsig = clever_callsig_info_for_test,
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    static const std::vector<yama::linksym> b_linksyms{
        yama::make_linksym("c"_str, yama::kind::primitive), // <- c exists for b's link indices to differ from a's
    };
    yama::type_info b_info{
        .fullname = "b"_str,
        .linksyms = b_linksyms,
        .info = yama::function_info{
            .callsig = clever_callsig_info_for_test,
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    yama::type_info c_info{
        .fullname = "c"_str,
        .linksyms = {},
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

