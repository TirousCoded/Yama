

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../utils/utils.h"


TEST(Contexts, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    {
        EXPECT_EQ(ymRefCount(dm), 1); // No ctx ref.
        SETUP_CTX; // Macro will do the create/destroy.
        EXPECT_EQ(ymRefCount(dm), 2); // ctx added ref.
        // RAII destroy ctx.
    }
    EXPECT_EQ(ymRefCount(dm), 1); // No ctx ref.
}

TEST(Contexts, RType) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    EXPECT_EQ(ymRType(ctx), YmRType_Ctx);
}

TEST(Contexts, RC) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    ASSERT_EQ(ymRefCount(ctx), 1); // Initial value.
    ymAddRef(ctx);
    EXPECT_EQ(ymRefCount(ctx), 2);
    ymAddRef(ctx);
    EXPECT_EQ(ymRefCount(ctx), 3);
    ymDrop(ctx);
    EXPECT_EQ(ymRefCount(ctx), 2);
    ymDrop(ctx);
    EXPECT_EQ(ymRefCount(ctx), 1);
    // ScopedDrop will drop final one, and the actual release of
    // the resource is unobservable.
}

TEST(Contexts, Dm) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    auto dmRef = ymCtx_Dm(ctx);
    ym::ScopedDrop dmRef_(dmRef);
    EXPECT_EQ(dmRef, dm);
    EXPECT_EQ(ymRefCount(dm), 3); // Added ref.
}

TEST(Contexts, Import) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX;
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    auto a = ymCtx_Import(ctx, path.c_str());
    auto b = ymCtx_Import(ctx, path.c_str()); // Should yield same result.
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(a, b);
}

TEST(Contexts, Import_IllegalPaths) {
    for (const auto& path : illegalPaths) {
        SETUP_ERRCOUNTER;
        SETUP_DM;
        SETUP_CTX;
        SETUP_PARCELDEF(p);
        EXPECT_EQ(ymCtx_Import(ctx, path.c_str()), nullptr)
            << "path == \"" << path << "\"";
        EXPECT_EQ(err[YmErrCode_IllegalPath], 1);
    }
}

