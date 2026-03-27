

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include <unordered_map>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "Loader.h"
#include "MAS.h"
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
	ym::Safe<YmObj> newType(ym::Safe<YmType> v);

	YmCallStackHeight callStkHeight() const noexcept;
	std::string fmtCallStk(YmCallStackHeight skip = 0) const;

	bool isUser() const noexcept; // Returns if in user pseudo-call.
	YmUInt16 args() const noexcept;
	YmLocals locals() const noexcept;
	YmObj* arg(YmUInt16 which, YmRefPolicy returnPolicy = YM_BORROW);
	YmType* ref(YmRef reference);
	YmObj* local(YmLocal where, YmRefPolicy returnPolicy = YM_BORROW);

	YmObj* pop() noexcept; // Returns taken ref.
	void popN(YmLocals n);
	bool put(YmLocal where, YmObj& what, YmRefPolicy whatPolicy = YM_TAKE);
	bool call(YmType& fn, YmUInt16 argsN, YmLocal returnTo);
	bool ret(YmObj* what, YmRefPolicy whatPolicy = YM_TAKE);


private:
	struct _CallFrame final {
		YmType* fn; // Fn being called (or nullptr for user call frame.)
		YmUInt16 args; // Number of passed args.
		YmLocal returnTo; // Where to put return value.
		YmLocal argsOffset; // Where in _globalObjStk this call frame's passed args are located.
		YmLocal localsOffset; // Where in _globalObjStk this call frame's local object stack begins.
		YmObj* returnValue = nullptr;
	};

	// TODO: This field is used to ensure all objects are released upon deinit, and
	//		 should be removed later when a more appropriate way to guarantee this.
	std::unordered_set<YmObj*> _objects;

	// The global stack of objects inside of which we alloc the data for each individual
	// call frame's local object stack, allocated in a linear fashion.
	std::vector<ym::Safe<YmObj>> _globalObjStk;

	// The call stack.
	std::vector<_CallFrame> _callStk;


	void _beginUserPseudoCall();
	bool _beginCall(YmType& fn, YmUInt16 args, YmLocal returnTo);
	bool _endCall() noexcept;
};

