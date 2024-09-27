

#include "static_verifier.h"

#include "../core/kind-features.h"

#include "../internals/util.h"


using namespace yama::string_literals;


yama::dm::static_verifier::static_verifier(std::shared_ptr<debug> dbg) 
    : api_component(dbg) {}

bool yama::dm::static_verifier::verify(const type_info& subject) {
    _begin_verify(subject);
    const bool success = _verify(subject);
    _end_verify(success);
    _post_verify_cleanup();
    return success;
}

std::string yama::dm::static_verifier::_cfg_block::fmt() const {
    return std::format("{{block [{}, {})}}", first, last);
}

bool yama::dm::static_verifier::_cfg_block::final_instr_has_jump(const bc::code& bcode) const noexcept {
    static_assert(bc::opcodes == 11);
    switch (bcode[final_instr_index()].opc) {
    case bc::opcode::ret:           return false;
    case bc::opcode::jump:          return true;
    case bc::opcode::jump_true:     return true;
    case bc::opcode::jump_false:    return true;
    default:                        return false;
    }
}

bool yama::dm::static_verifier::_cfg_block::final_instr_has_fallthrough(const bc::code& bcode) const noexcept {
    static_assert(bc::opcodes == 11);
    switch (bcode[final_instr_index()].opc) {
    case bc::opcode::ret:           return false;
    case bc::opcode::jump:          return false;
    case bc::opcode::jump_true:     return true;
    case bc::opcode::jump_false:    return true;
    default:                        return true;
    }
}

std::string yama::dm::static_verifier::_fmt_branch(size_t from, size_t to) {
    return std::format("{{branch {} -> {}}}", from, to);
}

void yama::dm::static_verifier::_dump_cfg(const bc::code& bcode) {
    YAMA_LOG(dbg(), dm_verif_c, "dumping CFG:");
    for (size_t i = 0; i < bcode.count(); i++) {
        // _cfg_blocks is a hash map w/ instrs as keys, so we're just gonna
        // quick-n'-dirty loop over all the instrs, skipping ones w/out blocks,
        // w/ doing this ensuring we print everything in the correct order
        if (!_cfg_blocks.contains(i)) {
            continue;
        }
        const auto& b = _cfg_blocks.at(i);
        YAMA_LOG(dbg(), dm_verif_c, "{}", b.fmt());
        constexpr const char* tab = "    ";
        YAMA_LOG(dbg(), dm_verif_c, "{}processed            : {}", tab, b.processed);
        YAMA_LOG(dbg(), dm_verif_c, "{}processed-by         : {}", tab,
            [&]() -> std::string {
                if (!b.processed) {
                    return "n/a";
                }
                else if (b.first == 0) { // entrypoint block doesn't *come from* anywhere
                    return "n/a";
                }
                else return _fmt_branch(b.processed_by, b.first);
            }());
        YAMA_LOG(dbg(), dm_verif_c, "{}initial register states:", tab);
        if (!b.processed) {
            YAMA_LOG(dbg(), dm_verif_c, "{}{}n/a", tab, tab);
        }
        else if (b.initial_reg_set.empty()) {
            YAMA_LOG(dbg(), dm_verif_c, "{}{}n/a", tab, tab);
        }
        else {
            for (size_t j = 0; j < b.initial_reg_set.size(); j++) {
                YAMA_LOG(dbg(), dm_verif_c, "{}{}register {}: {}", tab, tab, j, b.initial_reg_set[j]);
            }
        }
        YAMA_LOG(dbg(), dm_verif_c, "{}final register states:", tab);
        if (!b.processed) {
            YAMA_LOG(dbg(), dm_verif_c, "{}{}n/a", tab, tab);
        }
        else if (b.final_reg_set.empty()) {
            YAMA_LOG(dbg(), dm_verif_c, "{}{}n/a", tab, tab);
        }
        else {
            for (size_t j = 0; j < b.final_reg_set.size(); j++) {
                YAMA_LOG(dbg(), dm_verif_c, "{}{}register {}: {}", tab, tab, j, b.final_reg_set[j]);
            }
        }
    }
}

bool yama::dm::static_verifier::_verify(const type_info& subject) {
    if (!_verify_type(subject)) {
        return false;
    }
    if (!_verify_constant_symbols(subject)) {
        return false;
    }
    if (!_verify_bcode(subject)) {
        return false;
    }
    return true;
}

