

#include "YmItem.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_CONFORMS_LOG 0


const std::string& YmItem::path() const noexcept {
    return parcel->path;
}

const std::string& YmItem::fullname() const noexcept {
    return _fullname;
}

const std::string& YmItem::localName() const noexcept {
    return info->localName;
}

YmItem* YmItem::owner() noexcept {
    return
        ymKind_IsMember(kind())
        ? constAs<_ym::ConstType::Ref>(info->owner.value()).get()
        : nullptr;
}

const YmItem* YmItem::owner() const noexcept {
    return
        ymKind_IsMember(kind())
        ? constAs<_ym::ConstType::Ref>(info->owner.value()).get()
        : nullptr;
}

ym::Safe<YmItem> YmItem::self() noexcept {
    auto found = owner();
    return ym::Safe(found ? found : this);
}

ym::Safe<const YmItem> YmItem::self() const noexcept {
    auto found = owner();
    return ym::Safe(found ? found : this);
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

YmItemParams YmItem::itemParams() const noexcept {
    return self()->info->itemParamCount();
}

YmItem* YmItem::itemParam(YmItemParamIndex index) const noexcept {
    return
        index < itemParams()
        ? self()->itemArgs[size_t(index)].get()
        : nullptr;
}

YmItem* YmItem::itemParam(const std::string& name) const noexcept {
    if (auto found = self()->info->queryItemParam(name)) {
        return self()->itemArgs[size_t(found->index)];
    }
    return nullptr;
}

YmItem* YmItem::itemParamConstraint(YmItemParamIndex index) const noexcept {
    return
        index < itemParams()
        ? self()->constAsRef(self()->info->itemParams[size_t(index)]->constraint).get()
        : nullptr;
}

YmItem* YmItem::itemParamConstraint(const std::string& name) const noexcept {
    if (auto found = self()->info->queryItemParam(name)) {
        return self()->constAsRef(self()->info->itemParams[size_t(found->index)]->constraint);
    }
    return nullptr;
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
        fullname(),
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
        fullname(),
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
    // TODO: The below method impl may do things like heap alloc, and w/ that plus the variable amount
    //       of work it does generally, do look into ways to optimize w/ domain/context-level caching.
#if _DUMP_CONFORMS_LOG
    ym::println("YmItem::conforms: {} vs. {}", fullname(), protocol->fullname());
#endif
    ymAssert(protocol->kind() == YmKind_Protocol);
    auto compare = [](YmItem& pMemb, _ym::ConstIndex constIndOfItemInPMemb, YmItem& match, YmItem& itemInMatch) -> bool {
        auto& symOfItemInPMemb = pMemb.info->consts[constIndOfItemInPMemb].as<_ym::RefInfo>().sym;
        if (!_ym::specifierHasSelf(symOfItemInPMemb)) {
            // If symOfItemInPMemb contains no $Self, then simply compare YmItem* values.
            auto& itemInPMemb = pMemb.constAs<_ym::ConstType::Ref>(constIndOfItemInPMemb);
#if _DUMP_CONFORMS_LOG
            ym::println("YmItem::conforms:     {} vs. {}", itemInMatch.fullname(), itemInPMemb->fullname());
#endif
            return itemInPMemb == itemInMatch;
        }
        else {
            // If symOfItemInPMemb contains $Self, then solve ref sym to item in pMemb such that $Self is
            // substituted w/ match.fullname() (+ other needed substitutions.)
            auto solvedSpecOfItemInPMemb = _ym::SpecSolver(pMemb.parcel, pMemb.self(), match.self())(symOfItemInPMemb);
#if _DUMP_CONFORMS_LOG
            ym::println("YmItem::conforms:     {} vs. {} (from {})", itemInMatch.fullname(), solvedSpecOfItemInPMemb.value_or("**Error**"), symOfItemInPMemb);
#endif
            return solvedSpecOfItemInPMemb == itemInMatch.fullname();
        }
        };
    // Check for each member req in protocol.
    for (YmMemberIndex i = 0; i < protocol->members(); i++) {
        auto& pMemb = ym::deref(protocol->member(i));
        auto pMembName = (std::string)pMemb.info->memberName().value();
#if _DUMP_CONFORMS_LOG
        ym::println("YmItem::conforms: Matching \"{}\".", pMembName);
#endif
        // Check that a matching item can be found for each member req in protocol.
        if (auto match = member(pMembName)) {
#if _DUMP_CONFORMS_LOG
            ym::println("YmItem::conforms: Return Types:");
#endif
            // Check return types.
            if (!compare(pMemb, ym::deref(pMemb.info->returnType), *match, ym::deref(match->returnType()))) {
                return false;
            }
#if _DUMP_CONFORMS_LOG
            ym::println("YmItem::conforms: Param Count: {} vs. {}", match->params(), pMemb.params());
            ym::println("YmItem::conforms: Params:");
#endif
            // Check param counts.
            if (pMemb.params() != match->params()) {
                return false;
            }
            for (YmParamIndex j = 0; j < match->params(); j++) {
                // Check param types.
                if (!compare(pMemb, pMemb.info->params[j].type, *match, ym::deref(match->paramType(j)))) {
                    return false;
                }
            }
        }
        else {
#if _DUMP_CONFORMS_LOG
            ym::println("YmItem::conforms: Match not found!", pMembName);
#endif
            return false;
        }
    }
#if _DUMP_CONFORMS_LOG
    ym::println("YmItem::conforms: Conforms!");
#endif
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
    ymAssert(info->consts.isRef(index));
    if (ref) {
        _consts[index] = _ym::Const::byIndex<size_t(_ym::ConstType::Ref)>(ym::Safe(ref));
    }
}

void YmItem::_initConstsArrayToDummyIntConsts() {
    ymAssert(_consts.empty());
    // Initialize _consts array to correct size w/ dummy int constants.
    _consts.resize(info->consts.size(), _ym::Const::byType<YmInt>(0));
}

void YmItem::_initFullname() {
    const auto [owner, memberExt] = _ym::split_s<YmChar>(localName(), "::", true);
    std::string argPack{};
    if (!itemArgs.empty()) {
        argPack += "[";
        bool first = true;
        for (const auto& arg : itemArgs) {
            if (!first) {
                argPack += ", ";
            }
            argPack += arg->fullname();
            first = false;
        }
        argPack += "]";
    }
    _fullname = std::format("{}:{}{}{}", path(), (std::string)owner, argPack, (std::string)memberExt);
}

void YmItem::_initFullname(YmItem& owner) {
    const auto [_, memberExt] = _ym::split_s<YmChar>(localName(), "::", true);
    _fullname = std::format("{}{}", owner.fullname(), memberExt);
}

