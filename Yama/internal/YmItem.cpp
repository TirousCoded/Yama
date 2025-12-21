

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

YmItem* YmItem::owner() const noexcept {
    return
        ymKind_IsMember(kind())
        ? constAs<_ym::ConstType::Ref>(info->owner.value()).get()
        : nullptr;
}

YmMembers YmItem::members() const noexcept {
    return info->memberCount();
}

YmItem* YmItem::member(YmMemberIndex member) const noexcept {
    return
        member < members()
        ? constAs<_ym::ConstType::Ref>(info->membersByIndex[member]).get()
        : nullptr;
}

YmItem* YmItem::member(const std::string& name) const noexcept {
    auto it = info->membersByName.find(name);
    return
        it != info->membersByName.end()
        ? constAs<_ym::ConstType::Ref>(it->second).get()
        : nullptr;
}

YmItem* YmItem::returnType() const noexcept {
    return
        info->returnType
        ? constAs<_ym::ConstType::Ref>(*info->returnType).get()
        : nullptr;
}

const YmChar* YmItem::paramName(YmParamIndex param) const {
    if (auto p = info->queryParam(param)) {
        return p->name.c_str();
    }
    _ym::Global::raiseErr(
        YmErrCode_ParamNotFound,
        "Cannot query parameter name; item {}; no parameter found at index {}!",
        fullname,
        param);
    return nullptr;
}

YmItem* YmItem::paramType(YmParamIndex param) const {
    if (auto p = info->queryParam(param)) {
        return constAs<_ym::ConstType::Ref>(p->type);
    }
    _ym::Global::raiseErr(
        YmErrCode_ParamNotFound,
        "Cannot query parameter type; item {}; no parameter found at index {}!",
        fullname,
        param);
    return nullptr;
}

YmItem* YmItem::ref(YmRef reference) const noexcept {
    return
        size_t(reference) < consts().size() && consts()[reference].is<ym::Safe<YmItem>>()
        ? consts()[reference].as<ym::Safe<YmItem>>().get()
        : nullptr;
}

std::optional<YmRef> YmItem::findRef(ym::Safe<YmItem> referenced) const noexcept {
    for (size_t i = 0; i < consts().size(); i++) {
        auto& c = consts()[i];
        if (_ym::constTypeOf(c) != _ym::ConstType::Ref) continue;
        if (c.as<ym::Safe<YmItem>>() != referenced) continue;
        return YmRef(i);
    }
    return std::nullopt;
}

bool YmItem::conforms(ym::Safe<YmItem> protocol) const noexcept {
    ymAssert(protocol->kind() == YmKind_Protocol);
    for (YmMemberIndex i = 0; i < protocol->members(); i++) {
        const auto expects = ym::Safe(protocol->member(i));
        if (const auto found = member((std::string)expects->info->memberName().value())) {
            if (expects->info->returnTypeIsSelf()) {
                if (found->returnType() != found->owner()) {
                    return false;
                }
            }
            else if (found->returnType() != expects->returnType()) {
                return false;
            }
            if (found->params() != expects->params()) {
                return false;
            }
            for (YmParamIndex j = 0; j < found->params(); j++) {
                if (expects->info->paramTypeIsSelf(j)) {
                    if (found->paramType(j) != found->owner()) {
                        return false;
                    }
                }
                else if (found->paramType(j) != expects->paramType(j)) {
                    return false;
                }
            }
        }
        else return false;
    }
    return true;
}

std::span<const _ym::Const> YmItem::consts() const noexcept {
    return std::span(_consts);
}

void YmItem::putValConst(size_t index) {
    ymAssert(index < info->consts.size());
    ymAssert(info->consts.isVal(index));
    static_assert(_ym::ConstTypes == 6);
    switch (_ym::constTypeOf(info->consts[index])) {
    case _ym::ConstType::Int:   _putValConstAs<YmInt>(index);   break;
    case _ym::ConstType::UInt:  _putValConstAs<YmUInt>(index);  break;
    case _ym::ConstType::Float: _putValConstAs<YmFloat>(index); break;
    case _ym::ConstType::Bool:  _putValConstAs<YmBool>(index);  break;
    case _ym::ConstType::Rune:  _putValConstAs<YmRune>(index);  break;
    default:                    YM_DEADEND;                     break;
    }
}

void YmItem::putRefConst(size_t index, YmItem* ref) {
    ymAssert(index < info->consts.size());
    ymAssert(_ym::constTypeOf(info->consts[index]) == _ym::ConstType::Ref);
    if (ref) {
        _consts[index] = _ym::Const::byType<ym::Safe<YmItem>>(ym::Safe(ref));
    }
}

void YmItem::_initConstsArrayToDummyIntConsts() {
    ymAssert(_consts.empty());
    // Initialize _consts array to correct size w/ dummy int constants.
    _consts.resize(info->consts.size(), _ym::Const::byType<YmInt>(0));
}

