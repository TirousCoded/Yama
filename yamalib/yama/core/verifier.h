

#pragma once


#include <set>

#include "api_component.h"
#include "module.h"
#include "parcel.h"


namespace yama {


    // Static verification is performed to ensure validity of modules
    // before use, and occurs in the absence of linkage.

    class verifier final : public api_component {
    public:
        verifier(std::shared_ptr<debug> dbg = nullptr);


        // Statically verifies subject, returning if it succeeds.
        // metadata and module_path dicate the context subject is verified in.
        // module_path specifies the module inside which verification occurs.
        bool verify(const module& subject, const parcel_metadata& metadata, const str& module_path);


    private:
        struct _callsig_report final {
            bool param_type_indices_are_in_bounds           = true;
            bool param_type_indices_specify_type_consts     = true;
            bool return_type_indices_are_in_bounds          = true;
            bool return_type_indices_specify_type_consts    = true;
        };

        // We'll use a vector of type ref names to encapsulate the initial/final
        // register set states of a CFG block.

        using _reg_set_state = std::vector<str>;

        struct _cfg_block final {
            size_t                  first = 0, last = 0;            // first/last form exclusive range [first, last).
            bool                    processed = false;              // If the CFG block has undergone symbolic execution yet.
            size_t                  processed_by = size_t(-1);      // The branched-from location which caused this CFG block to be initially processed.
            _reg_set_state          initial_reg_set, final_reg_set; // The initial/final register set states of the CFG block.


            std::string fmt() const;

            // Being exclusive, the final instr index for the block is last - 1.
            inline size_t final_instr_index() const noexcept {
                return last - 1;
            }

            // Returns if the final instr of the block has a primary (ie. non-fallthrough) sBx.
            // Offset-based jump branch associated w/ it (ie. instrs like jump and jump_if.)
            bool final_instr_has_jump(const bc::code& bcode) const noexcept;

            // Returns if the final instr of the block has a fallthrough-based branch associated w/ it.
            bool final_instr_has_fallthrough(const bc::code& bcode) const noexcept;
        };


        std::set<size_t> _cfg_division_points;
        std::unordered_map<size_t, _cfg_block> _cfg_blocks;


        const module* _current_module;
        std::optional<str> _current_module_path;
        const parcel_metadata* _current_metadata;
        std::optional<lid_t> _current_item;

        const module& _module() const;
        const str& _module_path() const;
        const parcel_metadata& _metadata() const;
        lid_t _item() const;

        void _bind_module(const module& m);
        void _bind_module_path(const str& module_path);
        void _bind_metadata(const parcel_metadata& md);
        void _bind_item(lid_t lid);


        std::string _fmt_branch(size_t from, size_t to);

        void _dump_cfg(module::item subject);


        bool _verify_module(const module& m, const str& module_path, const parcel_metadata& metadata);
        
        bool _verify_item(lid_t lid);
        void _post_item_verify_cleanup();

        bool _verify_item_unqualified_name();
        bool _verify_item_callsig();
        bool _verify_item_constsyms();
        bool _verify_item_ownership();
        bool _verify_item_bcode();

        _callsig_report _gen_callsig_report(const callsig* callsig);

        bool _verify_constsyms();
        bool _verify_constsym(const_t index);
        bool _verify_constsym_qualified_name(const_t index);
        bool _verify_constsym_callsig(const_t index);

        bool _verify_bcode();
        bool _verify_bcode_is_found();
        bool _verify_bcode_not_empty(const bc::code& bcode);

        void _build_cfg(const bc::code& bcode);
        void _build_cfg_division_points(const bc::code& bcode);
        void _add_start_and_end_division_points(const bc::code& bcode);
        void _add_end_of_instr_division_point(const bc::code& bcode, size_t i);
        void _add_jump_dest_division_point(const bc::code& bcode, size_t i, int16_t sBx);
        void _build_cfg_blocks(const bc::code& bcode);
        void _add_cfg_block(const bc::code& bcode, size_t first, size_t last);

        bool _verif_cfg(module::item subject);