void yama::dm::static_verifier::_begin_verify(const type_info& subject) {
    YAMA_LOG(dbg(), dm_verif_c, "verifying {}...", subject.fullname);
}

void yama::dm::static_verifier::_end_verify(bool success) {
    // TODO: should we even log anything here?
}

void yama::dm::static_verifier::_post_verify_cleanup() {
    _cfg_division_points.clear();
    _cfg_blocks.clear();
}

bool yama::dm::static_verifier::_verify_type(const type_info& subject) {
    if (!_verify_type_callsig(subject)) {
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_type_callsig(const type_info& subject) {
    const bool type_has_callsig = (bool)subject.callsig();
    if (!type_has_callsig) {
        // if no callsig to check, default to returning successful
        return true;
    }
    const auto report = gen_callsig_report(subject, subject.callsig());
    const bool success =
        report.param_type_indices_are_in_bounds &&
        report.param_type_indices_specify_type_consts &&
        report.return_type_indices_are_in_bounds &&
        report.return_type_indices_specify_type_consts;
    if (!success) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_type_callsig_invalid);
    }
    if (!report.param_type_indices_are_in_bounds) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_param_type_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains out-of-bounds param type constant indices!",
            subject.fullname, deref_assert(subject.callsig()).fmt(subject.consts));
    }
    if (!report.param_type_indices_specify_type_consts) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_param_type_not_type_const);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains param type constant indices specifying non-type constant symbols!",
            subject.fullname, deref_assert(subject.callsig()).fmt(subject.consts));
    }
    if (!report.return_type_indices_are_in_bounds) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_return_type_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains out-of-bounds return type constant indices!",
            subject.fullname, deref_assert(subject.callsig()).fmt(subject.consts));
    }
    if (!report.return_type_indices_specify_type_consts) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_return_type_not_type_const);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains return type constant indices specifying non-type constant symbols!",
            subject.fullname, deref_assert(subject.callsig()).fmt(subject.consts));
    }
    return success;
}

bool yama::dm::static_verifier::_verify_constant_symbols(const type_info& subject) {
    bool success = true;
    for (const_t i = 0; i < subject.consts.size(); i++) {
        // keep iterating if we find a failure so as to get a chance
        // to log all issues found
        if (!_verify_constant_symbol(subject, i)) {
            success = false;
            continue;
        }
    }
    return success;
}

bool yama::dm::static_verifier::_verify_constant_symbol(const type_info& subject, const_t index) {
    if (!_verify_constant_symbol_callsig(subject, index)) {
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_constant_symbol_callsig(const type_info& subject, const_t index) {
    const bool constant_symbol_has_callsig = (bool)subject.consts.callsig(index);
    if (!constant_symbol_has_callsig) {
        // if no callsig to check, default to returning successful
        return true;
    }
    const auto report = gen_callsig_report(subject, subject.consts.callsig(index));
    const bool success =
        report.param_type_indices_are_in_bounds &&
        report.param_type_indices_specify_type_consts &&
        report.return_type_indices_are_in_bounds &&
        report.return_type_indices_specify_type_consts;
    if (!success) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_constsym_callsig_invalid);
    }
    if (!report.param_type_indices_are_in_bounds) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_param_type_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains out-of-bounds param type constant indices!",
            subject.fullname, subject.consts.fmt_type_const(index), index, deref_assert(subject.consts.callsig(index)).fmt(subject.consts));
    }
    if (!report.param_type_indices_specify_type_consts) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_param_type_not_type_const);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains param type constant indices specifying non-type constant symbols!",
            subject.fullname, subject.consts.fmt_type_const(index), index, deref_assert(subject.consts.callsig(index)).fmt(subject.consts));
    }
    if (!report.return_type_indices_are_in_bounds) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_return_type_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains out-of-bounds return type constant indices!",
            subject.fullname, subject.consts.fmt_type_const(index), index, deref_assert(subject.consts.callsig(index)).fmt(subject.consts));
    }
    if (!report.return_type_indices_specify_type_consts) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_callsig_return_type_not_type_const);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains return type constant indices specifying non-type constant symbols!",
            subject.fullname, subject.consts.fmt_type_const(index), index, deref_assert(subject.consts.callsig(index)).fmt(subject.consts));
    }
    return success;
}

