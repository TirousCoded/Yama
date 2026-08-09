

#include "YmObj.h"
#include "YmType.h"


YmObj::YmObj(YmCtx& ctx, YmType& type) :
	ctx(ctx),
	type(type) {
}

_ym::Slots YmObj::slots() const noexcept {
	return type->slots();
}

YmObj::Slot& YmObj::slot(_ym::Slots index) noexcept {
	ymAssert(index < slots());
	return _ym::ObjHAL::element(*this, index);
}

const YmObj::Slot& YmObj::slot(_ym::Slots index) const noexcept {
	ymAssert(index < slots());
	return _ym::ObjHAL::element(*this, index);
}

const _ym::LiteInternalRef& YmObj::refSlot(_ym::Slots index) const noexcept {
	ymAssert(type->checkIsRefSlot(index));
	return slot(index).ref;
}

void YmObj::dropRefSlot(_ym::Slots index) noexcept {
	ymAssert(type->checkIsRefSlot(index));
	slot(index).ref.drop();
}

void YmObj::dropAllRefSlots() noexcept {
	forEachRefSlotIndex([this](_ym::Slots index) {
		dropRefSlot(index);
		});
}

void YmObj::assignRefSlot(_ym::Slots index, _ym::TempRef value) noexcept {
	ymAssert(type->checkIsRefSlot(index));
	slot(index).ref.assign(std::move(value));
}

_ym::TempRef YmObj::stealRefSlot(_ym::Slots index, bool frontendRef) noexcept {
	ymAssert(type->checkIsRefSlot(index));
	return slot(index).ref.steal(frontendRef);
}

std::optional<YmInt> YmObj::toInt() const noexcept {
	return type->isInt() ? std::make_optional(slot(0).i) : std::nullopt;
}

std::optional<YmUInt> YmObj::toUInt() const noexcept {
	return type->isUInt() ? std::make_optional(slot(0).ui) : std::nullopt;
}

std::optional<YmFloat> YmObj::toFloat() const noexcept {
	return type->isFloat() ? std::make_optional(slot(0).f) : std::nullopt;
}

std::optional<YmBool> YmObj::toBool() const noexcept {
	return type->isBool() ? std::make_optional(slot(0).b) : std::nullopt;
}

std::optional<YmRune> YmObj::toRune() const noexcept {
	return type->isRune() ? std::make_optional(slot(0).r) : std::nullopt;
}

YmType* YmObj::toType() const noexcept {
	return type->isType() ? slot(0).type : nullptr;
}

void YmObj::box(_ym::TempRef value, const ym::Safe<YmType>* ptable) noexcept {
	if (type->isProtocol()) {
		assignRefSlot(0, std::move(value));
		slot(1) = Slot{ .ptable = ptable };
	}
}

_ym::TempRef YmObj::boxed() const noexcept {
	return
		type->isProtocol()
		? refSlot(0).borrow()
		: nullptr;
}

const ym::Safe<YmType>* YmObj::ptable() const noexcept {
	return
		type->isProtocol()
		? slot(1).ptable
		: nullptr;
}

