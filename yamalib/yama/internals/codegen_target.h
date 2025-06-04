

#pragma once


#include "../core/bcode.h"
#include "../core/type_info.h"

#include "safeptr.h"
#include "ast.h"
#include "csymtab.h"
#include "cvalue.h"


namespace yama::internal {


    class translation_unit;


    using label_id_t = bc::code_writer::label_id_t;


    // TODO: at present, ALL custom types are fn types

    class codegen_target final {
    public:
        safeptr<translation_unit> tu;

        bc::code_writer cw; // upload_target applies this to current target
        bc::syms syms;


        codegen_target(translation_unit& tu);


        bool has_target() const noexcept;
        yama::type_info& target() noexcept;

        std::shared_ptr<csymtab_entry> try_target_csym_entry();
        res<csymtab_entry> target_csym_entry();
        template<typename T>
        inline T& target_csym() {
            return target_csym_entry()->as<T>();
        }

        std::optional<size_t> target_param_index(const str& name);


        static_assert(kinds == 3); // reminder

        void gen_target_fn(const str& unqualified_name);
        void gen_target_struct(const str& unqualified_name);

        void upload_target(const ast_node& where); // uploads target to module, unbinding it


        void autosym(taul::source_pos pos); // binds new cw autosym
        inline auto gen_label() noexcept { return _next_label++; } // used to gen code_writer label IDs


        void add_cvalue_put_instr(uint8_t reg, const cvalue& x);


    private:
        std::optional<yama::type_info> _current_target = std::nullopt;
        label_id_t _next_label = 0;


        void _bind_bare_bones_type_info(const str& unqualified_name, type_info::info_t&& info);
        void _apply_bcode_to_target(const ast_node& where);
    };
}

