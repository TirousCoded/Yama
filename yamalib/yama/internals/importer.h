

#pragma once


#include "../core/api_component.h"
#include "../core/module_ref.h"
#include "../core/parcel.h"
#include "../core/verifier.h"

#include "safeptr.h"
#include "env.h"
#include "specifiers.h"
#include "res_state.h"
#include "mid_provider.h"


namespace yama::internal {


    class domain_data;


    class importer final : public api_component {
    public:
        module_area state;
        mid_provider mids;


        importer(std::shared_ptr<debug> dbg, domain_data& dd);


        std::optional<yama::module_ref> import(const env& e, const str& path, res_state& upstream);

        // IMPORTANT: see res_area::commit comment in res_state.h for info on protects_upstream

        // this MUST be called IMMEDIATELY after a call to import finishes in order to cleanup
        // properly and, if successful, commit state upstream

        void commit_or_discard();
        void commit_or_discard(std::shared_mutex& protects_upstream);

        // these are used by compiler

        std::optional<import_result> import_for_import_dir(const import_path& path);
        bool upload_compiled_module(const import_path& path, res<module> new_module);


    private:
        safeptr<domain_data> _dd;

        bool _last_was_success = false;


        // locks _dd->new_data_mtx upon detecting need to generate new data, releasing lock
        // during commit_or_discard call

        std::atomic_bool _holds_lock = false;

        void _lock();
        void _unlock();


        bool _import(const env& e, const import_path& path);
        std::optional<import_path> _resolve_import_path(const env& e, const str& path);
        bool _check_already_imported(const import_path& path);
        bool _handle_fresh_import_and_memoize(const env& e, const import_path& path, std::shared_ptr<parcel> p);

        bool _verify_and_memoize(const res<module>& m, mid_t mid, const import_path& path);
        bool _verify(const module& m, const import_path& path);
        void _report_module_not_found(const import_path& path);
        void _memoize(const res<module>& m, mid_t mid, const import_path& path);
    };
}

