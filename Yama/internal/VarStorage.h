

#pragma once


#include <unordered_map>

#include "../yama/yama.h"

#include "obj-ref-helpers.h"


namespace _ym {


	class VarStorage final {
	public:
		VarStorage(YmCtx& ctx);
		~VarStorage() noexcept;
		

		bool isInit(YmType& varType) const noexcept;
		_ym::TempRef fetch(YmType& varType) const noexcept; // Returns borrowed ref.
		_ym::TempRef pull(YmType& varType); // Returns borrowed ref.
		bool push(YmType& varType, _ym::TempRef what, bool initVar);
		void initialize(YmType& varType);
		void reset() noexcept;


	private:
		YmCtx* _ctx;
		std::unordered_map<const YmType*, _ym::InternalRef> _storage;
	};
}

