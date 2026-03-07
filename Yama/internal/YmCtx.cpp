

#include "YmCtx.h"

#include "general.h"
#include "SpecSolver.h"
#include "YmObj.h"
#include "YmParcel.h"
#include "YmType.h"


YmCtx::YmCtx(ym::Safe<YmDm> domain) :
    domain(domain),
    loader(std::make_shared<_ym::CtxLoader>(domain->loader)) {}

YmCtx::~YmCtx() noexcept {
    ymAssert(objects == 0);
    reset(); // Cleanup
}

std::shared_ptr<YmParcel> YmCtx::import(const std::string& path) {
    if (auto s = _ym::Spec::path(path)) {
        return loader->import(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Import failed; \"{}\" syntax error!",
            path);
        return nullptr;
    }
}

std::shared_ptr<YmType> YmCtx::load(const std::string& fullname) {
    if (auto s = _ym::Spec::type(fullname)) {
        return loader->load(*s);
    }
    else {
        _ym::Global::raiseErr(
            YmErrCode_IllegalSpecifier,
            "Load failed; \"{}\" syntax error!",
            fullname);
        return nullptr;
    }
}

YmObj* YmCtx::create(YmType& type) {
    objects++;
    auto al = mas.allocator<int>();
    ym::Safe result(_ym::ObjHAL::create(YmObj(*this, type), al));
    result->refs.addRef();
    ymAssert(result->refs.count() == 1);
    return result;
}

YmRefCount YmCtx::secure(YmObj& obj) {
    return obj.refs.addRef();
}

YmRefCount YmCtx::release(YmObj& obj) {
    auto old = obj.refs.drop();
    if (old == 1) {
        objects--;
        auto al = mas.allocator<int>();
        _ym::ObjHAL::destroy(obj, al);
    }
    return old;
}

void YmCtx::reset() {
    // TODO
}

ym::Safe<YmObj> YmCtx::newNone() {
    return ym::Safe(create(loader->ldNone()));
}

ym::Safe<YmObj> YmCtx::newInt(YmInt v) {
    auto result = ym::Safe(create(loader->ldInt()));
    result->slot(0).i = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newUInt(YmUInt v) {
    auto result = ym::Safe(create(loader->ldUInt()));
    result->slot(0).ui = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newFloat(YmFloat v) {
    auto result = ym::Safe(create(loader->ldFloat()));
    result->slot(0).f = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newBool(YmBool v) {
    auto result = ym::Safe(create(loader->ldBool()));
    result->slot(0).b = v;
    return result;
}

ym::Safe<YmObj> YmCtx::newRune(YmRune v) {
    auto result = ym::Safe(create(loader->ldRune()));
    result->slot(0).r = v;
    return result;
}

