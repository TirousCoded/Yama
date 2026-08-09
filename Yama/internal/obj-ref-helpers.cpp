

#include "obj-ref-helpers.h"

#include "YmObj.h"


_ym::TempRef::TempRef(YmObj* ref, YmRefPolicy policy, bool frontend) noexcept :
	_ref(ref),
	_policy(policy),
	// Only meaningful for non-borrowed.
	_frontend(policy != YM_BORROW ? frontend : bool{}) {
}

_ym::TempRef::TempRef(std::nullptr_t) noexcept :
	TempRef() {
}

_ym::TempRef::~TempRef() noexcept {
	drop();
}

_ym::TempRef::TempRef(TempRef&& other) noexcept :
	TempRef() {
	std::swap(_ref, other._ref);
	std::swap(_policy, other._policy);
	std::swap(_frontend, other._frontend);
}

_ym::TempRef& _ym::TempRef::operator=(TempRef&& other) noexcept {
	drop();
	std::swap(_ref, other._ref);
	std::swap(_policy, other._policy);
	std::swap(_frontend, other._frontend);
	return *this;
}

_ym::TempRef _ym::TempRef::borrow(YmObj* ref) noexcept {
	return TempRef(ref, YM_BORROW, bool{});
}

_ym::TempRef _ym::TempRef::take(YmObj* ref, bool frontend, bool secure) noexcept {
	if (!ref) {
		return nullptr;
	}
	TempRef result(ref, YM_TAKE, frontend);
	if (secure) {
		result->ctx->secure(*result, frontend);
	}
	return result;
}

_ym::TempRef _ym::TempRef::takeIfOk(YmObj* ref, bool frontend, bool secure) noexcept {
	if (!ref) {
		return nullptr;
	}
	TempRef result(ref, YM_TAKE_IF_OK, frontend);
	if (secure) {
		result->ctx->secure(*result, frontend);
	}
	return result;
}

_ym::TempRef _ym::TempRef::borrowOrTake(YmObj* ref, bool frontend, bool secureIfTaken, YmRefPolicy policy) noexcept {
	return
		policy == YM_BORROW
		? borrow(ref)
		: take(ref, frontend, secureIfTaken);
}

_ym::TempRef::operator bool() const noexcept {
	return get();
}

YmObj* _ym::TempRef::get() const noexcept {
	return _ref;
}

_ym::TempRef::operator YmObj* () const noexcept {
	return get();
}

YmObj& _ym::TempRef::operator*() const noexcept {
	return ym::deref(get());
}

YmObj* _ym::TempRef::operator->() const noexcept {
	return ym::Safe(get());
}

YmRefPolicy _ym::TempRef::policy() const noexcept {
	return _policy;
}

void _ym::TempRef::drop() noexcept {
	if (_ref) {
		if (_policy == YM_TAKE) {
			_ref->ctx->release(*_ref, _frontend);
		}
		_ref = nullptr;
	}
}

void _ym::TempRef::dropAsOk() noexcept {
	if (_ref) {
		if (_policy != YM_BORROW) {
			_ref->ctx->release(*_ref, _frontend);
		}
		_ref = nullptr;
	}
}

YmObj* _ym::TempRef::consume() noexcept {
	auto result = _ref;
	_ref = nullptr;
	return result;
}

_ym::TempRef& _ym::TempRef::changeSide(bool frontend) noexcept {
	if (policy() != YM_BORROW && _frontend != frontend) {
		_ref->ctx->secure(*_ref, frontend);
		_ref->ctx->release(*_ref, _frontend);
		_frontend = frontend;
	}
	return *this;
}

_ym::LiteInternalRef::LiteInternalRef(TempRef value) noexcept :
	LiteInternalRef() {
	_ref =
		value && value.policy() == YM_BORROW
		? TempRef::take(value, false, true).consume()
		: value.changeSide(false).consume();
}

_ym::LiteInternalRef::LiteInternalRef(std::nullptr_t) noexcept :
	LiteInternalRef() {
}

