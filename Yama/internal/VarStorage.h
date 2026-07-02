

#pragma once


#include <unordered_map>

#include "../yama/yama.h"
#include "ParcelInfo.h"


namespace _ym {


	class VarStorage final {
	public:
		VarStorage(YmCtx& ctx);
		~VarStorage() noexcept;
		

		bool isInit(YmType& varType) const noexcept;
		YmObj* fetch(YmType& varType, YmRefPolicy returnPolicy) const noexcept;
		YmObj* pull(YmType& varType, YmRefPolicy returnPolicy);
		bool push(YmType& varType, YmObj* what, YmRefPolicy whatPolicy, bool initVar);
		void initialize(YmType& varType);
		void reset() noexcept;


	private:
		YmCtx* _ctx;
		std::unordered_map<const TypeInfo*, YmObj*> _storage;
	};
}

