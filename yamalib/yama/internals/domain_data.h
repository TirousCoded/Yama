

#pragma once


#include <mutex>
#include <shared_mutex>

#include "../core/debug.h"
#include "../core/verifier.h"

#include "res_state.h"
#include "installer.h"
#include "compiler.h"
#include "importer.h"
#include "loader.h"


namespace yama::internal {


    struct quick_access final {
        item_ref none, int0, uint, float0, bool0, char0, type;
    };


    class domain_data final {
    public:
        std::optional<quick_access> quick_access; // immutable, so no lock needed

        // TODO: maybe look into using spinlock instead of shared_mutex for state_mtx

        // state will be protected by a shared mutex, so that multiple threads
        // can query already loaded/imported types/modules w/out unneeded thread
        // contention

        // the exclusive lock will be held when new types are being committed

        // in order to exclusive lock you MUST first lock new_data_mtx

        std::shared_mutex state_mtx;

        // when performing round of loading/importing/installing of NEW DATA, this
        // lock MUST be held

        std::recursive_mutex new_data_mtx;

        install_state installs;
        res_state state;

        installer installer;
        importer importer;
        loader loader;
        verifier verif;
        compiler compiler;


        domain_data(const std::shared_ptr<debug>& dbg);
    };
}