        void _report_dead_code_blocks(module::item subject);

        bool _visit_entrypoint_block(module::item subject);
        bool _visit_block(module::item subject, size_t block_instr_index, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _visit_processed_block(module::item subject, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _visit_unprocessed_block(module::item subject, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);

        bool _verify_no_register_coherence_violation(module::item subject, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        
        bool _symbolic_exec(module::item subject, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _symbolic_exec_step(module::item subject, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from, size_t i);
        
        // Instruction-Level Verif Checks

        bool _verify_RTop_exists(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RTop_is_type_bool(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_in_bounds(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_none(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_none_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_bool(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        // NOTE: 'this call' refers to the call the instruction is in, for verifying ret instrs.
        bool _verify_RA_is_return_type_of_this_call(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_in_bounds(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_in_bounds_for_copy_instr(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_in_bounds_for_call_instr(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_is_return_type_of_call_object(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_is_return_type_of_call_object_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_KoB_in_bounds(module::item subject, size_t i);
        bool _verify_KoB_is_object_const(module::item subject, size_t i);
        bool _verify_KtB_in_bounds(module::item subject, size_t i);
        bool _verify_KtB_is_type_const(module::item subject, size_t i);
        bool _verify_KtC_in_bounds(module::item subject, size_t i);
        bool _verify_KtC_is_type_const(module::item subject, size_t i);
        bool _verify_ArgB_in_bounds(module::item subject, size_t i);
        bool _verify_RA_and_RB_agree_on_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_RB_agree_on_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_KoB_agree_on_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_KoB_agree_on_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_KtB_agree_on_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_KtB_agree_on_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_ArgB_agree_on_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RA_and_ArgB_agree_on_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_and_KtC_agree_on_type(module::item subject, _cfg_block& block, size_t i);
        bool _verify_RB_and_KtC_agree_on_type_skip_if_reinit(module::item subject, _cfg_block& block, size_t i);
        bool _verify_ArgRs_legal_call_object(module::item subject, _cfg_block& block, size_t i);
        bool _verify_ArgRs_in_bounds(module::item subject, _cfg_block& block, size_t i);
        bool _verify_ArgRs_have_at_least_one_object(module::item subject, size_t i);
        bool _verify_param_arg_registers_are_correct_number_and_types(module::item subject, _cfg_block& block, size_t i);
        bool _verify_pushing_does_not_overflow(module::item subject, _cfg_block& block, size_t i);

        // Block-Level Verif Checks

        bool _verify_program_counter_valid_after_jump_by_sBx_offset(module::item subject, size_t i);
        bool _verify_program_counter_valid_after_fallthrough(module::item subject, _cfg_block& block, size_t i);
        
        _reg_set_state _make_entrypoint_initial_reg_set(module::item subject);

        static_assert(ptypes == 7); // reminder
        str _none_type();
        str _int_type();
        str _uint_type();
        str _float_type();
        str _bool_type();
        str _char_type();
        str _type_type();

        str _R_type(const _cfg_block& block, size_t index);
        str _R_call_object_type_return_type(module::item subject, const _cfg_block& block, size_t index);
        str _Ko_type(module::item subject, size_t index);
        str _Kt_type(module::item subject, size_t index); // For _Kt_type, it's the type named in the constant.
        str _Arg_type(module::item subject, size_t index);

        size_t _calc_jump_dest(size_t i, int16_t sBx);
        bool _jump_dest_in_bounds(const bc::code& bcode, size_t i, int16_t sBx);

        bool _is_builtin_prim_type(const str& x);

        std::optional<size_t> _find_type_const(module::item subject, const str& x);
        std::optional<kind> _find_type_kind(module::item subject, const str& x);

        bool _is_newtop(uint8_t x) const noexcept;

        void _push(str type, _cfg_block& block);
        void _put(str type, size_t index, _cfg_block& block);
        void _pop(size_t n, _cfg_block& block);

        static std::string fmt_reg_set_state_diagnostic(const _reg_set_state& x);
    };
}