yama::dm::static_verifier::_callsig_report yama::dm::static_verifier::gen_callsig_report(const type_info& subject, const callsig_info* callsig) {
    _callsig_report result{};
    YAMA_DEREF_SAFE(callsig) {
        for (const auto& I : callsig->params) {
            if (I >= subject.consts.size()) {
                result.param_type_indices_are_in_bounds = false;
                break;
            }
            else if (!is_type_const(subject.consts.const_type(I).value())) {
                result.param_type_indices_specify_type_consts = false;
                break;
            }
        }
        if (callsig->ret >= subject.consts.size()) {
            result.return_type_indices_are_in_bounds = false;
        }
        else if (!is_type_const(subject.consts.const_type(callsig->ret).value())) {
            result.return_type_indices_specify_type_consts = false;
        }
    }
    return result;
}

bool yama::dm::static_verifier::_verify_bcode(const type_info& subject) {
    // if no bcode on this type, return success *by default*
    if (!subject.bcode()) {
        return true;
    }
    // if not *using* bcode w/ this type, return success *by default*
    if (subject.call_fn() != bcode_call_fn) {
        return true;
    }
    // fail if bcode is illegally empty (ie. we've established that it's not allowed to be)
    if (!_verify_bcode_not_empty(subject, *subject.bcode())) {
        return false;
    }
    // build our control-flow graph
    _build_cfg(*subject.bcode());
    // perform our main *symbol execution* step, which is where we actually verify bcode validity
    if (!_verif_cfg(subject, *subject.bcode())) {
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_bcode_not_empty(const type_info& subject, const bc::code& bcode) {
    if (bcode.count() == 0) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_binary_is_empty);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} uses bcode, but it's bcode contains no instructions!",
            subject.fullname);
        return false;
    }
    return true;
}

void yama::dm::static_verifier::_build_cfg(const bc::code& bcode) {
    _build_cfg_division_points(bcode);
    _build_cfg_blocks(bcode);
}

void yama::dm::static_verifier::_build_cfg_division_points(const bc::code& bcode) {
    // NOTE: collect up 'division points', defined by:
    //          1) start/end of bcode
    //          2) jump destinations (which are not out-of-bounds)
    //          3) positions immediately after jump instrs (aka. fallthrough dests)
    //          4) positions immediately after 'exitpoint' instrs like 'ret'
    _add_start_and_end_division_points(bcode);
    for (size_t i = 0; i < bcode.count(); i++) {
        const auto instr = bcode[i];
        static_assert(bc::opcodes == 11);
        if (instr.opc == bc::opcode::ret) {
            _add_end_of_instr_division_point(bcode, i);
        }
        else if (instr.opc == bc::opcode::jump) {
            _add_end_of_instr_division_point(bcode, i);
            _add_jump_dest_division_point(bcode, i, instr.sBx);
        }
        else if (instr.opc == bc::opcode::jump_true) {
            _add_end_of_instr_division_point(bcode, i);
            _add_jump_dest_division_point(bcode, i, instr.sBx);
        }
        else if (instr.opc == bc::opcode::jump_false) {
            _add_end_of_instr_division_point(bcode, i);
            _add_jump_dest_division_point(bcode, i, instr.sBx);
        }
    }
}

void yama::dm::static_verifier::_add_start_and_end_division_points(const bc::code& bcode) {
    _cfg_division_points.emplace(0);
    _cfg_division_points.emplace(bcode.count());
}

void yama::dm::static_verifier::_add_end_of_instr_division_point(const bc::code& bcode, size_t i) {
    _cfg_division_points.emplace(i + 1);
}

void yama::dm::static_verifier::_add_jump_dest_division_point(const bc::code& bcode, size_t i, int16_t sBx) {
    if (!_jump_dest_in_bounds(bcode, i, sBx)) {
        return;
    }
    _cfg_division_points.emplace(_calc_jump_dest(i, sBx));
}

