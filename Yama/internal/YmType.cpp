

#include "YmType.h"

#include "general.h"
#include "SpecSolver.h"


#define _DUMP_CONFORMS_LOG 0

#if _DUMP_CONFORMS_LOG
#include "../yama++/print.h"
#endif


YmKind YmType::kind() const noexcept {
    return info->kind();
}

const _ym::Spec& YmType::path() const noexcept {
    return parcel->path;
}

const _ym::Spec& YmType::fullname() const noexcept {
    return _fullname;
}

const std::string& YmType::localName() const noexcept {
    return info->localName();
}

bool YmType::isRegular() const noexcept {
    return info->isRegular();
}

bool YmType::isIrregular() const noexcept {
    return info->isIrregular();
}

bool YmType::isPrimitive() const noexcept {
    return info->isPrimitive();
}

bool YmType::isGetter() const noexcept {
    return info->isGetter();
}

bool YmType::isSetter() const noexcept {
    return info->isSetter();
}

bool YmType::isVarLike() const noexcept {
    return info->isVarLike();
}

bool YmType::isProtocolReq() const noexcept {
    return info->isProtocolReq();
}

bool YmType::hasCallSig() const noexcept {
    return info->hasCallSig();
}

bool YmType::isOwner() const noexcept {
    return info->isOwner();
}

bool YmType::isMember() const noexcept {
    return info->isMember();
}

bool YmType::canHaveMembers() const noexcept {
    return info->canHaveMembers();
}

bool YmType::hasDefaultValue() const noexcept {
    return info->hasDefaultValue();
}

bool YmType::isStruct() const noexcept {
    return info->isStruct();
}

bool YmType::isProtocol() const noexcept {
    return info->isProtocol();
}

bool YmType::isFn() const noexcept {
    return info->isFn();
}

bool YmType::isVar() const noexcept {
    return info->isVar();
}

bool YmType::isVarAssigner() const noexcept {
    return info->isVarAssigner();
}

bool YmType::isMethod() const noexcept {
    return info->isMethod();
}

bool YmType::isProperty() const noexcept {
    return info->isProperty();
}

bool YmType::isPropertyAssigner() const noexcept {
    return info->isPropertyAssigner();
}

bool YmType::isRegularStruct() const noexcept {
    return info->isRegularStruct();
}

bool YmType::isRegularProtocol() const noexcept {
    return info->isRegularProtocol();
}

bool YmType::isRegularFn() const noexcept {
    return info->isRegularFn();
}

bool YmType::isRegularVar() const noexcept {
    return info->isRegularVar();
}

bool YmType::isRegularVarAssigner() const noexcept {
    return info->isRegularVarAssigner();
}

bool YmType::isRegularMethod() const noexcept {
    return info->isRegularMethod();
}

bool YmType::isRegularProperty() const noexcept {
    return info->isRegularProperty();
}

bool YmType::isRegularPropertyAssigner() const noexcept {
    return info->isRegularPropertyAssigner();
}

bool YmType::isNone() const noexcept {
    return info->isNone();
}

bool YmType::isInt() const noexcept {
    return info->isInt();
}

bool YmType::isUInt() const noexcept {
    return info->isUInt();
}

bool YmType::isFloat() const noexcept {
    return info->isFloat();
}

bool YmType::isBool() const noexcept {
    return info->isBool();
}

bool YmType::isRune() const noexcept {
    return info->isRune();
}

bool YmType::isType() const noexcept {
    return info->isType();
}

bool YmType::isMethodReq() const noexcept {
    return info->isMethodReq();
}

bool YmType::isStoredVarGet() const noexcept {
    return info->isStoredVarGet();
}

bool YmType::isStoredVarSet() const noexcept {
    return info->isStoredVarSet();
}

bool YmType::isStoredPropertyGet() const noexcept {
    return info->isStoredPropertyGet();
}

bool YmType::isStoredPropertySet() const noexcept {
    return info->isStoredPropertySet();
}

bool YmType::isCallable() const noexcept {
    return
        info->hasCallSig() &&
        (isMethodReq() ? isObjMethod() : true);
}

bool YmType::isTypeMethod() const noexcept {
    return owner() && !hasSelfParam();
}

bool YmType::isObjMethod() const noexcept {
    return owner() && hasSelfParam();
}

