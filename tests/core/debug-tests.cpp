

#include <gtest/gtest.h>

#include <yama/core/res.h>
#include <yama/core/debug.h>


// TODO: we're really lacking in testing of debug layers


TEST(DebugTests, ProxyDebug_Init) {
    const auto base = yama::make_res<yama::stderr_debug>(yama::compile_c | yama::ctx_llcmd_c | yama::ctx_panic_c);

    const auto proxy = yama::proxy_dbg(base, ~yama::ctx_panic_c);
    ASSERT_TRUE(proxy);

    ASSERT_EQ(proxy->base, base.base());
    ASSERT_EQ(proxy->cats, yama::compile_c | yama::ctx_llcmd_c);
}

TEST(DebugTests, ProxyDbg_Success) {
    const auto base = yama::make_res<yama::stderr_debug>(yama::compile_c | yama::ctx_llcmd_c | yama::ctx_panic_c);

    const auto proxy = yama::proxy_dbg(base, ~yama::ctx_panic_c);
    ASSERT_TRUE(proxy);

    ASSERT_EQ(proxy->base, base.base());
    ASSERT_EQ(proxy->cats, yama::compile_c | yama::ctx_llcmd_c);
}

TEST(DebugTests, ProxyDbg_Failure) {
    ASSERT_FALSE(yama::proxy_dbg(nullptr, ~yama::ctx_panic_c));
}

