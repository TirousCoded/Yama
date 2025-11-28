

#include "YmItem.h"

#include "general.h"


std::string_view YmItem::path() const noexcept {
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    return path;
}

std::string_view YmItem::localName() const noexcept {
    const auto [path, localName] = _ym::split_s<YmChar>(fullname, ":");
    ymAssert(!localName.empty());
    return localName;
}

std::span<const _ym::Const> YmItem::consts() const noexcept {
    return std::span(_consts);
}

void YmItem::putValConst(YmConst index) {
    ymAssert(index < info->consts.size());
    ymAssert(_ym::isValConstType(YmConstType(info->consts[index].index())));
    static_assert(YmConstType_Num == 6);
    switch (info->consts[index].index()) {
    case YmConstType_Int:   _putValConstAs<YmInt>(index);   break;
    case YmConstType_UInt:  _putValConstAs<YmUInt>(index);  break;
    case YmConstType_Float: _putValConstAs<YmFloat>(index); break;
    case YmConstType_Bool:  _putValConstAs<YmBool>(index);  break;
    case YmConstType_Rune:  _putValConstAs<YmRune>(index);  break;
    default:                YM_DEADEND;                     break;
    }
}

void YmItem::putRefConst(YmConst index, YmItem* ref) {
    ymAssert(index < info->consts.size());
    ymAssert(YmConstType(info->consts[index].index()) == YmConstType_Ref);
    if (ref) {
        _consts[index] = _ym::Const::byType<ym::Safe<YmItem>>(ym::Safe(ref));
    }
}

void YmItem::_initConstsArrayToDummyIntConsts() {
    ymAssert(_consts.empty());
    ymAssert(info->consts.size() <= YmWord(YM_MAX_CONST));
    // Initialize _consts array to correct size w/ dummy int constants.
    _consts.resize(info->consts.size(), _ym::Const::byType<YmInt>(0));
}

