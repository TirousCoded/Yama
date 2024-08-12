

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
    // if type has a callsig, check whether its param and return type indices
    // are in-bounds
    if (!_verify_type_callsig_indices(subject)) {
        return false;
    }
    // check each link symbol to see if it has a callsig, and whether or not
    // what is found is correct for the kind of type it is
    if (!_verify_linksym_callsigs(subject)) {
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

inline bool yama::dm::static_verifier::_verify_type_callsig_indices(const type_info& subject) {
    const bool type_has_callsig = (bool)subject.callsig();
    if (!type_has_callsig) {
        // if no callsig to check, default to returning successful
        return true;
    }
    if (!subject.callsig()->verify_indices(subject.linksyms)) {
        YAMA_LOG(
            dbg(), static_verif_c, 
            "error: {} callsig (expressed using link symbols) {} contains out-of-bounds link indices!", 
            subject.fullname, subject.callsig()->fmt(subject.linksyms));
        return false;
    }
    return true;
}

inline bool yama::dm::static_verifier::_verify_linksym_callsigs(const type_info& subject) {
    bool success = true;
    for (link_index i = 0; i < subject.linksyms.size(); i++) {
        // keep iterating if we find a failure so as to get a chance
        // to log all issues found
        if (!_verify_linksym_callsig(subject, i)) {
            success = false;
            continue;
        }
        if (!_verify_linksym_callsig_indices(subject, i)) {
            success = false;
            continue;
        }
    }
    return success;
}

inline bool yama::dm::static_verifier::_verify_linksym_callsig(const type_info& subject, link_index index) {
    const auto& linksym = subject.linksyms[index];
    const bool type_has_callsig = (bool)linksym.callsig;
    const bool kind_uses_callsig = is_callable(linksym.kind);
    if (type_has_callsig && !kind_uses_callsig) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} link symbol {} (at link index {}) has a callsig, but for {} types this is illegal!",
            subject.fullname, linksym.fullname, index, subject.kind());
    }
    if (!type_has_callsig && kind_uses_callsig) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} link symbol {} (at link index {}) has no callsig, but for {} types this is illegal!",
            subject.fullname, linksym.fullname, index, subject.kind());
    }
    return type_has_callsig == kind_uses_callsig;
}

inline bool yama::dm::static_verifier::_verify_linksym_callsig_indices(const type_info& subject, link_index index) {
    const auto& linksym = subject.linksyms[index];
    const bool linksym_has_callsig = (bool)linksym.callsig;
    if (!linksym_has_callsig) {
        // if no callsig to check, default to returning successful
        return true;
    }
    if (!linksym.callsig->verify_indices(subject.linksyms)) {
        YAMA_LOG(
            dbg(), static_verif_c,
            "error: {} link symbol {} (at link index {}) callsig (expressed using link symbols) {} contains out-of-bounds link indices!",
            subject.fullname, linksym.fullname, index, linksym.callsig->fmt(subject.linksyms));
        return false;
    }
    return true;
}

