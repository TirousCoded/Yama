

#pragma once


#include <set>

#include "api_component.h"
#include "type_info.h"
#include "module_info.h"
#include "parcel.h"


namespace yama {


    // verifier performs static verification of type_data

    // static verification is performed upon the upload of type_data,
    // to determine if it's valid for use

    // static verification occurs in the absence of linkage info, which
    // is established later during instantiation


    class verifier final : public api_component {
    public:
        verifier(std::shared_ptr<debug> dbg = nullptr);


        // module_path specifies the module inside which verification occurs

        bool verify(const type_info& subject, const parcel_metadata& metadata, const str& module_path);
        bool verify(const module_info& subject, const parcel_metadata& metadata, const str& module_path);


    private:
        struct _callsig_report final {
            bool param_type_indices_are_in_bounds           = true;
            bool param_type_indices_specify_type_consts     = true;
            bool return_type_indices_are_in_bounds          = true;
            bool return_type_indices_specify_type_consts    = true;
        };

        // we'll use a vector of type ref names to encapsulate the initial/final
        // register set states of a CFG block

        using _reg_set_state = std::vector<str>;

        struct _cfg_block final {
            size_t                  first = 0, last = 0;            // first/last form exclusive range [first, last)
            bool                    processed = false;              // if the CFG block has undergone symbolic execution yet
            size_t                  processed_by = size_t(-1);      // the branched-from location which caused this CFG block to be initially processed
            _reg_set_state          initial_reg_set, final_reg_set; // the initial/final register set states of the CFG block


            std::string fmt() const;

            // being exclusive, the final instr index for the block is last - 1
            inline size_t final_instr_index() const noexcept {
                return last - 1;
            }

            // returns if the final instr of the block has a primary (ie. non-fallthrough) sBx
            // offset-based jump branch associated w/ it (ie. instrs like jump and jump_if)
            bool final_instr_has_jump(const bc::code& bcode) const noexcept;

            // returns if the final instr of the block has a fallthrough-based branch associated w/ it
            bool final_instr_has_fallthrough(const bc::code& bcode) const noexcept;
        };


        std::set<size_t> _cfg_division_points;
        std::unordered_map<size_t, _cfg_block> _cfg_blocks;


        std::string _fmt_branch(size_t from, size_t to);

        void _dump_cfg(const type_info& subject, const bc::code& bcode);


        std::optional<str> _current_module_path;

        const str& _module_path() const;
        void _bind_module_path(const str& module_path);


        bool _verify(const type_info& subject, const str& module_path, const parcel_metadata& metadata);
        void _begin_verify(const type_info& subject);
        void _end_verify(bool success);
        void _post_verify_cleanup();

        bool _verify_type(const type_info& subject);
        bool _verify_type_param_names(const type_info& subject);
        bool _verify_type_callsig(const type_info& subject);

        bool _verify_constant_symbols(const type_info& subject, const parcel_metadata& metadata);
        bool _verify_constant_symbol(const type_info& subject, const_t index, const parcel_metadata& metadata);
        bool _verify_constant_symbol_qualified_name(const type_info& subject, const_t index, const parcel_metadata& metadata);
        bool _verify_constant_symbol_callsig(const type_info& subject, const_t index);

        _callsig_report gen_callsig_report(const type_info& subject, const callsig_info* callsig);

        bool _verify_bcode(const type_info& subject);
        bool _verify_bcode_not_empty(const type_info& subject, const bc::code& bcode);

        void _build_cfg(const bc::code& bcode);
        void _build_cfg_division_points(const bc::code& bcode);
        void _add_start_and_end_division_points(const bc::code& bcode);
        void _add_end_of_instr_division_point(const bc::code& bcode, size_t i);
        void _add_jump_dest_division_point(const bc::code& bcode, size_t i, int16_t sBx);
        void _build_cfg_blocks(const bc::code& bcode);
        void _add_cfg_block(const bc::code& bcode, size_t first, size_t last);

        bool _verif_cfg(const type_info& subject, const bc::code& bcode);

        void _report_dead_code_blocks(const type_info& subject, const bc::code& bcode);

        bool _visit_entrypoint_block(const type_info& subject, const bc::code& bcode);
        bool _visit_block(const type_info& subject, const bc::code& bcode, size_t block_instr_index, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _visit_processed_block(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _visit_unprocessed_block(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);

        bool _verify_no_register_coherence_violation(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        
        bool _symbolic_exec(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from);
        bool _symbolic_exec_step(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from, size_t i);
        
        // instruction-level verif checks

        bool _verify_RTop_exists(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RTop_is_type_bool(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_in_bounds(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_none(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_none_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_is_type_bool(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        // NOTE: 'this call' refers to the call the instruction is in, for verifying ret instrs
        bool _verify_RA_is_return_type_of_this_call(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RB_in_bounds_for_copy_instr(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RB_in_bounds_for_call_instr(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RB_is_return_type_of_call_object(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RB_is_return_type_of_call_object_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_KoB_in_bounds(const type_info& subject, const bc::code& bcode, size_t i);
        bool _verify_KoB_is_object_const(const type_info& subject, const bc::code& bcode, size_t i);
        bool _verify_ArgB_in_bounds(const type_info& subject, const bc::code& bcode, size_t i);
        bool _verify_RA_and_RB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_and_RB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_and_KoB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_and_KoB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_and_ArgB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_RA_and_ArgB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_ArgRs_legal_call_object(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_ArgRs_in_bounds(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_ArgRs_have_at_least_one_object(const type_info& subject, const bc::code& bcode, size_t i);
        bool _verify_param_arg_registers_are_correct_number_and_types(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        bool _verify_pushing_does_not_overflow(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);

        // block-level verif checks

        bool _verify_program_counter_valid_after_jump_by_sBx_offset(const type_info& subject, const bc::code& bcode, size_t i);
        bool _verify_program_counter_valid_after_fallthrough(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i);
        
        _reg_set_state _make_entrypoint_initial_reg_set(const type_info& subject);
        
        str _none_type();
        str _bool_type();
        str _R_type(const _cfg_block& block, size_t index);
        str _R_call_object_type_return_type(const type_info& subject, const _cfg_block& block, size_t index);
        str _Ko_type(const type_info& subject, size_t index);
        str _Arg_type(const type_info& subject, size_t index);

        size_t _calc_jump_dest(size_t i, int16_t sBx);
        bool _jump_dest_in_bounds(const bc::code& bcode, size_t i, int16_t sBx);

        std::optional<size_t> _find_type_const(const type_info& subject, const str& x);

        bool _is_newtop(uint8_t x) const noexcept;

        void _push(str type, _cfg_block& block);
        void _put(str type, size_t index, _cfg_block& block);
        void _pop(size_t n, _cfg_block& block);

        static std::string fmt_reg_set_state_diagnostic(const _reg_set_state& x);
    };
}

