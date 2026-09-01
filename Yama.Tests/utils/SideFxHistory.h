

#pragma once


#include <format>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include <yama/yama.h>
#include <yama++/scalar.h>
#include <yama++/print.h>
#include <yama++/Variant.h>


namespace {
	inline void compFloatsHelper(YmFloat lhs, YmFloat rhs, bool& result) {
		result = false;
		// This'll abort the fn if it fails.
		ASSERT_DOUBLE_EQ(lhs, rhs);
		// If we reach here, then we know lhs == rhs.
		result = true;
	}
	inline bool compFloats(YmFloat lhs, YmFloat rhs) {
		bool result{};
		compFloatsHelper(lhs, rhs, result);
		return result;
	}


	struct UnknownVal {
		constexpr bool operator==(const UnknownVal&) const noexcept = default;
	};
	struct NoneVal {
		constexpr bool operator==(const NoneVal&) const noexcept = default;
	};
	using Val = ym::Variant<
		UnknownVal, // For when a YmObj* doesn't have a valid Val repr.
		NoneVal,
		YmInt,
		YmUInt,
		YmFloat,
		YmBool,
		YmRune,
		YmType*
	>;
	
	inline std::string fmt(const Val& x) {
		if (x.is<UnknownVal>())					return std::format("**UNKNOWN**");
		else if (x.is<NoneVal>())				return std::format("None");
		else if (auto p = x.tryAs<YmInt>())		return std::format("Int {}", ym::fmt(*p));
		else if (auto p = x.tryAs<YmUInt>())	return std::format("UInt {}", ym::fmt(*p));
		else if (auto p = x.tryAs<YmFloat>())	return std::format("Float {}", ym::fmt(*p));
		else if (auto p = x.tryAs<YmBool>())	return std::format("Bool {}", ym::fmt(*p));
		else if (auto p = x.tryAs<YmRune>())	return std::format("Rune {}", ym::fmt(*p));
		else if (auto p = x.tryAs<YmType*>())	return std::format("Type {}", ymType_Fullname(*p));
		else									return "**INVALID**";
	}

	inline bool compare(const Val& lhs, const Val& rhs) {
		bool bothAreFloatsAndTheyMismatch =
			lhs.is<YmFloat>() &&
			rhs.is<YmFloat>() &&
			!compFloats(lhs.as<YmFloat>(), rhs.as<YmFloat>());
		return bothAreFloatsAndTheyMismatch || lhs != rhs;
	}

	inline Val toVal(YmObj* obj) {
		if (obj) {
			auto fln = ymType_Fullname(ymObj_Type(obj));
			if (fln == "yama:None")		return NoneVal{};
			if (fln == "yama:Int")		return ymObj_ToInt(obj, nullptr);
			if (fln == "yama:UInt")		return ymObj_ToUInt(obj, nullptr);
			if (fln == "yama:Float")	return ymObj_ToFloat(obj, nullptr);
			if (fln == "yama:Bool")		return ymObj_ToBool(obj, nullptr);
			if (fln == "yama:Rune")		return ymObj_ToRune(obj, nullptr);
			if (fln == "yama:Type")		return ymObj_ToType(obj, nullptr);
		}
		return UnknownVal{};
	}


	class SideFxHistory final {
	public:
		SideFxHistory() = default;
		~SideFxHistory() noexcept = default;
		SideFxHistory(const SideFxHistory&) = default;
		SideFxHistory(SideFxHistory&&) noexcept = default;
		SideFxHistory& operator=(const SideFxHistory&) = default;
		SideFxHistory& operator=(SideFxHistory&&) noexcept = default;


		inline size_t size() const noexcept {
			return _vals.size();
		}
		inline const Val& val(size_t index) const {
			return _vals.at(index);
		}

		inline SideFxHistory& observe(Val x) {
			_vals.emplace_back(std::move(x));
			return *this;
		}
		inline SideFxHistory& observeNone() {
			return observe(NoneVal{});
		}
		inline SideFxHistory& observeInt(YmInt x) {
			return observe(Val(x));
		}
		inline SideFxHistory& observeUInt(YmUInt x) {
			return observe(Val(x));
		}
		inline SideFxHistory& observeFloat(YmFloat x) {
			return observe(Val(x));
		}
		inline SideFxHistory& observeBool(YmBool x) {
			return observe(Val(x));
		}
		inline SideFxHistory& observeRune(YmRune x) {
			return observe(Val(x));
		}
		inline SideFxHistory& observeType(YmType* x) {
			return observe(Val(x));
		}

		inline bool expectEq(const SideFxHistory& other) const {
			std::string report = "SideFxHistory::expectEq";
			bool sameSize = size() == other.size();
			bool success = sameSize;
			report += std::format("\n  Size: {} vs. {}{}", size(), other.size(),
				sameSize ? "" : " (Mismatch)");
			for (size_t i = 0; i < std::min(size(), other.size()); i++) {
				bool sameVal = compare(val(i), other.val(i));
				success = success && sameVal;
				report += std::format("\n  {} vs. {}{}", ::fmt(val(i)), ::fmt(other.val(i)),
					sameVal ? "" : " (Mismatch)");
			}
			if (!success) {
				ym::println("{}", report);
			}
			return success;
		}

		inline std::string fmt() const {
			std::string result = "SideFxHistory";
			result += std::format("\n  Size: {}", size());
			for (const auto& val : _vals) {
				result += std::format("\n  {}", val);
			}
			return result;
		}


	private:
		std::vector<Val> _vals;
	};


	// Used to observe side effects from sidefx parcel.
	SideFxHistory sidefx;

	inline void clearSideFx() {
		sidefx = SideFxHistory{};
	}

	inline YmParcelDef* mkSideFxParcelDef() {
		YmParcelDef* p = ymParcelDef_Create();
		if (p) {
			ymParcelDef_AddFn(p, "observeNone", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeNone();
				},
				nullptr);

			ymParcelDef_AddFn(p, "observeInt", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeInt(ymObj_ToInt(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeInt", "value", "yama:Int");

			ymParcelDef_AddFn(p, "observeUInt", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeUInt(ymObj_ToUInt(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeUInt", "value", "yama:UInt");

			ymParcelDef_AddFn(p, "observeFloat", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeFloat(ymObj_ToFloat(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeFloat", "value", "yama:Float");

			ymParcelDef_AddFn(p, "observeBool", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeBool(ymObj_ToBool(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeBool", "value", "yama:Bool");

			ymParcelDef_AddFn(p, "observeRune", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeRune(ymObj_ToRune(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeRune", "value", "yama:Rune");

			ymParcelDef_AddFn(p, "observeType", "yama:None",
				[](YmCtx* ctx, YmType* type, void*) {
					sidefx.observeType(ymObj_ToType(ymCtx_Arg(ctx, 0, YM_BORROW), nullptr));
				},
				nullptr);
			ymParcelDef_AddParam(p, "observeType", "value", "yama:Type");
		}
		else ADD_FAILURE();
		return p;
	}
	inline void setupSideFxParcel(YmDm* dm) {
		if (!dm) FAIL();
		if (auto p = mkSideFxParcelDef()) {
			ymDm_BindParcelDef(dm, "sidefx", p);
			ymParcelDef_Release(p);
		}
		else FAIL();
	}
}



