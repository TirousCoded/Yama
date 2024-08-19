

#include "static_verifier.h"

#include "../core/kind-features.h"


yama::dm::static_verifier::static_verifier(std::shared_ptr<debug> dbg) 
    : api_component(dbg) {}

bool yama::dm::static_verifier::verify(const type_info& subject) {
    _begin_verify(subject);
    const bool success = _verify(subject);
    _end_verify(success);
    return success;
}

bool yama::dm::static_verifier::_verify(const type_info& subject) {
    if (!_verify_type(subject)) {
        return false;
    }
    if (!_verify_constant_symbols(subject)) {
        return false;
    }
    return true;
}

void yama::dm::static_verifier::_begin_verify(const type_info& subject) {
    YAMA_LOG(dbg(), static_verif_c, "static verif. {} type {}...", subject.kind(), subject.fullname);
}

void yama::dm::static_verifier::_end_verify(bool success) {
    if (success) {
        YAMA_LOG(dbg(), static_verif_c, "static verif. success!");
    }
    else {
        YAMA_LOG(dbg(), static_verif_c, "static verif. failure!");
    }
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
    if (!report.indices_are_in_bounds) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains out-of-bounds constant indices!",
            subject.fullname, subject.callsig()->fmt(subject.consts));
    }
    if (!report.indices_specify_type_consts) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} callsig (expressed using constant symbols) {} contains constant indices specifying non-type constant symbols!",
            subject.fullname, subject.callsig()->fmt(subject.consts));
    }
    return
        report.indices_are_in_bounds &&
        report.indices_specify_type_consts;
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
    if (!report.indices_are_in_bounds) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains out-of-bounds constant indices!",
            subject.fullname, subject.consts.fmt_type_const(index), index, subject.consts.callsig(index)->fmt(subject.consts));
    }
    if (!report.indices_specify_type_consts) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} type constant symbol {} (at constant index {}) callsig (expressed using constant symbols) {} contains constant indices specifying non-type constant symbols!",
            subject.fullname, subject.consts.fmt_type_const(index), index, subject.consts.callsig(index)->fmt(subject.consts));
    }
    return
        report.indices_are_in_bounds &&
        report.indices_specify_type_consts;
}

yama::dm::static_verifier::callsig_report yama::dm::static_verifier::gen_callsig_report(const type_info& subject, const callsig_info* callsig) {
    callsig_report result{};
    YAMA_DEREF_SAFE(callsig) {
        for (const auto& I : callsig->params) {
            if (I >= subject.consts.size()) {
                result.indices_are_in_bounds = false;
                break;
            }
            else if (!is_type_const(subject.consts.const_type(I).value())) {
                result.indices_specify_type_consts = false;
                break;
            }
        }
        if (callsig->ret >= subject.consts.size()) {
            result.indices_are_in_bounds = false;
        }
        else if (!is_type_const(subject.consts.const_type(callsig->ret).value())) {
            result.indices_specify_type_consts = false;
        }
    }
    return result;
}

