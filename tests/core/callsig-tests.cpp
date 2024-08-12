

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig.h>
#include <yama/core/type_info.h>
#include <yama/core/type.h>
#include <yama/dm/type_instance.h>


using namespace yama::string_literals;


yama::dm::type_instance<std::allocator<void>> make_type_inst_1(
    yama::str fullname,
    std::vector<yama::linksym>&& linksyms) {
    yama::type_info info{
        .fullname = fullname,
        .linksyms = std::forward<decltype(linksyms)&&>(linksyms),
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    return
        yama::dm::type_instance<std::allocator<void>>(
            std::allocator<void>{}, fullname, yama::make_res<yama::type_info>(info));
}

yama::dm::type_instance<std::allocator<void>> make_type_inst_2(
    yama::str fullname,
    yama::callsig_info callsig,
    std::vector<yama::linksym>&& linksyms) {
    yama::type_info info{
        .fullname = fullname,
        .linksyms = std::forward<decltype(linksyms)&&>(linksyms),
        .info = yama::function_info{
            .callsig = callsig,
            .call_fn = yama::noop_call_fn,
            .locals = 10,
        },
    };
    return
        yama::dm::type_instance<std::allocator<void>>(
            std::allocator<void>{}, fullname, yama::make_res<yama::type_info>(info));
}


// NOTE: the below 'Usage_***' tests will test ctor, params, param_type, 
//       and return_type, all at once, via a series of specific use cases,
//       rather than testing per-method

TEST(CallSigTests, Usage_CompleteType) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    x_inst.put_link(1, b);
    x_inst.put_link(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.links());

    
    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_TRUE(x_callsig.param_type(1));
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(1)) EXPECT_EQ(x_callsig.param_type(1).value(), b);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);
    
    EXPECT_TRUE(x_callsig.return_type());

    if (x_callsig.return_type()) EXPECT_EQ(x_callsig.return_type().value(), b);
}

TEST(CallSigTests, Usage_IncompleteType_OutOfBoundsParamTypeLinkIndex) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 7, 2 }, 1); // <- out-of-bounds link index 7
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    x_inst.put_link(1, b);
    x_inst.put_link(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.links());

    
    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_FALSE(x_callsig.param_type(1)); // <- failed due to out-of-bounds link index
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);
    
    EXPECT_TRUE(x_callsig.return_type());

    if (x_callsig.return_type()) EXPECT_EQ(x_callsig.return_type().value(), b);
}

TEST(CallSigTests, Usage_IncompleteType_OutOfBoundsReturnTypeLinkIndex) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 7); // <- out-of-bounds link index 7
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    x_inst.put_link(1, b);
    x_inst.put_link(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.links());


    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_TRUE(x_callsig.param_type(1));
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(1)) EXPECT_EQ(x_callsig.param_type(1).value(), b);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);

    EXPECT_FALSE(x_callsig.return_type()); // <- failed due to out-of-bounds link index
}

TEST(CallSigTests, Usage_IncompleteType_ParamTypeIsStub) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 0);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    //x_inst.put_link(1, b); <- stub
    x_inst.put_link(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.links());


    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_FALSE(x_callsig.param_type(1)); // <- failed due to stub
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);

    EXPECT_TRUE(x_callsig.return_type());

    if (x_callsig.return_type()) EXPECT_EQ(x_callsig.return_type().value(), a);
}

TEST(CallSigTests, Usage_IncompleteType_ReturnTypeIsStub) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 0, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    //x_inst.put_link(1, b); <- stub
    x_inst.put_link(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.links());


    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_TRUE(x_callsig.param_type(1));
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(1)) EXPECT_EQ(x_callsig.param_type(1).value(), a);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);

    EXPECT_FALSE(x_callsig.return_type()); // <- failed due to stub
}

