

#pragma once


#include <functional>

#include "../core/api_component.h"
#include "../core/module.h"
#include "../core/parcel.h"
#include "../core/env.h"
#include "../core/specifiers.h"
#include "../core/verifier.h"
#include "../core/compiler.h"

#include "res_state.h"
#include "install_manager.h"


namespace yama::internal {


    class importer final : public api_component {
    public:
        importer(std::shared_ptr<debug> dbg, install_manager& install_manager, verifier& verifier, compiler& compiler);

        virtual ~importer() noexcept = default;


        std::optional<yama::module> import(const env& e, const str& path, res_state& upstream);


    private:
        install_manager* _install_manager;
        verifier* _verifier;
        compiler* _compiler;


        install_manager& _get_install_manager() const noexcept;
        verifier& _get_verifier() const noexcept;
        compiler& _get_compiler() const noexcept;


        module_area _state;


        bool _import(const env& e, const import_path& path);
        std::optional<import_path> _resolve_import_path(const env& e, const str& path);
        bool _check_already_imported(const import_path& path);
        bool _handle_fresh_import_and_memoize(const env& e, const import_path& path, std::shared_ptr<parcel> p);


        bool _verify(const module_info& m, const import_path& path, parcel_id id);
        void _report_module_not_found(const import_path& path);

        
        static bool _do_verify(const std::shared_ptr<debug>& dbg, const env& e, verifier& verifier, const module_info& m, const import_path& path, std::shared_ptr<parcel> p);
        static void _do_report_module_not_found(const std::shared_ptr<debug>& dbg, const env& e, const import_path& path);


        class _compiler_services final : public compiler_services {
        public:
            _compiler_services(std::shared_ptr<debug> dbg, parcel_id compilation_env_parcel_id, importer& upstream);


            yama::env env() const override final;
            std::optional<import_result_ext> import(const yama::import_path& path) override final;


        private:
            importer* _upstream;
            yama::env _env;


            bool _verify(const module_info& m, const import_path& path, std::shared_ptr<parcel> p);
            void _report_module_not_found(const import_path& path);
        };

        res<compiler_services> _acquire_cs(parcel_id compilation_env_parcel_id);
    };
}

