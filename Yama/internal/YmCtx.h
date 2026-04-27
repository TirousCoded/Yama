

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <unordered_map>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "CallArgPack.h"
#include "Loader.h"
#include "MAS.h"
#include "PTableManager.h"
#include "RefCounter.h"
#include "YmDm.h"


struct YmCtx final {
public:
    // refs is not managed internally by this class.
    _ym::AtomicRefCounter<YmRefCount> refs;

    const ym::Safe<YmDm> domain;
    const std::shared_ptr<_ym::CtxLoader> loader;
	_ym::HeapMAS mas; // TODO: Replace later.


    YmCtx(ym::Safe<YmDm> domain);
	~YmCtx() noexcept;


    std::shared_ptr<YmParcel> import(const std::string& path);
    std::shared_ptr<YmType> load(const std::string& fullname);

	// Creates an uninitialized object of type.
	YmObj* create(YmType& type);
	YmRefCount secure(YmObj& obj);
	YmRefCount release(YmObj& obj);
	void reset();

	ym::Safe<YmObj> newNone();
	ym::Safe<YmObj> newInt(YmInt v);
	ym::Safe<YmObj> newUInt(YmUInt v);
	ym::Safe<YmObj> newFloat(YmFloat v);
	ym::Safe<YmObj> newBool(YmBool v);
	ym::Safe<YmObj> newRune(YmRune v);
	ym::Safe<YmObj> newType(YmType& v);
	YmObj* newDefault(YmType& type);

	YmCallStackHeight callStkHeight() const noexcept;
	std::string fmtCallStk(YmCallStackHeight skip = 0) const;

	bool isUser() const noexcept; // Returns if in user pseudo-call.
	YmUInt16 args() const noexcept;
	YmLocals locals() const noexcept;
	YmObj* arg(YmUInt16 which, YmRefPolicy returnPolicy = YM_BORROW);
	bool setArg(YmUInt16 which, YmObj& newArg, YmRefPolicy newArgPolicy = YM_TAKE);
	YmType* ref(YmRef reference);
	YmObj* local(YmLocal where, YmRefPolicy returnPolicy = YM_BORROW);

	YmObj* pull() noexcept; // Returns taken ref.
	void pop(YmLocals n);
	bool put(YmLocal where, YmObj& what, YmRefPolicy whatPolicy = YM_TAKE);
	bool swap(YmLocal a, YmLocal b);
	bool call(YmType& fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo);
	bool ret(YmObj* what, YmRefPolicy whatPolicy = YM_TAKE);
	bool convert(YmType& type, YmLocal returnTo);


private:
	struct _CallFrame final {
		// Fn being called (or nullptr for user call frame.)
		YmType* fn;
		_ym::CallArgPack argPack;
		// Where to put return value.
		YmLocal returnTo;
		// Where in _globalObjStk this call frame's local object stack begins (and below that are its args.)
		YmUInt32 localsOffset;
		// When protocol methods are called, they never appear on call stack, instead forwarding
		// directly to the method gotten from their ptable. This flag indicates if this call frame
		// is for one of these forwarded calls.
		bool fwdFromProto = false;
		// The bound return value object.
		YmObj* returnValue = nullptr;


		inline YmParams args() const noexcept { return argPack.args(); }
		inline YmParams positionalArgs() const noexcept { return argPack.positionalArgs(); }
		inline YmParams namedArgs() const noexcept { return argPack.namedArgs(); }
		inline YmParams dummies() const noexcept { return argPack.dummies(); }
		inline YmUInt32 localOffset(YmLocal where) const noexcept { return localsOffset + where; }
		inline std::optional<YmUInt32> argOffset(YmUInt16 which) const noexcept {
			ymAssert(which == YmUInt8(which));
			if (auto offset = argPack.argOffset(YmUInt8(which))) {
				return localsOffset - args() + *offset;
			}
			return std::nullopt;
		}
	};

	// TODO: This field is used to ensure all objects are released upon deinit, and
	//		 should be removed later when a more appropriate way to guarantee this.
	std::unordered_set<YmObj*> _objects;

	// The global stack of objects inside of which we alloc the data for each individual
	// call frame's local object stack, allocated in a linear fashion.
	std::vector<ym::Safe<YmObj>> _globalObjStk;

	// The call stack.
	std::vector<_CallFrame> _callStk;

	_ym::PTableManager _ptables;


	void _beginUserPseudoCall();
	bool _beginCall(YmType& fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo);
	bool _endCall() noexcept;
	void _dispatchCall(YmType& fn);

	bool _localInBoundsForWrite(YmLocal where) const noexcept;

	static YmRune _uint2rune(YmUInt x) noexcept;
};

