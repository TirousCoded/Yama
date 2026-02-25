

#include "YmType.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_CONFORMS_LOG 0


const _ym::Spec& YmType::path() const noexcept {
    return parcel->path;
}

const _ym::Spec& YmType::fullname() const noexcept {
    return _fullname;
}

const std::string& YmType::localName() const noexcept {
    return info->localName;
}

std::optional<std::string> YmType::callsuff() const {
    if (!ymKind_IsCallable(kind())) {
        return std::nullopt;
    }
    std::string result{};
    result += "(";
    for (YmParams i = 0; i < params(); i++) {
        if (i >= 1) {
            result += ", ";
        }
        result += paramType(i)->fullname().string();
    }
    result += ") -> ";
    result += returnType()->fullname().string();
    return ym::retopt(result);
}

std::optional<_ym::Spec> YmType::callsig() const {
    if (auto result = callsuff()) {
        return _ym::Spec::typeFast(fullname().string() + *result);
    }
    return std::nullopt;
}

bool YmType::checkCallSuff(std::string_view callsuff) const {
    // TODO: A heap allocation being needed here is suboptimal.
    auto ours = this->callsuff();
    return
        ours
        ? *ours == callsuff
        : true;
}

bool YmType::checkCallSuff(std::optional<std::string_view> callsuff) const {
    return
        callsuff
        ? checkCallSuff(*callsuff)
        : true;
}

YmType* YmType::owner() noexcept {
    return
        ymKind_IsMember(kind())
        ? constAs<_ym::ConstType::Ref>(info->owner.value()).get()
        : nullptr;
}

const YmType* YmType::owner() const noexcept {
    return
        ymKind_IsMember(kind())
        ? constAs<_ym::ConstType::Ref>(info->owner.value()).get()
        : nullptr;
}

ym::Safe<YmType> YmType::self() noexcept {
    auto found = owner();
    return ym::Safe(found ? found : this);
}

ym::Safe<const YmType> YmType::self() const noexcept {
    auto found = owner();
    return ym::Safe(found ? found : this);
}

YmMembers YmType::members() const noexcept {
    return info->memberCount();
}

YmType* YmType::member(YmMemberIndex member) const noexcept {
    return
        member < members()
        ? constAs<_ym::ConstType::Ref>(info->membersByIndex[member]).get()
        : nullptr;
}

YmType* YmType::member(const std::string& name) const noexcept {
    auto it = info->membersByName.find(name);
    return
        it != info->membersByName.end()
        ? constAs<_ym::ConstType::Ref>(it->second).get()
        : nullptr;
}

YmTypeParams YmType::typeParams() const noexcept {
    return self()->info->typeParamCount();
}

YmType* YmType::typeParam(YmTypeParamIndex index) const noexcept {
    return
        index < typeParams()
        ? self()->typeArgs[size_t(index)].get()
        : nullptr;
}

YmType* YmType::typeParam(const std::string& name) const noexcept {
    if (auto found = self()->info->queryTypeParam(name)) {
        return self()->typeArgs[size_t(found->index)];
    }
    return nullptr;
}

YmType* YmType::typeParamConstraint(YmTypeParamIndex index) const noexcept {
    return
        index < typeParams()
        ? self()->constAsRef(self()->info->typeParams[size_t(index)]->constraint).get()
        : nullptr;
}

YmType* YmType::typeParamConstraint(const std::string& name) const noexcept {
    if (auto found = self()->info->queryTypeParam(name)) {
        return self()->constAsRef(self()->info->typeParams[size_t(found->index)]->constraint);
    }
    return nullptr;
}

YmType* YmType::returnType() const noexcept {
    return
        info->returnType
        ? constAs<_ym::ConstType::Ref>(*info->returnType).get()
        : nullptr;
}

const YmChar* YmType::paramName(YmParamIndex param) const {
    if (auto p = info->queryParam(param)) {
        return p->name.c_str();
    }
    _ym::Global::raiseErr(
        YmErrCode_ParamNotFound,
        "Cannot query parameter name; type {}; no parameter found at index {}!",
        fullname(),
        param);
    return nullptr;
}

YmType* YmType::paramType(YmParamIndex param) const {
    if (auto p = info->queryParam(param)) {
        return constAs<_ym::ConstType::Ref>(p->type);
    }
    _ym::Global::raiseErr(
        YmErrCode_ParamNotFound,
        "Cannot query parameter type; type {}; no parameter found at index {}!",
        fullname(),
        param);
    return nullptr;
}

YmType* YmType::ref(YmRef reference) const noexcept {
    return
        size_t(reference) < consts().size() && consts()[reference].is<ym::Safe<YmType>>()
        ? consts()[reference].as<ym::Safe<YmType>>().get()
        : nullptr;
}

