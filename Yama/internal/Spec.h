

#pragma once


#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <taul/hashing.h>

#include "../yama/yama.h"
#include "../yama/scalars.h"
#include "../yama++/macros.h"


namespace _ym {


	class RedirectSet;
	class SpecSolver;


	// TODO: I'm not particularly happy w/ Spec at the moment, and I think we should
	//		 revise it sooner rather than later.

	class Spec final {
	public:
		enum class Type : YmUInt8 {
			Path,
			Type,
		};


		Spec() = delete;
		~Spec() noexcept = default;
		Spec(const Spec&) = default;
		Spec(Spec&&) noexcept = default;
		Spec& operator=(const Spec&) = default;
		Spec& operator=(Spec&&) noexcept = default;

		static std::optional<Spec> path(const std::string& path, SpecSolver& solver);
		static std::optional<Spec> path(const std::string& path);
		static std::optional<Spec> type(const std::string& fullname, SpecSolver& solver);
		static std::optional<Spec> type(const std::string& fullname);
		static std::optional<Spec> either(const std::string& specifier, SpecSolver& solver);
		static std::optional<Spec> either(const std::string& specifier);

		static Spec pathFast(std::string normalizedPath);
		static Spec typeFast(std::string normalizedFullname);


		bool operator==(const Spec&) const noexcept = default;
		inline bool operator==(const std::convertible_to<std::string_view> auto& other) const noexcept {
			return string() == std::string_view(other);
		}
		std::strong_ordering operator<=>(const Spec& other) const noexcept;


		const std::string& string() const noexcept;
		inline operator const std::string& () const noexcept { return string(); }
		inline operator std::string_view () const noexcept { return string(); }
		inline operator const char* () const noexcept { return string().c_str(); }

		// Returns the path/fullname of the specifier (ie. the callsuff is removed.)
		std::string_view base() const noexcept;
		std::optional<std::string_view> callsuff() const noexcept;

		Type type() const noexcept;
		bool isPath() const noexcept;
		bool isType() const noexcept;

		Spec& assertPath() noexcept;
		const Spec& assertPath() const noexcept;
		Spec& assertType() noexcept;
		const Spec& assertType() const noexcept;
		Spec& assertNoCallSuff() noexcept;
		const Spec& assertNoCallSuff() const noexcept;

		size_t hash() const noexcept;
		std::string fmt() const;

		Spec transformed(RedirectSet* redirects, YmParcel* here = nullptr, YmType* typeParamsCtx = nullptr, YmType* self = nullptr) const;
		Spec removeCallSuff() const;


	private:
		std::string _spec;
		Type _type;


		Spec(std::string s, Type t);
	};
}

YM_FORMATTER(_ym::Spec, x.fmt());

// NOTE: See https://www.youtube.com/shorts/ovXsleVu6eQ.
//          * Doesn't mention the need to also overload std::equal_to w/ is_transparent.

namespace std {
	template<>
	struct hash<_ym::Spec> final {
		using is_transparent = void;
		size_t operator()(const _ym::Spec& x) const {
			return taul::hash((std::string_view)x.string());
		}
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
	template<>
	struct equal_to<_ym::Spec> final {
		using is_transparent = void;
		bool operator()(const _ym::Spec& lhs, const _ym::Spec& rhs) const {
			return lhs == rhs;
		}
		bool operator()(const _ym::Spec& lhs, const std::string& rhs) const {
			return lhs.string() == rhs;
		}
		bool operator()(const _ym::Spec& lhs, const std::string_view& rhs) const {
			return lhs.string() == rhs;
		}
		bool operator()(const _ym::Spec& lhs, const char* rhs) const {
			return lhs.string() == rhs;
		}
	};
}

