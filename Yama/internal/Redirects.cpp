

#include "Redirects.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_LOG 0

#if _DUMP_LOG
#include "../yama++/print.h"
#endif


_ym::Spec _ym::RedirectSet::resolve(const Spec& path) const {
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
#if _DUMP_LOG
    ym::println("RedirectSet::resolve: Resolving {} ({} redirects to check).", path, _redirects.size());
#endif
    auto s = path.string();
    for (auto it = _redirects.rbegin(); it != _redirects.rend(); std::advance(it, 1)) {
        auto& [before, after] = *it;
#if _DUMP_LOG
        ym::println("RedirectSet::resolve: {} -> {}", before, after);
#endif
        if (path.string().starts_with(before.string())) {
#if _DUMP_LOG
            ym::println("RedirectSet::resolve: Match!");
#endif
            // Redirect found, so modify path, and exit loop as we found the correct redirect.
            s.replace(0, before.string().length(), after.string());
            break;
        }
    }
#if _DUMP_LOG
    ym::println("RedirectSet::resolve: Result: {}", s);
#endif
    // Return result path, whether we found a redirect for it or not.
    return Spec::pathFast(std::move(s));
}

void _ym::Redirects::add(const Spec& subject, const Spec& before, const Spec& after) {
    _redirects.insert_or_assign(
        std::make_pair(subject.assertNoCallSuff(), before.assertNoCallSuff()),
        after.assertNoCallSuff());
#if _DUMP_LOG
    ym::println("RedirectSet::add: After...");
    for (const auto& [subjectAndBefore, after] : _redirects) {
        const auto& [subject, before] = subjectAndBefore;
        ym::println("RedirectSet::add:     {} / {} -> {}", subject, before, after);
    }
#endif
}

_ym::RedirectSet _ym::Redirects::compute(const Spec& path) const {
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
        if (path.string().starts_with(subject.string())) {
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
