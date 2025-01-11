

#include <gtest/gtest.h>

#include <yama/core/general.h>
#include <yama/core/callsig.h>
#include <yama/core/const_table_info.h>
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


yama::internal::type_instance<std::allocator<void>> make_type_inst_1(
    yama::str fullname,
    yama::const_table_info consts) {
    yama::type_info info{
        .fullname = fullname,
        .consts = std::move(consts),
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };
    return
        yama::internal::type_instance<std::allocator<void>>(
            std::allocator<void>{}, fullname, yama::make_res<yama::type_info>(info));
}

yama::internal::type_instance<std::allocator<void>> make_type_inst_2(
    yama::str fullname,
    yama::callsig_info callsig,
    yama::const_table_info consts) {
    yama::type_info info{
        .fullname = fullname,
        .consts = std::move(consts),
        .info = yama::function_info{
            .callsig = callsig,
            .call_fn = yama::noop_call_fn,
            .max_locals = 10,
        },
    };
    return
        yama::internal::type_instance<std::allocator<void>>(
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

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::primitive_type_const>(1, b);
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());

    
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

TEST(CallSigTests, Usage_IncompleteType_OutOfBoundsParamTypeConstIndex) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 7, 2 }, 1); // <- out-of-bounds link index 7
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::primitive_type_const>(1, b);
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());

    
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

TEST(CallSigTests, Usage_IncompleteType_OutOfBoundsReturnTypeConstIndex) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 7); // <- out-of-bounds link index 7
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::primitive_type_const>(1, b);
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());


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

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 0);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    //x_inst.put<yama::primitive_type_const>(1, b); <- stub
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());


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

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 0, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    //x_inst.put<yama::primitive_type_const>(1, b); <- stub
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());


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

TEST(CallSigTests, Usage_IncompleteType_ParamTypeIsNotATypeConstant) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type c(c_inst);

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_int(31)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 0);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::int_const>(1, 31); // <- not a type constant!
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());


    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_FALSE(x_callsig.param_type(1)); // <- failed due to not a type constant
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);

    EXPECT_TRUE(x_callsig.return_type());

    if (x_callsig.return_type()) EXPECT_EQ(x_callsig.return_type().value(), a);
}

TEST(CallSigTests, Usage_IncompleteType_ReturnTypeIsNotATypeConstant) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type c(c_inst);

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_int(31)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 0, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::int_const>(1, 31); // <- not a type constant!
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);

    const yama::callsig x_callsig(x_callsig_info, x.consts());


    EXPECT_EQ(x_callsig.params(), 3);

    EXPECT_TRUE(x_callsig.param_type(0));
    EXPECT_TRUE(x_callsig.param_type(1));
    EXPECT_TRUE(x_callsig.param_type(2));
    EXPECT_FALSE(x_callsig.param_type(3)); // <- out-of-bounds param index

    if (x_callsig.param_type(0)) EXPECT_EQ(x_callsig.param_type(0).value(), a);
    if (x_callsig.param_type(1)) EXPECT_EQ(x_callsig.param_type(1).value(), a);
    if (x_callsig.param_type(2)) EXPECT_EQ(x_callsig.param_type(2).value(), c);

    EXPECT_FALSE(x_callsig.return_type()); // <- failed due to not a type constant
}

TEST(CallSigTests, Equality) {
    const auto a_inst = make_type_inst_1("a"_str, {});
    const auto b_inst = make_type_inst_1("b"_str, {});
    const auto c_inst = make_type_inst_1("c"_str, {});
    const yama::type a(a_inst);
    const yama::type b(b_inst);
    const yama::type c(c_inst);

    // x1_callsig
    const auto x1_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x1_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x1_inst = make_type_inst_2("x1"_str, x1_callsig_info, std::move(x1_consts));
    x1_inst.put<yama::primitive_type_const>(0, a);
    x1_inst.put<yama::primitive_type_const>(1, b);
    x1_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x1(x1_inst);

    // x2_callsig
    const auto x2_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x2_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x2_inst = make_type_inst_2("x2"_str, x2_callsig_info, std::move(x2_consts));
    x2_inst.put<yama::primitive_type_const>(0, a);
    x2_inst.put<yama::primitive_type_const>(1, b);
    x2_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x2(x2_inst);

    // y_callsig
    const auto y_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str);
    const auto y_callsig_info = yama::make_callsig_info({}, 0);
    auto y_inst = make_type_inst_2("y"_str, y_callsig_info, std::move(y_consts));
    y_inst.put<yama::primitive_type_const>(0, a);
    const yama::type y(y_inst);

    // z1_callsig
    const auto z1_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str);
    const auto z1_callsig_info = yama::make_callsig_info({ 1 }, 1); // <- param/return type indices out-of-bounds!
    auto z1_inst = make_type_inst_2("z1"_str, z1_callsig_info, std::move(z1_consts));
    z1_inst.put<yama::primitive_type_const>(0, a);
    const yama::type z1(z1_inst);

    // z2_callsig
    const auto z2_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str);
    const auto z2_callsig_info = yama::make_callsig_info({ 0 }, 0);
    auto z2_inst = make_type_inst_2("z2"_str, z2_callsig_info, std::move(z2_consts));
    //z2_inst.put<yama::primitive_type_const>(0, a); <- stub
    const yama::type z2(z2_inst);

    // x1_callsig and x2_callsig are referentially different, but structurally equal

    // z1_callsig and z2 callsig are likewise, like the above two, but w/ the added
    // nuance of their std::nullopt being different reasons

    const yama::callsig x1_callsig(x1_callsig_info, x1.consts());
    const yama::callsig x2_callsig(x2_callsig_info, x2.consts());
    const yama::callsig y_callsig(y_callsig_info, y.consts());
    const yama::callsig z1_callsig(z1_callsig_info, z1.consts());
    const yama::callsig z2_callsig(z2_callsig_info, z2.consts());

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

    const auto x_consts =
        yama::const_table_info()
        .add_primitive_type("a"_str)
        .add_primitive_type("b"_str)
        .add_primitive_type("c"_str);
    const auto x_callsig_info = yama::make_callsig_info({ 0, 1, 2 }, 1);
    auto x_inst = make_type_inst_2("x"_str, x_callsig_info, std::move(x_consts));
    x_inst.put<yama::primitive_type_const>(0, a);
    x_inst.put<yama::primitive_type_const>(1, b);
    x_inst.put<yama::primitive_type_const>(2, c);
    const yama::type x(x_inst);
    
    const yama::callsig x_callsig(x_callsig_info, x.consts());

    std::string expected = "fn(a, b, c) -> b";
    std::string actual = x_callsig.fmt();

    std::cerr << expected << "\n" << actual << "\n";

    EXPECT_EQ(expected, actual);
}