void yama::dm::static_verifier::_build_cfg_blocks(const bc::code& bcode) {
    // NOTE: the trick here is that std::set, being *ordered*, will ensure that our
    //       set of division points is organized correctly for us, while also ensuring
    //       that we avoid duplicate division points
    YAMA_ASSERT(_cfg_division_points.size() >= 2);
    for (auto it0 = _cfg_division_points.begin(), it1 = std::next(it0); it1 != _cfg_division_points.end(); it0++, it1++) {
        _add_cfg_block(bcode, *it0, *it1);
    }
    YAMA_ASSERT(_cfg_blocks.size() == _cfg_division_points.size() - 1);
}

void yama::dm::static_verifier::_add_cfg_block(const bc::code& bcode, size_t first, size_t last) {
    YAMA_ASSERT(last - first >= 1);
    _cfg_block new_block{
        .first = first,
        .last = last,
    };
    YAMA_ASSERT(!_cfg_blocks.contains(first));
    _cfg_blocks[first] = std::move(new_block);
}

bool yama::dm::static_verifier::_verif_cfg(const type_info& subject, const bc::code& bcode) {
    if (!_visit_entrypoint_block(subject, bcode)) {
        return false;
    }
    _report_dead_code_blocks(subject, bcode);
    return true;
}

void yama::dm::static_verifier::_report_dead_code_blocks(const type_info& subject, const bc::code& bcode) {
    for (const auto& I : _cfg_blocks) {
        if (I.second.processed) {
            continue;
        }
        YAMA_LOG(
            dbg(), dm_verif_c,
            "warning: detected dead code {}!",
            I.second.fmt());
    }
}

