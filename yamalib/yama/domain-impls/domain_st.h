

#pragma once


#include "../core/mas.h"
#include "../core/type_info.h"
#include "../core/type.h"
#include "../core/domain.h"

#include "../dm/type_instance.h"
#include "../dm/res_db.h"
#include "../dm/compiler.h"
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


    protected:

        std::optional<std::vector<type_info>> do_compile(const taul::source_code& src) override final;
        bool do_verify(const type_info& x) override final;
        void do_upload(type_info&& x) override final;


    private:

        res<mas> _mas;

        dm::res_db<res<type_info>> _type_info_db;
        dm::res_db<res<dm::type_instance<std::allocator<void>>>> _type_db;
        dm::res_db<res<dm::type_instance<std::allocator<void>>>> _type_batch_db;
        dm::compiler _compiler;
        dm::static_verifier _verif;
        dm::type_instantiator<std::allocator<void>> _instant;
    };
}

