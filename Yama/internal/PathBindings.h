

#pragma once


#include <memory>
#include <unordered_map>

#include "YmParcel.h"
#include "Spec.h"


namespace _ym {


	class PathBindings final {
	public:
		PathBindings() = default;


		std::shared_ptr<YmParcel> get(const Spec& path) const;
		void set(const Spec& path, std::shared_ptr<YmParcel> x);
		void reset() noexcept;


	private:
		std::unordered_map<Spec, std::shared_ptr<YmParcel>> _bindings;
	};
}