TEST(CallSigTests, Equality) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    // x1_callsig
    std::vector<yama::linksym> x1_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x1_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x1_inst = make_type_inst_2("x1"_str, x1_callsig_info, std::move(x1_linksyms));
    x1_inst.put_link(0, a);
    x1_inst.put_link(1, b);
    x1_inst.put_link(2, c);
    const yama::type x1(x1_inst);

    // x2_callsig
    std::vector<yama::linksym> x2_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x2_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x2_inst = make_type_inst_2("x2"_str, x2_callsig_info, std::move(x2_linksyms));
    x2_inst.put_link(0, a);
    x2_inst.put_link(1, b);
    x2_inst.put_link(2, c);
    const yama::type x2(x2_inst);

    // y_callsig
    std::vector<yama::linksym> y_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
    };
    const auto y_callsig_info = yama::make_callsig_info({}, 0);
    auto y_inst = make_type_inst_2("y"_str, y_callsig_info, std::move(y_linksyms));
    y_inst.put_link(0, a);
    const yama::type y(y_inst);

    // z1_callsig
    std::vector<yama::linksym> z1_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
    };
    const auto z1_callsig_info = yama::make_callsig_info({ 1 }, 1); // <- param/return type indices out-of-bounds!
    auto z1_inst = make_type_inst_2("z1"_str, z1_callsig_info, std::move(z1_linksyms));
    z1_inst.put_link(0, a);
    const yama::type z1(z1_inst);

    // z2_callsig
    std::vector<yama::linksym> z2_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
    };
    const auto z2_callsig_info = yama::make_callsig_info({ 0 }, 0);
    auto z2_inst = make_type_inst_2("z2"_str, z2_callsig_info, std::move(z2_linksyms));
    //z2_inst.put_link(0, a); <- stub
    const yama::type z2(z2_inst);

    // x1_callsig and x2_callsig are referentially different, but structurally equal

    // z1_callsig and z2 callsig are likewise, like the above two, but w/ the added
    // nuance of their std::nullopt being different reasons

    const yama::callsig x1_callsig(x1_callsig_info, x1.links());
    const yama::callsig x2_callsig(x2_callsig_info, x2.links());
    const yama::callsig y_callsig(y_callsig_info, y.links());
    const yama::callsig z1_callsig(z1_callsig_info, z1.links());
    const yama::callsig z2_callsig(z2_callsig_info, z2.links());

    EXPECT_EQ(x1_callsig, x1_callsig);
    EXPECT_EQ(x1_callsig, x2_callsig);
    EXPECT_NE(x1_callsig, y_callsig);
    EXPECT_NE(x1_callsig, z1_callsig);
    EXPECT_NE(x1_callsig, z2_callsig);

    EXPECT_EQ(x2_callsig, x2_callsig);
    EXPECT_NE(x2_callsig, y_callsig);
    EXPECT_NE(x2_callsig, z1_callsig);
    EXPECT_NE(x2_callsig, z2_callsig);

    EXPECT_EQ(y_callsig, y_callsig);
    EXPECT_NE(y_callsig, z1_callsig);
    EXPECT_NE(y_callsig, z2_callsig);

    EXPECT_EQ(z1_callsig, z1_callsig);
    EXPECT_EQ(z1_callsig, z2_callsig);

    EXPECT_EQ(z2_callsig, z2_callsig);
}

TEST(CallSigTests, Fmt) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    std::vector<yama::linksym> x_linksyms{
        yama::make_linksym("a"_str, yama::kind::primitive),
        yama::make_linksym("b"_str, yama::kind::primitive),
        yama::make_linksym("c"_str, yama::kind::primitive),
    };
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_linksyms));
    x_inst.put_link(0, a);
    x_inst.put_link(1, b);
    x_inst.put_link(2, c);
    const yama::type x(x_inst);
    
    const yama::callsig x_callsig(x_callsig_info, x.links());

    std::string expected = "fn(a, b, c) -> b";
    std::string actual = x_callsig.fmt();

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

