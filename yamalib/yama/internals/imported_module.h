

#pragma once


#include "../core/res.h"
#include "../core/ids.h"
#include "../core/module.h"


namespace yama::internal {


	struct imported_module final {
		res<module> m;
		mid_t id;
	};
}

