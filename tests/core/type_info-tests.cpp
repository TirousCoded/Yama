

#include <gtest/gtest.h>

#include <yama/core/const_table_info.h>
#include <yama/core/const_table.h>
#include <yama/core/type_info.h>


using namespace yama::string_literals;


static_assert(yama::kinds == 2);

TEST(TypeInfoTests, PrimitiveType) {
    yama::type_info abc{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::primitive_info{
            .ptype = yama::ptype::bool0,
        },
    };

    EXPECT_EQ(abc.kind(), yama::kind::primitive);

    EXPECT_EQ(abc.ptype(), yama::ptype::bool0);
    EXPECT_EQ(abc.callsig(), nullptr);
    EXPECT_EQ(abc.call_fn(), std::nullopt);
    EXPECT_EQ(abc.locals(), 0);
    EXPECT_EQ(abc.bcode(), nullptr);
}

TEST(TypeInfoTests, FunctionType) {
    auto cf = [](yama::context&, yama::const_table) {};

    yama::type_info abc{
        .fullname = "abc"_str,
        .consts = {},
        .info = yama::function_info{
            .callsig = yama::make_callsig_info({ 0, 1, 2 }, 1),
            .call_fn = cf,
            .locals = 17,
        },
    };

    EXPECT_EQ(abc.kind(), yama::kind::function);

    EXPECT_EQ(abc.ptype(), std::nullopt);
    EXPECT_EQ(abc.callsig(), &(std::get<yama::function_info>(abc.info).callsig));
    EXPECT_EQ(abc.call_fn(), cf);
    EXPECT_EQ(abc.locals(), 17);
    EXPECT_EQ(abc.bcode(), &(std::get<yama::function_info>(abc.info).bcode));
}