_ym::LiteInternalRef::operator bool() const noexcept {
	return get();
}

YmObj* _ym::LiteInternalRef::get() const noexcept {
	return _ref;
}

_ym::LiteInternalRef::operator YmObj* () const noexcept {
	return get();
}

YmObj& _ym::LiteInternalRef::operator*() const noexcept {
	return ym::deref(get());
}

YmObj* _ym::LiteInternalRef::operator->() const noexcept {
	return ym::Safe(get());
}

_ym::TempRef _ym::LiteInternalRef::borrow() const noexcept {
	return TempRef::borrow(get());
}

_ym::TempRef _ym::LiteInternalRef::take(bool frontend) const noexcept {
	return TempRef::take(*this, frontend, true);
}

_ym::TempRef _ym::LiteInternalRef::borrowOrTake(bool shouldBorrow, bool frontend) const noexcept {
	return
		// Borrow if nullptr.
		(shouldBorrow || !*this)
		? borrow()
		: take(frontend);
}

_ym::TempRef _ym::LiteInternalRef::steal(bool frontend) noexcept {
	if (frontend) {
		// If frontend, we gotta secure a new frontend ref, release our
		// existing backend one, and return the former.
		auto result = take(true);
		drop();
		return result;
	}
	else {
		// If backend, we just return existing backend ref.
		auto result = TempRef::take(*this, false, false);
		_ref = nullptr;
		return result;
	}
}

void _ym::LiteInternalRef::drop() noexcept {
	(void)steal(false);
}

void _ym::LiteInternalRef::assign(TempRef other) noexcept {
	// NOTE: If both *this and other ref some obj X, and X has a ref count of 1,
	//		 and other is a borrowed ref, it would be WRONG to drop *this prior
	//		 to incrementing X, as then X would INCORRECTLY be released!
	//
	//		 This can easily be solved by just doing nothing if other is borrowed,
	//		 and both *this and other ref same object.
	if (get() == other && other.policy() == YM_BORROW) {
		return;
	}
	drop();
	*this = std::move(other);
}

_ym::InternalRef::InternalRef(TempRef value) noexcept :
	_ref(std::move(value)) {
}

_ym::InternalRef::InternalRef(std::nullptr_t) noexcept :
	_ref(nullptr) {
}

_ym::InternalRef::~InternalRef() noexcept {
	drop(); // RAII
}

_ym::InternalRef::InternalRef(InternalRef&& other) noexcept :
	_ref(std::move(other._ref)) {
	// NOTE: Need this as apparently LiteInternalRef's autogen move ctor DOESN'T
	//		 DO THIS AUTOMATICALLY!!!
	other._ref = nullptr;
}

_ym::InternalRef& _ym::InternalRef::operator=(InternalRef&& other) noexcept {
	// Gotta call LiteInternalRef::assign to account for an important nuance,
	// which includes us needing to prior default move-assign from problematically
	// calling dtor and dropping.
	_ref.assign(other.steal(false));
	return *this;
}

_ym::InternalRef::operator bool() const noexcept {
	return (bool)_ref;
}

YmObj* _ym::InternalRef::get() const noexcept {
	return _ref;
}

_ym::InternalRef::operator YmObj* () const noexcept {
	return _ref;
}

YmObj& _ym::InternalRef::operator*() const noexcept {
	return *_ref;
}

YmObj* _ym::InternalRef::operator->() const noexcept {
	return &*_ref; // Null checks.
}

_ym::TempRef _ym::InternalRef::borrow() const noexcept {
	return _ref.borrow();
}

_ym::TempRef _ym::InternalRef::take(bool frontend) const noexcept {
	return _ref.take(frontend);
}

_ym::TempRef _ym::InternalRef::borrowOrTake(bool shouldBorrow, bool frontend) const noexcept {
	return _ref.borrowOrTake(shouldBorrow, frontend);
}

_ym::TempRef _ym::InternalRef::steal(bool frontend) noexcept {
	return _ref.steal(frontend);
}

void _ym::InternalRef::drop() noexcept {
	_ref.drop();
}