bool yama::dm::static_verifier::_visit_entrypoint_block(const type_info& subject, const bc::code& bcode) {
    // NOTE: notice that we're passing an *dummy* size_t(-1) incoming_branched_from value
    if (!_visit_block(subject, bcode, 0, _make_entrypoint_initial_reg_set(subject), size_t(-1))) {
        YAMA_LOG(
            dbg(), dm_verif_c,
            "warning: {} bcode symbolic execution aborted; there may be additional issues which hadn't yet been detected!",
            subject.fullname);
        _dump_cfg(bcode);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_visit_block(const type_info& subject, const bc::code& bcode, size_t block_instr_index, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from) {
    YAMA_ASSERT(_cfg_blocks.contains(block_instr_index));
    auto& b = _cfg_blocks.at(block_instr_index);
    return
        b.processed
        ? _visit_processed_block(subject, bcode, b, incoming_reg_set, incoming_branched_from)
        : _visit_unprocessed_block(subject, bcode, b, incoming_reg_set, incoming_branched_from);
}

bool yama::dm::static_verifier::_visit_processed_block(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from) {
    YAMA_ASSERT(incoming_branched_from < bcode.count());
    if (!_verify_no_register_coherence_violation(subject, bcode, block, incoming_reg_set, incoming_branched_from)) {
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_visit_unprocessed_block(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from) {
    block.processed = true;
    block.processed_by = incoming_branched_from;
    // use copy of incoming_reg_set for block.[initial/final]_reg_set
    block.initial_reg_set = incoming_reg_set;
    block.final_reg_set = incoming_reg_set;

    // perform symbol execution (updating block.final_reg_set)
    if (!_symbolic_exec(subject, bcode, block, incoming_reg_set, incoming_branched_from)) {
        return false;
    }
    
    // NOTE: at this point, if either the primary or fallthrough branch destinations are out-of-bounds,
    //       we're just gonna quietly *skip* them, as checking for branch validity is gonna be left
    //       to other parts of the verifier to handle

    const auto final_instr_index = block.final_instr_index();
    const auto final_instr = bcode[final_instr_index];
    
    const bool has_jump = block.final_instr_has_jump(bcode);
    const bool has_fallthrough = block.final_instr_has_fallthrough(bcode);

    if (has_jump && _jump_dest_in_bounds(bcode, final_instr_index, final_instr.sBx)) {

        const auto jump_dest = _calc_jump_dest(final_instr_index, final_instr.sBx);
        if (!_visit_block(subject, bcode, jump_dest, block.final_reg_set, final_instr_index)) {
            return false;
        }
    }
    if (has_fallthrough && block.last < bcode.count()) {

        const auto jump_dest = block.last;
        if (!_visit_block(subject, bcode, jump_dest, block.final_reg_set, final_instr_index)) {
            return false;
        }
    }
    return true;
}

bool yama::dm::static_verifier::_verify_no_register_coherence_violation(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from) {
    YAMA_ASSERT(block.initial_reg_set.size() == incoming_reg_set.size());
    bool success = true;
    for (size_t i = 0; i < block.initial_reg_set.size(); i++) {
        const auto& incoming = incoming_reg_set[i];
        const auto& expected = block.initial_reg_set[i];
        if (incoming == expected) {
            continue;
        }
        YAMA_RAISE(dbg(), dsignal::dm_verif_violates_register_coherence);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode register coherence violation: for {}, incoming register {} is type {}, but dest {} expected type {}!",
            subject.fullname, _fmt_branch(incoming_branched_from, block.first), i, incoming, block.fmt(), expected);
        // don't abort after detecting one violation, instead report for any and all registers in error
        success = false;
    }
    return success;
}

bool yama::dm::static_verifier::_symbolic_exec(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from) {
    for (size_t i = block.first; i < block.last; i++) {
        if (!_symbolic_exec_step(subject, bcode, block, incoming_reg_set, incoming_branched_from, i)) {
            return false;
        }
    }
    // NOTE: here we handle branching related checks, which we do at the *block-level*,
    //       rather than at the *instruction-level*
    if (
        block.final_instr_has_jump(bcode) &&
        !_verify_program_counter_valid_after_jump_by_sBx_offset(subject, bcode, block.final_instr_index())) {
        return false;
    }
    if (
        block.final_instr_has_fallthrough(bcode) &&
        !_verify_program_counter_valid_after_fallthrough(subject, bcode, block, block.final_instr_index())) {
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_symbolic_exec_step(const type_info& subject, const bc::code& bcode, _cfg_block& block, const _reg_set_state& incoming_reg_set, size_t incoming_branched_from, size_t i) {
    const auto instr = bcode[i];
    static_assert(bc::opcodes == 11);
    switch (instr.opc) {
    case bc::opcode::noop:
    {
        // do nothing
    }
    break;
    case bc::opcode::load_none:
    {
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_RA_is_type_none_skip_if_reinit(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
        block.final_reg_set[instr.A] = _none_type();
    }
    break;
    case bc::opcode::load_const:
    {
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_KoB_in_bounds(subject, bcode, i) &&
            _verify_KoB_is_object_const(subject, bcode, i) &&
            _verify_RA_and_KoB_agree_on_type_skip_if_reinit(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
        block.final_reg_set[instr.A] = _Ko_type(subject, instr.B);
    }
    break;
    case bc::opcode::load_arg:
    {
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_ArgB_in_bounds(subject, bcode, i) &&
            _verify_RA_and_ArgB_agree_on_type_skip_if_reinit(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
        block.final_reg_set[instr.A] = _Arg_type(subject, instr.B);
    }
    break;
    case bc::opcode::copy:
    {
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_RB_in_bounds(subject, bcode, i) &&
            _verify_RA_and_RB_agree_on_type_skip_if_reinit(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
        // NOTE: for copy, R(B) is the one that gets mutated by R(A), not the other way around
        block.final_reg_set[instr.B] = _R_type(block, instr.A);
    }
    break;
    case bc::opcode::call:
    {
        const bool valid =
            _verify_arg_registers_in_bounds(subject, bcode, i) &&
            _verify_arg_registers_have_at_least_one_object(subject, bcode, i) &&
            _verify_RA_is_legal_call_object(subject, bcode, block, i) &&
            _verify_param_arg_registers_are_correct_number_and_types(subject, bcode, block, i) &&
            _verify_RC_in_bounds(subject, bcode, i) &&
            _verify_RC_is_return_type_of_call_object_skip_if_reinit(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
        block.final_reg_set[instr.C] = _R_call_object_type_return_type(subject, block, instr.A);
    }
    break;
    case bc::opcode::call_nr:
    {
        const bool valid =
            _verify_arg_registers_in_bounds(subject, bcode, i) &&
            _verify_arg_registers_have_at_least_one_object(subject, bcode, i) &&
            _verify_RA_is_legal_call_object(subject, bcode, block, i) &&
            _verify_param_arg_registers_are_correct_number_and_types(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
    }
    break;
    case bc::opcode::ret:
    {
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_RA_is_return_type_of_this_call(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
    }
    break;
    case bc::opcode::jump:
    {
        // NOTE: see _symbolic_exec for general branch-related verif checks (which get done at *block-level*)
        // NOTE: see _visit_unprocessed_block for branch-related code which calls _visit_block (again, *block-level*)
        const bool valid = 
            // all check have been *generalized away*, lol
            true;
        if (!valid) {
            return false;
        }
    }
    break;
    case bc::opcode::jump_true:
    {
        // NOTE: see _symbolic_exec for general branch-related verif checks (which get done at *block-level*)
        // NOTE: see _visit_unprocessed_block for branch-related code which calls _visit_block (again, *block-level*)
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_RA_is_type_bool(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
    }
    break;
    case bc::opcode::jump_false:
    {
        // NOTE: see _symbolic_exec for general branch-related verif checks (which get done at *block-level*)
        // NOTE: see _visit_unprocessed_block for branch-related code which calls _visit_block (again, *block-level*)
        const bool valid =
            _verify_RA_in_bounds(subject, bcode, i) &&
            _verify_RA_is_type_bool(subject, bcode, block, i);
        if (!valid) {
            return false;
        }
    }
    break;
    default: YAMA_DEADEND; break;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    if (bcode[i].A >= subject.locals()) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) out-of-bounds!",
            subject.fullname, i, bcode[i].A);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_is_type_none(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _other = _none_type();
    if (_RA != _other) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_wrong_type);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) is type {}, but it must be {}!",
            subject.fullname, i, bcode[i].A, _RA, _other);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_is_type_none_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    return
        bcode.reinit_flag(i)
        ? true
        : _verify_RA_is_type_none(subject, bcode, block, i);
}

bool yama::dm::static_verifier::_verify_RA_is_type_bool(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _other = _bool_type();
    if (_RA != _other) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_wrong_type);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) is type {}, but it must be {}!",
            subject.fullname, i, bcode[i].A, _RA, _other);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_is_legal_call_object(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _RA_index = _find_type_const(subject, _RA).value();
    const auto _RA_kind = subject.consts.kind(_RA_index).value();
    const auto _RA_is_legal_call_object_type = is_callable(_RA_kind);
    if (!_RA_is_legal_call_object_type) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_illegal_callobj);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) is type {}, which cannot be used as call object!",
            subject.fullname, i, bcode[i].A, _RA);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_is_return_type_of_this_call(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto RA = _R_type(block, bcode[i].A);
    const auto this_return_type_index = deref_assert(subject.callsig()).ret;
    const str this_return_type = subject.consts.fullname(this_return_type_index).value();
    if (RA != this_return_type) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_wrong_type);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) is type {}, but ret expects return type {}!",
            subject.fullname, i, bcode[i].A, RA, this_return_type);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RB_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    if (bcode[i].B >= subject.locals()) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RB_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(B) (B == {}) out-of-bounds!",
            subject.fullname, i, bcode[i].B);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RC_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    if (bcode[i].C >= subject.locals()) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RC_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(C) (C == {}) out-of-bounds!",
            subject.fullname, i, bcode[i].C);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RC_is_return_type_of_call_object(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto RA = _R_type(block, bcode[i].A);
    const auto RA_index = _find_type_const(subject, RA).value();
    const auto RA_return_type_index = deref_assert(subject.consts.callsig(RA_index)).ret;
    const str RA_return_type = subject.consts.fullname(RA_return_type_index).value();
    const auto RC = _R_type(block, bcode[i].C);
    if (RC != RA_return_type) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RC_wrong_type);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) call object return type is {}, but return target R(C) (C == {}) is type {}!",
            subject.fullname, i, bcode[i].A, RA_return_type, bcode[i].C, RC);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RC_is_return_type_of_call_object_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    return
        bcode.reinit_flag(i)
        ? true
        : _verify_RC_is_return_type_of_call_object(subject, bcode, block, i);
}

