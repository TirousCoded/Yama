

#pragma once


#include <memory>
#include <string>
#include <unordered_map>

#include "YmParcel.h"


namespace _ym {


	class PathBindings final {
	public:
		PathBindings() = default;


		std::shared_ptr<YmParcel> get(const std::string& path) const;
		void set(const std::string& path, std::shared_ptr<YmParcel> x);
		void reset() noexcept;


	private:
		std::unordered_map<std::string, std::shared_ptr<YmParcel>> _bindings;
	};
}

