

#pragma once


#include "../core/bcode.h"
#include "../core/module.h"

#include "safeptr.h"
#include "ast.h"
#include "csymtab.h"
#include "cvalue.h"


namespace yama::internal {


    class translation_unit;


    using label_id_t = bc::code_writer::label_id_t;


    // Carries info about the item currently being generated.
    class codegen_target final {
    public:
        safeptr<translation_unit> tu;

        // where == nullptr checks if has target bound currently.
        const ast_node* where = nullptr;
        kind kind = {};
        str owner_name;
        std::optional<str> member_name;
        str unqualified_name;
        const_table consts;
        std::optional<callsig> callsig;
        size_t max_locals = 0;

        bc::code_writer cw;
        bc::syms syms;


        codegen_target(translation_unit& tu);


        static_assert(kinds == 4); // Reminder.

        // Starts new target.
        void start(const ast_FnDecl& decl);
        // Starts new target.
        void start(const ast_StructDecl& decl);

        // Pushes target to module, invaliding current state (until next start call.)
        void finish();


        inline bool has_target() const noexcept {
            return where;
        }

        std::shared_ptr<csym> try_target_csym_entry();
        res<csym> target_csym_entry();
        template<csym_type T>
        inline T& target_csym() {
            return *target_csym_entry()->expect<T>();
        }

        std::optional<size_t> target_param_index(const str& name);


        // Binds new cw autosym.
        void autosym(taul::source_pos pos);
        // Generate next code_writer label ID.
        inline auto gen_label() noexcept {
            return _next_label++;
        }


        void add_cvalue_put_instr(uint8_t reg, const cvalue& x);


    private:
        label_id_t _next_label = 0;


        void _apply_bcode_to_output();
    };
}

