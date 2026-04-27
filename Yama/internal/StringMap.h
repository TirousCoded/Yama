

#pragma once


#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <taul/hashing.h>


namespace _ym {


	// NOTE: See https://www.youtube.com/shorts/ovXsleVu6eQ.
	//          * Doesn't mention the need to also overload std::equal_to w/ is_transparent.

	struct StringMapHash final {
		using is_transparent = void;

		size_t operator()(const std::string& x) const {
			return taul::hash((std::string_view)x);
		}
		size_t operator()(const std::string_view& x) const {
			return taul::hash(x);
		}
		size_t operator()(const char* x) const {
			return taul::hash((std::string_view)x);
		}
	};
	struct StringMapEq final {
		using is_transparent = void;

		bool operator()(const std::string& lhs, const std::string& rhs) const {
			return lhs == rhs;
		}
		bool operator()(const std::string& lhs, const std::string_view& rhs) const {
			return lhs == rhs;
		}
		bool operator()(const std::string& lhs, const char* rhs) const {
			return lhs == rhs;
		}
	};


	template<typename T, typename Allocator = std::allocator<std::pair<const std::string, T>>>
	using StringMap = std::unordered_map<std::string, T, StringMapHash, StringMapEq, Allocator>;

	template<typename T, typename Allocator = std::allocator<std::pair<const std::string, T>>>
	using StringMultiMap = std::unordered_multimap<std::string, T, StringMapHash, StringMapEq, Allocator>;

	template<typename Allocator = std::allocator<std::string>>
	using StringSet = std::unordered_set<std::string, StringMapHash, StringMapEq, Allocator>;

	template<typename Allocator = std::allocator<std::string>>
	using StringMultiSet = std::unordered_multiset<std::string, StringMapHash, StringMapEq, Allocator>;
}

