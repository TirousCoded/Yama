

#include "VarStorage.h"

#include "YmCtx.h"


_ym::VarStorage::VarStorage(YmCtx& ctx) :
	_ctx(&ctx) {
}

_ym::VarStorage::~VarStorage() noexcept {
}

bool _ym::VarStorage::isInit(YmType& varType) const noexcept {
	return fetch(varType);
}

_ym::TempRef _ym::VarStorage::fetch(YmType& varType) const noexcept {
	ymAssert(varType.isStoredVarGet());
	if (auto it = _storage.find(&varType); it != _storage.end()) {
		auto& [var, obj] = *it;
		return obj.borrow();
	}
	return nullptr;
}

_ym::TempRef _ym::VarStorage::pull(YmType& varType) {
	if (auto result = fetch(varType)) {
		return result;
	}
	initialize(varType);
	return fetch(varType);
}

bool _ym::VarStorage::push(YmType& varType, _ym::TempRef what, bool initVar) {
	ymAssert(varType.isStoredVarGet());
	if (!what) {
		return false;
	}
	if (initVar) {
		initialize(varType);
	}
	_storage[&varType] = std::move(what);
	return true;
}

void _ym::VarStorage::initialize(YmType& varType) {
	using namespace ym;
	if (isInit(varType)) {
		return;
	}
	ymCtx_Call(Safe(_ctx), Safe(varType.initializer()), 0, "", YM_PUSH);
	push(varType, Safe(_ctx)->pull(false), false);
}

void _ym::VarStorage::reset() noexcept {
	_storage.clear();
}