std::optional<std::string> YmType::callsuff() const {
    if (!info->hasCallSig()) {
        return std::nullopt;
    }
    std::string result{};
    result += "(";
    for (YmParams i = 0; i < positionalParams(); i++) {
        if (i >= 1) {
            result += ", ";
        }
        result += param(i)->type().fullname().string();
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

YmType* YmType::var() noexcept {
    return constAsRef(info->varConst());
}

const YmType* YmType::var() const noexcept {
    return constAsRef(info->varConst());
}

YmType* YmType::owner() noexcept {
    return
        isMember()
        ? constAsRef(info->ownerConst().value()).get()
        : nullptr;
}

const YmType* YmType::owner() const noexcept {
    return
        isMember()
        ? constAsRef(info->ownerConst().value()).get()
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

YmTypeParams YmType::typeParams() const noexcept {
    // Can't forget the 'self()->' part!
    return self()->info->typeParams();
}

std::optional<YmType::TypeParam> YmType::typeParam(YmTypeParamIndex index) const noexcept {
    // Can't forget the 'self()->' part!
    return _mkHelper<TypeParam>(self()->info->typeParam(index));
}

std::optional<YmType::TypeParam> YmType::typeParam(const std::string& name) const noexcept {
    // Can't forget the 'self()->' part!
    return _mkHelper<TypeParam>(self()->info->typeParam(name));
}

YmMembers YmType::members() const noexcept {
    return info->members();
}

std::optional<YmType::Member> YmType::member(YmMemberIndex index) const noexcept {
    return _mkHelper<Member>(info->member(index));
}

std::optional<YmType::Member> YmType::member(const std::string& name) const noexcept {
    return _mkHelper<Member>(info->member(name));
}

YmType* YmType::returnType() const noexcept {
    return constAsRef(info->returnTypeConst());
}

YmParams YmType::params() const noexcept {
    return info->params();
}

YmParams YmType::positionalParams() const noexcept {
    return info->positionalParams();
}

YmParams YmType::namedParams() const noexcept {
    return info->namedParams();
}

std::optional<YmType::Param> YmType::param(YmParamIndex index) const noexcept {
    return _mkHelper<Param>(info->param(index));
}

std::optional<YmType::Param> YmType::param(const std::string& name) const noexcept {
    return _mkHelper<Param>(info->param(name));
}

bool YmType::hasSelfParam() const noexcept {
    return positionalParams() >= 1 && param(0)->isSelfParam();
}

YmType* YmType::assigner() const noexcept {
    return constAsRef(info->assignerConst());
}

YmType* YmType::initializer() const noexcept {
    return constAsRef(info->initializerConst());
}

YmType* YmType::ref(YmRef reference) const noexcept {
    return
        size_t(reference) < _refs.size()
        ? _refs[reference]
        : nullptr;
}

bool YmType::depends(ym::Safe<YmType> other) const noexcept {
    for (auto& c : consts()) {
        if (_ym::constTypeOf(c) != _ym::ConstType::Ref) continue;
        if (c.as<ym::Safe<YmType>>() != other) continue;
        return true;
    }
    return false;
}

bool YmType::conforms(ym::Safe<YmType> protocol) const noexcept {
    // TODO: The below method impl may do things like heap alloc, and w/ that plus the variable amount
    //       of work it does generally, do look into ways to optimize w/ domain/context-level caching.
#if _DUMP_CONFORMS_LOG
    ym::println("YmType::conforms: {} vs. {}", fullname(), protocol->fullname());
#endif
    ymAssert(protocol->isProtocol());
    if (info->hasCallSig()) {
#if _DUMP_CONFORMS_LOG
        ym::println("YmType::conforms:     Failed; {} is a type with a call signature!", fullname());
#endif
        return false;
    }
    auto compare = [](YmType& pMemb, _ym::ConstIndex constIndOfTypeInPMemb, YmType& match, YmType& typeInMatch) -> bool {
        auto& symOfTypeInPMemb = pMemb.info->consts[constIndOfTypeInPMemb].as<_ym::RefInfo>().sym;
        if (!_ym::specifierHasSelf(symOfTypeInPMemb)) {
            // If symOfTypeInPMemb contains no $Self, then simply compare YmType* values.
            auto& typeInPMemb = *pMemb.constAsRef(constIndOfTypeInPMemb);
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms:     {} vs. {}", typeInMatch.fullname(), typeInPMemb->fullname());
#endif
            return typeInPMemb.sameAs(typeInMatch);
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
#if _DUMP_CONFORMS_LOG
        ym::println("YmType::conforms: Matching \"{}\".", pMemb.info->memberName());
#endif
        // Check that a matching type can be found for each member req in protocol.
        if (auto match = member(pMemb.name())) {
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms: Return Types:");
#endif
            // Check return types.
            if (!compare(
                pMemb.type(),
                pMemb.type().info->returnTypeConst().value(),
                match->type(),
                ym::deref(match->type().returnType()))) {
                return false;
            }
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms: Positional Param Count: {} vs. {}", match->params(), pMemb.params());
            ym::println("YmType::conforms: Positional Params:");
#endif
            // Check positional param counts.
            if (pMemb.type().positionalParams() != match->type().positionalParams()) {
                return false;
            }
            for (YmParamIndex j = 0; j < match->type().positionalParams(); j++) {
                // Check positional param types.
                if (!compare(
                    pMemb.type(),
                    pMemb.type().info->param(j)->typeConst,
                    match->type(),
                    match->type().param(j)->type())) {
                    return false;
                }
            }
        }
        else {
#if _DUMP_CONFORMS_LOG
            ym::println("YmType::conforms: Match not found!", pMemb.info->memberName());
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

void YmType::buildRefs() {
    for (size_t i = 0; i < info->refs.size(); i++) {
        if (auto ref = _consts.at(info->refs.at(i)).tryAs<ym::Safe<YmType>>()) {
            _refs.push_back(ref->get());
        }
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
    _fullname = _ym::Spec::typeFast(std::format("{}{}", owner.fullname(), (std::string)memberExt));
}

const decltype(YmType::typeArgs)& YmType::_getTypeArgs() const noexcept {
    return (isMember() ? owner() : this)->typeArgs;
}

YmType& YmType::TypeParam::constraint() const noexcept {
    return *self->constAsRef(info->constraintConst);
}

YmType& YmType::TypeParam::arg() const noexcept {
    return *self->_getTypeArgs()[size_t(index())];
}

YmType& YmType::Member::type() const noexcept {
    return *self->constAsRef(info->typeConst);
}

YmType& YmType::Param::type() const noexcept {
    return *self->constAsRef(info->typeConst);
}

bool YmType::Param::isSelfParam() const noexcept {
    return
        index() == 0 &&
        isPositional() &&
        type().sameAs(self->owner());
}

