

#include "YmObj.h"
#include "YmType.h"


YmObj::YmObj(YmCtx& ctx, YmType& type) :
	ctx(ctx),
	type(type) {
}

YmObj::Slot& YmObj::slot(size_t index) noexcept {
	return _ym::ObjHAL::element(*this, index);
}

const YmObj::Slot& YmObj::slot(size_t index) const noexcept {
	return _ym::ObjHAL::element(*this, index);
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