std::optional<YmRef> YmType::findRef(ym::Safe<YmType> referenced) const noexcept {
    for (size_t i = 0; i < consts().size(); i++) {
        auto& c = consts()[i];
        if (_ym::constTypeOf(c) != _ym::ConstType::Ref) continue;
        if (c.as<ym::Safe<YmType>>() != referenced) continue;
        return YmRef(i);
    }
    return std::nullopt;
}

bool YmType::conforms(ym::Safe<YmType> protocol) const noexcept {
    // TODO: The below method impl may do things like heap alloc, and w/ that plus the variable amount
    //       of work it does generally, do look into ways to optimize w/ domain/context-level caching.
#if _DUMP_CONFORMS_LOG
    ym::println("YmType::conforms: {} vs. {}", fullname(), protocol->fullname());
#endif
    ymAssert(protocol->kind() == YmKind_Protocol);
    auto compare = [](YmType& pMemb, _ym::ConstIndex constIndOfTypeInPMemb, YmType& match, YmType& typeInMatch) -> bool {
        auto& symOfTypeInPMemb = pMemb.info->consts[constIndOfTypeInPMemb].as<_ym::RefInfo>().sym;
        if (!_ym::specifierHasSelf(symOfTypeInPMemb)) {
            // If symOfTypeInPMemb contains no $Self, then simply compare YmType* values.
            auto& typeInPMemb = pMemb.constAs<_ym::ConstType::Ref>(constIndOfTypeInPMemb);
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms:     {} vs. {}", typeInMatch.fullname(), typeInPMemb->fullname());
#endif
            return typeInPMemb == typeInMatch;
        }
        else {
            // If symOfTypeInPMemb contains $Self, then solve ref sym to type in pMemb such that $Self is
            // substituted w/ match.fullname() (+ other needed substitutions.)
            auto solvedSpecOfTypeInPMemb = symOfTypeInPMemb.transformed(nullptr, pMemb.parcel, pMemb.self(), match.self());
#if _DUMP_CONFORMS_LOG
            // TODO: Original code has 'solvedSpecOfTypeInPMemb.value_or("**Error**")', so if ever we encounter a situation
            //       where solvedSpecOfTypeInPMemb needs to be std::nullopt, add this part back in.
            ym::println("YmType::conforms:     {} vs. {} (from {})", typeInMatch.fullname(), solvedSpecOfTypeInPMemb, symOfTypeInPMemb);
#endif
            return solvedSpecOfTypeInPMemb == typeInMatch.fullname();
        }
        };
    // Check for each member req in protocol.
    for (YmMemberIndex i = 0; i < protocol->members(); i++) {
        auto& pMemb = ym::deref(protocol->member(i));
        auto pMembName = (std::string)pMemb.info->memberName().value();
#if _DUMP_CONFORMS_LOG
        ym::println("YmType::conforms: Matching \"{}\".", pMembName);
#endif
        // Check that a matching type can be found for each member req in protocol.
        if (auto match = member(pMembName)) {
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms: Return Types:");
#endif
            // Check return types.
            if (!compare(pMemb, ym::deref(pMemb.info->returnType), *match, ym::deref(match->returnType()))) {
                return false;
            }
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms: Param Count: {} vs. {}", match->params(), pMemb.params());
            ym::println("YmType::conforms: Params:");
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
            ym::println("YmType::conforms: Match not found!", pMembName);
#endif
            return false;
        }
    }
#if _DUMP_CONFORMS_LOG
    ym::println("YmType::conforms: Conforms!");
#endif
    return true;
}

std::span<const _ym::Const> YmType::consts() const noexcept {
    return std::span(_consts);
}

void YmType::putValConst(size_t index) {
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

void YmType::putRefConst(size_t index, YmType* ref) {
    ymAssert(index < info->consts.size());
    ymAssert(info->consts.isRef(index));
    if (ref) {
        _consts[index] = _ym::Const::byIndex<size_t(_ym::ConstType::Ref)>(ym::Safe(ref));
    }
}

void YmType::_initConstsArrayToDummyIntConsts() {
    ymAssert(_consts.empty());
    // Initialize _consts array to correct size w/ dummy int constants.
    _consts.resize(info->consts.size(), _ym::Const::byType<YmInt>(0));
}

void YmType::_initFullname() {
    const auto [owner, memberExt] = _ym::split_s<YmChar>(localName(), "::", true);
    std::string argPack{};
    if (!typeArgs.empty()) {
        argPack += "[";
        bool first = true;
        for (const auto& arg : typeArgs) {
            if (!first) {
                argPack += ", ";
            }
            argPack += arg->fullname().string();
            first = false;
        }
        argPack += "]";
    }
    _fullname = _ym::Spec::typeFast(std::format("{}:{}{}{}", path(), (std::string)owner, argPack, (std::string)memberExt));
}

void YmType::_initFullname(YmType& owner) {
    const auto [_, memberExt] = _ym::split_s<YmChar>(localName(), "::", true);
    _fullname = _ym::Spec::typeFast(std::format("{}{}", owner.fullname(), memberExt));
}

