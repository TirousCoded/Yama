

#include "VarStorage.h"

#include "YmCtx.h"


_ym::VarStorage::VarStorage(YmCtx& ctx) :
	_ctx(&ctx) {
}

_ym::VarStorage::~VarStorage() noexcept {
	// End-user must manually ensure no init var data remains at dtor.
	ymAssert(_storage.empty());
}

bool _ym::VarStorage::isInit(YmType& varType) const noexcept {
	return fetch(varType, YM_BORROW);
}

YmObj* _ym::VarStorage::fetch(YmType& varType, YmRefPolicy returnPolicy) const noexcept {
	ymAssert(varType.isStoredVarGet());
	if (auto it = _storage.find(varType.info); it != _storage.end()) {
		auto& [var, obj] = *it;
		// If end-user wants to own a ref to *obj, we gotta secure one for them.
		if (returnPolicy != YM_BORROW) {
			ymObj_Secure(obj);
		}
		return obj;
	}
	return nullptr;
}

YmObj* _ym::VarStorage::pull(YmType& varType, YmRefPolicy returnPolicy) {
	if (auto result = fetch(varType, returnPolicy)) {
		return result;
	}
	initialize(varType);
	return fetch(varType, returnPolicy);
}

bool _ym::VarStorage::push(YmType& varType, YmObj* what, YmRefPolicy whatPolicy, bool initVar) {
	ymAssert(varType.isStoredVarGet());
	// NOTE: If ever push can fail while what != nullptr, and whatPolicy == YM_TAKE, we gotta
	//		 remember to release what.
	if (!what) {
		return false;
	}
	if (initVar) {
		initialize(varType);
	}
	_storage[varType.info] = what;
	// If end-user passed a borrowed ref, we gotta secure new ref for storage to own.
	if (whatPolicy == YM_BORROW) {
		ymObj_Secure(what);
	}
	return true;
}

void _ym::VarStorage::initialize(YmType& varType) {
	using namespace ym;
	if (isInit(varType)) {
		return;
	}
	ymCtx_Call(Safe(_ctx), Safe(varType.initializer()), 0, "", YM_PUSH);
	push(varType, ymCtx_Pull(Safe(_ctx)), YM_TAKE, false);
}

void _ym::VarStorage::reset() noexcept {
	for (const auto& [var, obj] : _storage) {
		ymObj_Release(obj);
	}
	_storage.clear();
}

