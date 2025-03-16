

#pragma once


#include <gtest/gtest.h>

#include <optional>

#include <yama/core/general.h>
#include <yama/core/res.h>
#include <yama/core/type_info.h>
#include <yama/core/module_info.h>
#include <yama/core/env.h>
#include <yama/core/verifier.h>


struct VerifierImplTestsParam final {
    std::function<yama::res<yama::verifier>(std::shared_ptr<yama::debug> dbg)> factory;
};

class VerifierImplTests : public testing::TestWithParam<VerifierImplTestsParam> {
public:

    std::shared_ptr<yama::dsignal_debug> dbg;
    std::shared_ptr<yama::verifier> verif;

    std::optional<yama::parcel_metadata> md;

    const yama::parcel_metadata& get_md() { return yama::deref_assert(md); }


    inline void SetUp() override final {
        dbg = std::make_shared<yama::dsignal_debug>(std::make_shared<yama::stderr_debug>());
        verif = GetParam().factory(dbg);

        // map names 'yama' and 'abc' to arbitrary parcel IDs
        md = yama::parcel_metadata{ yama::str::lit("self"), { yama::str::lit("yama"), yama::str::lit("abc") } };
    }

    inline void TearDown() override final {
        //
    }
};

