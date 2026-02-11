

#include "Redirects.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


std::string _ym::RedirectSet::resolve(std::string path) const {
    // NOTE: _redirects being sorted in ascending order ensures that given two entries, one of which is
    //       a subpath of the other, the subpath one is guaranteed to always appear before the other.
    // 
    //       Example:
    //          x
    //          x/y
    //          x/y/z
    //          abc
    //          abc/def
    // 
    //       This means that by iterating backwards, we'll always encounter the more specific path of
    //       the two first, letting us easily impl shadowing.
    for (auto it = _redirects.rbegin(); it != _redirects.rend(); std::advance(it, 1)) {
        auto& [before, after] = *it;
        if (path.starts_with(before)) {
            // Redirect found, so modify path, and exit loop as we found the correct redirect.
            path.replace(0, before.length(), after);
            break;
        }
    }
    // Return path, whether we found a redirect for it or not.
    return path;
}

void _ym::Redirects::add(const std::string& subject, const std::string& before, const std::string& after) {
    assertNormalNonCallSig(subject);
    assertNormalNonCallSig(before);
    assertNormalNonCallSig(after);
    _redirects.insert_or_assign(std::make_pair(subject, before), after);
}

_ym::RedirectSet _ym::Redirects::compute(const std::string& path) const {
    // NOTE: _redirects being sorted in ascending order ensures that given two entries, one of which is
    //       a subpath of the other, the subpath one is guaranteed to always appear before the other.
    // 
    //       Example:
    //          x
    //          x/y
    //          x/y/z
    //          abc
    //          abc/def
    // 
    //       This means that by having below overwrite existing before/after entries in result, more specific
    //       subject path before/after entries will shadow those of less specific subject paths.
    RedirectSet result{};
    for (const auto& [subjectAndBefore, after] : _redirects) {
        const auto& [subject, before] = subjectAndBefore;
        if (path.starts_with(subject)) {
            result._redirects.insert_or_assign(before, after);
        }
    }
#if _DUMP_LOG
    ym::println("Redirects::compute: Printing redirects (path={}):", path);
    for (const auto& [before, after] : result._redirects) {
        ym::println("Redirects::compute:     {} -> {}", before, after);
    }
#endif
    return result;
}
