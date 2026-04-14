

#pragma once


#include <unordered_map>
#include <vector>

#include "../yama++/Safe.h"
#include "YmType.h"


namespace _ym {


	// TODO: Should this be integrated into our loading system?

	/* NOTE: Due to how our system works, it's currently impossible to 100% know prior to
	*		 loading whether a given method type is a type or object method, as that requires
	*		 knowing if it has a self-parameter.
	*
	*		 We can't know in all cases whether methods have self-parameters or not, as the
	*		 symbol for its first param's type could resolve to the same type as $Self, or
	*		 could not, w/ this variability meaning that whether such methods are type/object
	*		 methods is determined potentially by loading it.
	* 
	*		 This makes generating ptables a bit complicated, as only object methods can be
	*		 performed via protocol method dynamic dispatch (w/ type methods not being able
	*		 to since they have no runtime info as to what ptable to use) but we can't 100%
	*		 always know prior to loading what set of methods will be object methods, in turn
	*		 complicating the creation of a statically defined ptable layout.
	* 
	*		 To this end, I've opted for the easy solution of just including ALL protocol
	*		 members in the ptable, and then determining post-loading whether a given protocol
	*		 method is an object method or not. This solution lets us use the member indices
	*		 of protocol members as ptable indices.
	* 
	*		 Maybe in the future we'll revise how this works, or maybe we'll just keep it.
	*/

	class PTableManager final {
	public:
		PTableManager() = default;


		// NOTE: Top types like yama:Any will have nullptr ptable ptrs, meaning we cannot use nullptr
		//		 checks to check validity, and thus we must wrap in std::optional.

		std::optional<const ym::Safe<YmType>*> fetch(YmType& proto, YmType& boxed) const noexcept;
		std::optional<const ym::Safe<YmType>*> load(YmType& proto, YmType& boxed);


	private:
		using _Key = std::pair<ym::Safe<YmType>, ym::Safe<YmType>>;
		struct _KeyHasher final {
			using is_transparent = void;
			inline size_t operator()(const _Key& k) const noexcept {
				return ym::hash(k.first, k.second);
			}
		};


		std::unordered_map<
			_Key,
			std::vector<ym::Safe<YmType>>,
			_KeyHasher
		> _ptables;


		std::optional<const ym::Safe<YmType>*> _generate(YmType& proto, YmType& boxed);

		static _Key _mkKey(YmType& proto, YmType& boxed) noexcept;
	};
}

