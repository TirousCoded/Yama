

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
	// TODO: Until we add guarantee of object memory being cleaned up on dtor, we'll
	//		 track object count w/ this and assert all is cleaned up on dtor.
	size_t objects = 0;

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
};