bool yama::dm::static_verifier::_verify_KoB_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    if (bcode[i].B >= subject.consts.size()) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_KoB_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: Ko(B) (B == {}) out-of-bounds!",
            subject.fullname, i, bcode[i].B);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_KoB_is_object_const(const type_info& subject, const bc::code& bcode, size_t i) {
    const auto KoB_const_type = subject.consts.const_type(bcode[i].B).value();
    if (!is_object_const(KoB_const_type)) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_KoB_not_object_const);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: Ko(B) (B == {}) must be object constant, but isn't!",
            subject.fullname, i, bcode[i].B);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_ArgB_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    // NOTE: remember, 'args' passed into a call includes the call object, not just callable type's params
    const auto args = deref_assert(subject.callsig()).params.size() + 1;
    if (bcode[i].B >= args) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_ArgB_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: Arg(B) (B == {}) out-of-bounds!",
            subject.fullname, i, bcode[i].B);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_and_RB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _RB = _R_type(block, bcode[i].B);
    if (_RA != _RB) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_and_RB_types_differ);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) and R(B) (B == {}) do not agree on type ({} != {})!",
            subject.fullname, i, bcode[i].A, bcode[i].B, _RA, _RB);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_and_RB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    return
        bcode.reinit_flag(i)
        ? true
        : _verify_RA_and_RB_agree_on_type(subject, bcode, block, i);
}

