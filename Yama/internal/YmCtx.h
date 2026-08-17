

#pragma once


#ifdef _YM_FORBID_INCLUDE_IN_YAMA_DOT_H
#error Not allowed to expose this header file to header file yama.h!
#endif


#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "Loader.h"
#include "ObjManager.h"
#include "PTableManager.h"
#include "RefCounter.h"
#include "StkState.h"
#include "VarStorage.h"
#include "YmDm.h"


struct YmCtx final {
public:
    // refs is not managed internally by this class.
    _ym::AtomicRefCounter refs;

    const ym::Safe<YmDm> domain;
    const std::shared_ptr<_ym::CtxLoader> loader;


    YmCtx(ym::Safe<YmDm> domain);
    ~YmCtx() noexcept;


    std::shared_ptr<YmParcel> import(const std::string& path);
    std::shared_ptr<YmType> load(const std::string& fullname);

    YmType& ldNone() const noexcept;
    YmType& ldInt() const noexcept;
    YmType& ldUInt() const noexcept;
    YmType& ldFloat() const noexcept;
    YmType& ldBool() const noexcept;
    YmType& ldRune() const noexcept;
    YmType& ldType() const noexcept;

    // A given root object may be traversed multiple times.
    inline void forEachRoot(ym::Callable<void, YmObj&> auto&& visitor) const {
        _objs.forEachRoot(visitor);
    }

    void setObjDestroyCallback(YmObjDestroyCallbackFn fn, void* user) noexcept;

    void reset();
    // Slots will be nullptr or 0.
    _ym::TempRef create(YmType& type, bool frontend);
    YmRefCount secure(YmObj& obj, bool frontend);
    YmRefCount release(YmObj& obj, bool frontend);

    _ym::TempRef newNone(bool frontendRef);
    _ym::TempRef newInt(YmInt v, bool frontendRef);
    _ym::TempRef newUInt(YmUInt v, bool frontendRef);
    _ym::TempRef newFloat(YmFloat v, bool frontendRef);
    _ym::TempRef newBool(YmBool v, bool frontendRef);
    _ym::TempRef newRune(YmRune v, bool frontendRef);
    _ym::TempRef newType(YmType& v, bool frontendRef);
    _ym::TempRef newDefault(YmType* type, bool frontendRef);

    void gcCollect();

    YmCallStackHeight callStkHeight() const noexcept;
    std::string fmtCallStk(YmCallStackHeight skip = 0) const;

    bool isUser() const noexcept; // Returns if in user pseudo-call.
    YmUInt16 args() const noexcept;
    YmLocals locals() const noexcept;

    _ym::TempRef arg(YmUInt16 which); // Returns borrowed ref.
    bool setArg(YmUInt16 which, _ym::TempRef newArg);
    YmType* ref(YmRef reference);
    _ym::TempRef local(YmLocal where); // Returns borrowed ref.
    // Returns taken ref to local at where, stealing it, and likewise leaving its
    // stack entry w/ an empty InternalRef (so be careful using this method.)
    _ym::TempRef stealLocal(YmLocal where, bool frontendRef);

    void pop(YmLocals n);
    void popUntil(YmLocals n);
    _ym::TempRef pull(bool frontendRef) noexcept; // Returns taken ref.
    bool copy(YmLocal from, YmLocal to);
    bool put(YmLocal where, _ym::TempRef what);
    bool swap(YmLocal a, YmLocal b);
    bool defaultInit(YmType* type, YmLocal where);
    bool structInit(YmType* type, std::string_view argNames, YmLocal where);
    bool call(YmType* fn, YmUInt16 argsN, std::string_view argNames, YmLocal returnTo);
    bool retObj(_ym::TempRef what);
    bool getVar(YmType* varType, YmLocal where);
    bool setVar(YmType* varType);
    bool getProperty(YmType* propertyType, YmLocal where);
    bool setProperty(YmType* propertyType);
    bool convert(YmType& type, YmLocal returnTo, bool coercion);


private:
    _ym::StkState _stk;
    _ym::ObjManager _objs;
    _ym::PTableManager _ptables;
    _ym::VarStorage _vars;


    std::optional<_ym::StructInitArgPackInfo> _resolveStructInitArgPackAndCoerceArgs(YmType& type, std::string_view argNames);
    // Fails quietly if argPack is empty.
    // Pops args from stack.
    _ym::TempRef _doStructInit(YmType& type, const std::optional<_ym::StructInitArgPackInfo>& argPack);

    bool _beginCall(YmType* fn, YmUInt16 args, std::string_view argNames, YmLocal returnTo);
    bool _endCall() noexcept;
    void _dispatchCall(YmType* fn);
    void _fwdIfProtocolMethodDispatch(YmType& fn);
    std::optional<_ym::CallArgPackInfo> _resolveArgPackCoerceArgsAndPushDummies(YmType& fn, YmUInt16 args, std::string_view argNames);
    std::optional<_ym::CallArgPackInfo> _parseArgPack(YmType& fn, std::string_view argNames) const;
    bool _checkArgPackAndCoerceArgs(YmType& fn, YmUInt16 args, const _ym::CallArgPackInfo& argPack);
    void _appendArgPackDummyObjs(const _ym::CallArgPackInfo& argPack);

    // Fails quietly.
    bool _coerce(YmLocal where, YmType& newType);
};

