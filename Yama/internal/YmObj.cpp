

#include "YmObj.h"
#include "YmType.h"


YmObj::YmObj(YmCtx& ctx, YmType& type) :
	ctx(ctx),
	type(type) {
}

void YmObj::cleanup() noexcept {
	if (isProtocol()) {
		ctx->release(ym::deref(boxed())); // Can't forget!
	}
}

YmObj::Slot& YmObj::slot(size_t index) noexcept {
	return _ym::ObjHAL::element(*this, index);
}

const YmObj::Slot& YmObj::slot(size_t index) const noexcept {
	return _ym::ObjHAL::element(*this, index);
}

bool YmObj::isProtocol() const noexcept {
	return type->kind() == YmKind_Protocol;
}

bool YmObj::isNone() const noexcept {
	return type == ctx->loader->ldNone();
}

bool YmObj::isInt() const noexcept {
	return type == ctx->loader->ldInt();
}

bool YmObj::isUInt() const noexcept {
	return type == ctx->loader->ldUInt();
}

bool YmObj::isFloat() const noexcept {
	return type == ctx->loader->ldFloat();
}

bool YmObj::isBool() const noexcept {
	return type == ctx->loader->ldBool();
}

bool YmObj::isRune() const noexcept {
	return type == ctx->loader->ldRune();
}

bool YmObj::isType() const noexcept {
	return type == ctx->loader->ldType();
}

std::optional<YmInt> YmObj::toInt() const noexcept {
	return isInt() ? std::make_optional(slot(0).i) : std::nullopt;
}

std::optional<YmUInt> YmObj::toUInt() const noexcept {
	return isUInt() ? std::make_optional(slot(0).ui) : std::nullopt;
}

std::optional<YmFloat> YmObj::toFloat() const noexcept {
	return isFloat() ? std::make_optional(slot(0).f) : std::nullopt;
}

std::optional<YmBool> YmObj::toBool() const noexcept {
	return isBool() ? std::make_optional(slot(0).b) : std::nullopt;
}

std::optional<YmRune> YmObj::toRune() const noexcept {
	return isRune() ? std::make_optional(slot(0).r) : std::nullopt;
}

YmType* YmObj::toType() const noexcept {
	return isType() ? slot(0).type : nullptr;
}

void YmObj::box(ym::Safe<YmObj> value, const ym::Safe<YmType>* ptable) noexcept {
	if (isProtocol()) {
		slot(0) = Slot{ .ref = value }; // We don't incr refs, instead stealing one.
		slot(1) = Slot{ .ptable = ptable };
	}
}

YmObj* YmObj::boxed() const noexcept {
	return
		isProtocol()
		? slot(0).ref
		: nullptr;
}

const ym::Safe<YmType>* YmObj::ptable() const noexcept {
	return
		isProtocol()
		? slot(1).ptable
		: nullptr;
}

