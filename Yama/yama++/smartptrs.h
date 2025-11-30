

#pragma once


#include <memory>

#include "../yama/yama.h"
#include "Safe.h"


namespace ym {


	// TODO: Below have not been unit tested.


	using ScopedDm = std::unique_ptr<YmDm, decltype(&ymDm_Destroy)>;
	using ScopedCtx = std::unique_ptr<YmCtx, decltype(&ymCtx_Destroy)>;
	using ScopedParcelDef = std::unique_ptr<YmParcelDef, decltype(&ymParcelDef_Destroy)>;


	// These bind*** methods exist to enable the creation of RAII objects to automate releasing
	// of already existing resource pointers.

	inline ScopedDm bindScoped(Safe<YmDm> resource) noexcept {
		return ScopedDm(resource, ymDm_Destroy);
	}
	inline ScopedCtx bindScoped(Safe<YmCtx> resource) noexcept {
		return ScopedCtx(resource, ymCtx_Destroy);
	}
	inline ScopedParcelDef bindScoped(Safe<YmParcelDef> resource) noexcept {
		return ScopedParcelDef(resource, ymParcelDef_Destroy);
	}


	inline ScopedDm makeDm() {
		return bindScoped(Safe(ymDm_Create()));
	}
	inline ScopedCtx makeCtx(Safe<YmDm> dm) {
		return bindScoped(Safe(ymCtx_Create(dm)));
	}
	inline ScopedParcelDef makeParcelDef() {
		return bindScoped(Safe(ymParcelDef_Create()));
	}
}

