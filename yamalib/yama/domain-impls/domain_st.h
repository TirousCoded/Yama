

#pragma once


#include "../core/mas.h"
#include "../core/type_data.h"
#include "../core/type.h"
#include "../core/domain.h"

#include "../dm/type_instance.h"
#include "../dm/res_db.h"
#include "../dm/static_verifier.h"
#include "../dm/type_instantiator.h"


namespace yama {


    // yama::domain_st is the standard single-threaded yama::domain impl

    // pretty obvious, but the '_st' stands for 'single-threaded'


    class domain_st final : public domain {
    public:

        domain_st(
            res<mas> mas,
            std::shared_ptr<debug> dbg = nullptr);


        res<mas> get_mas() override final;
        std::optional<type> load(const str& fullname) override final;
        bool push(type_data x) override final;


    private:

        res<mas> _mas;

        dm::res_db<type_data> _type_data_db;
        dm::res_db<res<dm::type_instance<std::allocator<void>>>> _type_db;
        dm::res_db<res<dm::type_instance<std::allocator<void>>>> _type_batch_db;
        dm::static_verifier _verif;
        dm::type_instantiator<std::allocator<void>> _instant;
    };
}