bool yama::dm::static_verifier::_verify_RA_and_KoB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _KoB = _Ko_type(subject, bcode[i].B);
    if (_RA != _KoB) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_and_KoB_types_differ);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) and Ko(B) (B == {}) do not agree on type ({} != {})!",
            subject.fullname, i, bcode[i].A, bcode[i].B, _RA, _KoB);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_and_KoB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    return
        bcode.reinit_flag(i)
        ? true
        : _verify_RA_and_KoB_agree_on_type(subject, bcode, block, i);
}

bool yama::dm::static_verifier::_verify_RA_and_ArgB_agree_on_type(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto _RA = _R_type(block, bcode[i].A);
    const auto _ArgB = _Arg_type(subject, bcode[i].B);
    if (_RA != _ArgB) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_RA_and_ArgB_types_differ);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: R(A) (A == {}) and Arg(B) (B == {}) do not agree on type ({} != {})!",
            subject.fullname, i, bcode[i].A, bcode[i].B, _RA, _ArgB);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_RA_and_ArgB_agree_on_type_skip_if_reinit(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    return
        bcode.reinit_flag(i)
        ? true
        : _verify_RA_and_ArgB_agree_on_type(subject, bcode, block, i);
}

bool yama::dm::static_verifier::_verify_arg_registers_in_bounds(const type_info& subject, const bc::code& bcode, size_t i) {
    const auto args_index = bcode[i].A;
    const auto args_count = bcode[i].B;
    if (!internal::range_contains<size_t>(0, subject.locals(), size_t(args_index), size_t(args_index) + size_t(args_count))) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_ArgRs_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: args registers [{}, {}) out-of-bounds!",
            subject.fullname, i, args_index, args_index + args_count);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_arg_registers_have_at_least_one_object(const type_info& subject, const bc::code& bcode, size_t i) {
    if (bcode[i].B == 0) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_ArgRs_zero_objects);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: no args registers (ie. B == 0)!",
            subject.fullname, i);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_param_arg_registers_are_correct_number_and_types(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    const auto args_index = bcode[i].A;
    const auto args_count = bcode[i].B;
    const auto callobj_type = _R_type(block, args_index);
    const auto callobj_index = _find_type_const(subject, callobj_type).value();
    const auto& callobj_params = deref_assert(subject.consts.callsig(callobj_index)).params;
    YAMA_ASSERT(args_count >= 1);
    YAMA_ASSERT(internal::range_contains<size_t>(0, subject.locals(), args_index, args_index + args_count));
    const auto param_args_index = args_index + 1;
    const auto param_args_count = args_count - 1;
    const auto expected_param_args_count = callobj_params.size();
    if (param_args_count != expected_param_args_count) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_ParamArgRs_wrong_number);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: param args register count is {}, but expected {}!",
            subject.fullname, i, param_args_count, expected_param_args_count);
        return false;
    }
    // keep iterating to report all errors involving improper param arg types
    bool success = true;
    for (size_t j = 0; j < param_args_count; j++) {
        const auto param_arg_index = param_args_index + j;
        const auto param_arg = _R_type(block, param_arg_index);
        const auto expected_param_arg_index = callobj_params[j];
        const str expected_param_arg = subject.consts.fullname(expected_param_arg_index).value();
        if (param_arg == expected_param_arg) {
            continue;
        }
        if (success) { // <- only raise dsignal once
            YAMA_RAISE(dbg(), dsignal::dm_verif_ParamArgRs_wrong_types);
        }
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: param arg register {} is type {}, but expected {}!",
            subject.fullname, i, j, param_arg, expected_param_arg);
        success = false;
    }
    return success;
}

