

#include "yama.h"

#include "../internal/general.h"


struct YmDm final : public _ym::Resource {
public:
    YmDm() : Resource(_ym::RType::Domain) {}


    static _ym::Safe<YmDm> create() { return _ym::Safe(new YmDm()); }
    static void destroy(_ym::Safe<YmDm> x) noexcept { delete x.get(); }
};

struct YmCtx final : public _ym::Resource {
public:
    const _ym::Safe<YmDm> domain;


    YmCtx(_ym::Safe<YmDm> domain) : Resource(_ym::RType::Context), domain(domain) {}


    static _ym::Safe<YmCtx> create(_ym::Safe<YmDm> domain) { return _ym::Safe(new YmCtx(domain)); }
    static void destroy(_ym::Safe<YmCtx> x) noexcept { delete x.get(); }
};

void _ymAssert(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line) {
    if (cond == YM_TRUE) return;
    _ym::println("Yama Assert Failed:\ncond: {}\nfile: {}\nline: {}", cond_txt, file, line);
    _ym::debugbreak();
}

void _ymVerify(YmBool cond, const YmChar* cond_txt, const YmChar* file, YmWord line) {
    if (cond == YM_TRUE) return;
    _ym::println("Yama Assert Failed:\ncond: {}\nfile: {}\nline: {}", cond_txt, file, line);
    _ym::debugbreak();
}

void _ymDrop(void* x) {
    auto res = _ym::Safe<_ym::Resource>(x);
    switch (res->rtype()) {
    case _ym::RType::Domain:
    {
        _ym::destroy(res.into<YmDm>());
    }
    break;
    case _ym::RType::Context:
    {
        _ym::destroy(res.into<YmCtx>());
    }
    break;
    default: YM_DEADEND; break;
    }
}

YmDm* ymNewDm(void) {
    return YmDm::create();
}

YmCtx* ymNewCtx(YmDm* domain) {
    return YmCtx::create(_ym::Safe(domain));
}

