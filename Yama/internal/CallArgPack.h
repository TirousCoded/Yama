

#pragma once


#include <array>
#include <optional>

#include "../yama/yama.h"
#include "YmType.h"


namespace _ym {


	// Input arg packs specify positional args, followed by named args, the ladder of which
	// is arbitrary in terms of what named args are specified, and in what order.
	// Within the call itself however, the call behaviour expects positional args and named
	// args to ALL be specified, and w/ named args being in a specific order.
	// This class encapsulates input arg pack info, and also a mapping between the param layout
	// expected by the call behaviour, and the input arg pack.
	// As part of this mapping, 'dummy' args are implicitly bound to all named params not
	// otherwise specified.
	// These dummy args correspond to special dummy arg objects which get appended to the
	// original input arg pack before the call begins to provide bindings for unspecified
	// named params.
	class CallArgPack final {
	public:
		inline CallArgPack(YmParams positionalParams, YmParams namedParams) :
			_positionalArgs(positionalParams),
			_dummies(namedParams) {
			_namedParamBindsMap.fill(_unbound);
		}
		inline CallArgPack(YmType& fn) :
			CallArgPack(fn.positionalParamCount(), fn.namedParamCount()) {
		}
		// Inits w/ 0 positional, named and dummy args.
		inline CallArgPack() {
			_namedParamBindsMap.fill(_unbound);
		}
		~CallArgPack() noexcept = default;
		CallArgPack(const CallArgPack&) = default;
		CallArgPack(CallArgPack&&) noexcept = default;
		CallArgPack& operator=(const CallArgPack&) = default;
		CallArgPack& operator=(CallArgPack&&) noexcept = default;


		inline YmParams positionalArgs() const noexcept { return _positionalArgs; }
		inline YmParams namedArgs() const noexcept { return _namedArgs; }
		inline YmParams dummies() const noexcept { return _dummies; }
		inline YmParams specifiedArgs() const noexcept { return positionalArgs() + namedArgs(); }
		inline YmParams args() const noexcept { return specifiedArgs() + dummies(); }
		inline YmParams paramCount() const noexcept { return args(); }
		inline YmParams namedParamCount() const noexcept { return namedArgs() + dummies(); }
		inline bool isPositionalParam(YmParamIndex which) const noexcept { return which < positionalArgs(); }
		inline bool isNamedParam(YmParamIndex which) const noexcept { return !isPositionalParam(which) && which < paramCount(); }
		inline bool isSpecified(YmParamIndex which) const noexcept {
			// Doing it like this works even after resolving dummy mappings.
			return _argOffset(which) < specifiedArgs();
		}
		inline std::optional<YmUInt8> argOffset(YmParamIndex which, bool failIfDummy = false) const noexcept {
			if (auto result = _argOffset(which);
				result != _unbound &&
				failIfDummy ? isSpecified(which) : true) {
				return result;
			}
			return std::nullopt;
		}

		// After init, call this sequentially for each named arg, in the order they appear,
		// to specify which named param said arg is to be bound to.
		// Returns if successful, failing if which already has an arg bound to it.
		inline bool specifyNextNamedArg(YmParamIndex which) noexcept {
			ymAssert(isNamedParam(which));
			ymAssert(_dummies >= 1);
			if (isSpecified(which)) {
				return false;
			}
			_binding(which) = specifiedArgs();
			_namedArgs++;
			_dummies--;
			return true;
		}
		// Call this after specifying all named arg bindings to finish specification of
		// the arg pack to finish up, which involves resolving dummy arg bindings for
		// all unbound named parameters.
		inline void done() noexcept {
			_genDummyBinds(positionalArgs(), 0);
		}
		// Updates an otherwise finished arg pack to include additional dummies, w/
		// dummy arg bindings being resolved for them.
		// This should only be used after done has been called.
		inline void addDummies(YmParams newDummies) noexcept {
			ymAssert(YM_MAX_POSITIONAL_PARAMS + YM_MAX_NAMED_PARAMS - paramCount() >= newDummies);
			_dummies += newDummies;
			_genDummyBinds(paramCount() - newDummies, _dummies - newDummies);
		}


	private:
		static constexpr YmParamIndex _unbound = YmParamIndex(-1);


		YmParams _positionalArgs = 0;
		YmParams _namedArgs = 0;
		YmParams _dummies = 0;
		// Maps named param indices to corresponding args in arg pack + dummies.
		// _unbound is set for params w/out an associated binding.
		std::array<YmParamIndex, YM_MAX_NAMED_PARAMS> _namedParamBindsMap = {};


		inline YmParamIndex& _binding(YmParamIndex which) noexcept {
			ymAssert(isNamedParam(which));
			return _namedParamBindsMap[which - positionalArgs()];
		}
		inline const YmParamIndex& _binding(YmParamIndex which) const noexcept {
			ymAssert(isNamedParam(which));
			return _namedParamBindsMap[which - positionalArgs()];
		}
		inline YmUInt8 _argOffset(YmParamIndex which) const noexcept {
			if (isPositionalParam(which))	return which;
			else if (isNamedParam(which))	return _binding(which);
			else							return _unbound;
		}
		inline void _genDummyBinds(YmParamIndex startFrom, YmUInt8 alreadyBoundDummies) noexcept {
			ymAssert(startFrom <= paramCount());
			YmUInt8 counter = specifiedArgs() + alreadyBoundDummies;
			for (YmParamIndex param = startFrom; param < paramCount(); param++) {
				if (!isSpecified(param)) {
					_binding(param) = counter;
					counter++;
				}
			}
			_assertParamsEachBoundAtMostOnce();
		}
		inline void _assertParamsEachBoundAtMostOnce() const noexcept {
#if defined(YM_DEBUG)
			for (YmParamIndex i = positionalArgs(); i < paramCount(); i++) {
				if (_binding(i) == _unbound) continue;
				for (YmParamIndex j = i + 1; j < paramCount(); j++) {
					ymAssert(_binding(i) != _binding(j));
				}
			}
#endif
		}
	};
}

