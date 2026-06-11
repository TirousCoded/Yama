

#pragma once


#include "Safe.h"


namespace ym {


	// A monad encapsulating a raw pointer exposed with a std::optional-esque interface.
	template<typename T>
	class Raw final {
	public:
		inline Raw(T* x) noexcept : _ptr(x) {}
		inline Raw(T& x) noexcept : Raw(&x) {}
		inline Raw(std::nullptr_t) noexcept : Raw() {}

		Raw() = default;
		Raw(const Raw&) = default;
		Raw(Raw&&) noexcept = default;
		Raw& operator=(const Raw&) = default;
		Raw& operator=(Raw&&) noexcept = default;


		inline T* get() const noexcept { return _ptr; }
		inline operator T* () const noexcept { return get(); }

		inline explicit operator Safe<T>() { return Safe(get()); }

		inline bool hasValue() const noexcept { return get(); }
		inline operator bool() const noexcept { return hasValue(); }

		inline T& value() {
			if (!*this) throw std::runtime_error("Bad Raw<T> access!");
			return **this;
		}
		template<typename U = std::remove_cv_t<T>>
		inline T valueOr(U&& defaultValue) const {
			return
				hasValue()
				? value()
				: static_cast<T>(std::forward<U>(defaultValue));
		}

		inline T& operator*() noexcept { return get(); }
		inline T* operator->() noexcept { return get(); }

		bool operator==(const Raw&) const noexcept = default;
		std::strong_ordering operator<=>(const Raw&) const noexcept = default;

		inline size_t hash() const noexcept {
			return ym::hash(get());
		}
		inline std::string fmt() const {
			return std::format("{}", (void*)get());
		}


	private:
		T* _ptr = nullptr;
	};
}

template<typename T>
struct std::hash<ym::Raw<T>> {
	inline size_t operator()(const ym::Raw<T>& x) const noexcept {
		return x.hash();
	}
};

template<typename T>
struct std::formatter<ym::Raw<T>> : std::formatter<std::string> {
	auto format(const ym::Raw<T>& x, format_context& ctx) const {
		return formatter<string>::format(x.fmt(), ctx);
	}
};
namespace std {
	template<typename T>
	inline std::ostream& operator<<(std::ostream& stream, const ym::Raw<T>& x) {
		return stream << x.fmt();
	}
}

