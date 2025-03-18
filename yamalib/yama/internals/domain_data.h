

#pragma once


#include "../core/debug.h"
#include "../core/verifier.h"

#include "compiler.h"
#include "res_state.h"
#include "install_manager.h"
#include "importer.h"
#include "loader.h"


namespace yama::internal {


    struct quick_access final {
        type none, int0, uint, float0, bool0, char0;
    };


    class domain_data final {
    public:
        std::optional<quick_access> quick_access;

        verifier verif;
        compiler compiler;

        res_state state;

        install_manager install_manager;
        importer importer;
        loader loader;


        domain_data(const std::shared_ptr<debug>& dbg);
    };
}

