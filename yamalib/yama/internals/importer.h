

#pragma once


#include "../core/api_component.h"
#include "../core/module.h"
#include "../core/parcel.h"
#include "../core/verifier.h"

#include "safeptr.h"
#include "env.h"
#include "specifiers.h"
#include "res_state.h"


namespace yama::internal {


    class domain_data;


    class importer final : public api_component {
    public:
        module_area state;


        importer(std::shared_ptr<debug> dbg, domain_data& dd);


        std::optional<yama::module> import(const env& e, const str& path, res_state& upstream);

        // these are used by compiler

        std::optional<import_result> import_for_import_dir(const import_path& path);
        bool upload_compiled_module(const import_path& path, res<module_info> new_module);


    private:
        safeptr<domain_data> _dd;


        bool _import(const env& e, const import_path& path);
        std::optional<import_path> _resolve_import_path(const env& e, const str& path);
        bool _check_already_imported(const import_path& path);
        bool _handle_fresh_import_and_memoize(const env& e, const import_path& path, std::shared_ptr<parcel> p);

        bool _verify_and_memoize(const res<module_info>& m, const import_path& path);
        bool _verify(const module_info& m, const import_path& path);
        void _report_module_not_found(const import_path& path);

        
        static bool _do_verify(const std::shared_ptr<debug>& dbg, const env& e, verifier& verifier, const module_info& m, const import_path& path, std::shared_ptr<parcel> p);
        static void _do_report_module_not_found(const std::shared_ptr<debug>& dbg, const env& e, const import_path& path);
    };
}