bool yama::dm::static_verifier::_verify_program_counter_valid_after_jump_by_sBx_offset(const type_info& subject, const bc::code& bcode, size_t i) {
    if (!_jump_dest_in_bounds(bcode, i, bcode[i].sBx)) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_puts_PC_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: jump by sBx (sBx == {}) would put program counter out-of-bounds!",
            subject.fullname, i, bcode[i].sBx);
        return false;
    }
    return true;
}

bool yama::dm::static_verifier::_verify_program_counter_valid_after_fallthrough(const type_info& subject, const bc::code& bcode, _cfg_block& block, size_t i) {
    if (block.last >= bcode.count()) {
        YAMA_RAISE(dbg(), dsignal::dm_verif_fallthrough_puts_PC_out_of_bounds);
        YAMA_LOG(
            dbg(), dm_verif_c,
            "error: {} bcode instr {}: {} fallthrough would put program counter out-of-bounds!",
            subject.fullname, i, block.fmt());
        return false;
    }
    return true;
}

yama::dm::static_verifier::_reg_set_state yama::dm::static_verifier::_make_entrypoint_initial_reg_set(const type_info& subject) {
    _reg_set_state result{};
    result.resize(subject.locals(), _none_type());
    return result;
}

yama::str yama::dm::static_verifier::_none_type() {
    return "None"_str;
}

yama::str yama::dm::static_verifier::_bool_type() {
    return "Bool"_str;
}

yama::str yama::dm::static_verifier::_R_type(const _cfg_block& block, size_t index) {
    return block.final_reg_set[index];
}

yama::str yama::dm::static_verifier::_R_call_object_type_return_type(const type_info& subject, const _cfg_block& block, size_t index) {
    const auto RA_type = _R_type(block, index);
    const auto RA_index = _find_type_const(subject, RA_type).value();
    const auto RA_return_type_index = deref_assert(subject.consts.callsig(RA_index)).ret;
    return subject.consts.fullname(RA_return_type_index).value();
}

yama::str yama::dm::static_verifier::_Ko_type(const type_info& subject, size_t index) {
    static_assert(const_types == 7);
    switch (subject.consts.const_type(index).value()) {
    case const_type::int0:          return "Int"_str;                               break;
    case const_type::uint:          return "UInt"_str;                              break;
    case const_type::float0:        return "Float"_str;                             break;
    case const_type::bool0:         return "Bool"_str;                              break;
    case const_type::char0:         return "Char"_str;                              break;
    case const_type::function_type: return subject.consts.fullname(index).value();  break;
    default:                        YAMA_DEADEND;                                   break;
    }
    return str();
}

yama::str yama::dm::static_verifier::_Arg_type(const type_info& subject, size_t index) {
    if (index == 0) { // callobj arg
        return subject.fullname;
    }
    else { // param arg
        return subject.consts.fullname(deref_assert(subject.callsig()).params[index - 1]).value();
    }
}

size_t yama::dm::static_verifier::_calc_jump_dest(size_t i, int16_t sBx) {
    return i + 1 + std::make_signed_t<size_t>(sBx);
}

bool yama::dm::static_verifier::_jump_dest_in_bounds(const bc::code& bcode, size_t i, int16_t sBx) {
    // NOTE: due to sBx being 16-bit, we 100% know that if sBx being negative results
    //       in integer underflow, that the result will be greater than bcode.count(),
    //       as 16-bit range isn't big enough to cause it to reach all the way back
    //       to the realistic size bcode.count() could be
    return _calc_jump_dest(i, sBx) < bcode.count();
}

std::optional<size_t> yama::dm::static_verifier::_find_type_const(const type_info& subject, const str& x) {
    // NOTE: need to do a O(n)-time search of the constant table
    for (size_t i = 0; i < subject.consts.size(); i++) {
        if (subject.consts.fullname(i) == x) {
            return std::make_optional(i);
        }
    }
    return std::nullopt;
}

