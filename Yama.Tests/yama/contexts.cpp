

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"
#include "../utils/CtxState.h"


TEST(Contexts, CreateAndDestroy) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_CTX(ctx); // Macro will do the create/destroy.
}

TEST(Contexts, InitialState) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_CallStackHeight(ctx), 1); // User Pseudo-Call
    EXPECT_EQ(ymCtx_Args(ctx), 0);
    EXPECT_EQ(ymCtx_Locals(ctx), 0);
}

TEST(Contexts, RefCounting) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    auto ctx = ymCtx_Create(dm);
    ASSERT_TRUE(ctx);
    EXPECT_EQ(ymCtx_RefCount(ctx), 1); // Initial
    EXPECT_EQ(ymCtx_Secure(ctx), 1); // 1 -> 2
    EXPECT_EQ(ymCtx_RefCount(ctx), 2);
    EXPECT_EQ(ymCtx_Secure(ctx), 2); // 2 -> 3
    EXPECT_EQ(ymCtx_RefCount(ctx), 3);
    EXPECT_EQ(ymCtx_Release(ctx), 3); // 3 -> 2
    EXPECT_EQ(ymCtx_RefCount(ctx), 2);
    EXPECT_EQ(ymCtx_Release(ctx), 2); // 2 -> 1
    EXPECT_EQ(ymCtx_RefCount(ctx), 1);
    EXPECT_EQ(ymCtx_Release(ctx), 1); // Destroys
}

TEST(Contexts, Dm_Borrow) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_Dm(ctx, YM_BORROW), dm);
    EXPECT_EQ(ymDm_RefCount(dm), 1); // Unchanged
}

TEST(Contexts, Dm_Take) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_Dm(ctx, YM_TAKE), dm);
    EXPECT_EQ(ymDm_RefCount(dm), 2); // Changed
    EXPECT_EQ(ymDm_Release(dm), 2);
}

TEST(Contexts, Dm_TakeIfOk) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_Dm(ctx, YM_TAKE_IF_OK), dm);
    EXPECT_EQ(ymDm_RefCount(dm), 2); // Changed
    EXPECT_EQ(ymDm_Release(dm), 2);
}

// NOTE: See special/importing.cpp for unit tests covering importing semantics.

TEST(Contexts, Import) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p);
    std::string path = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    ymDm_BindParcelDef(dm, path.c_str(), p);
    auto a = ymCtx_Import(ctx, path.c_str());
    auto b = ymCtx_Import(ctx, path.c_str()); // Should yield same result.
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    ASSERT_EQ(a, b);
}

TEST(Contexts, Import_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymDm_BindParcelDef(dm, "p/q/r", p_def);
    auto a = ymCtx_Import(ctx, "p/q/r");
    auto b = ymCtx_Import(ctx, "p  /  q /      \r r");
    auto c = ymCtx_Import(ctx, "    p/  q  \n\n\n /r   ");
    ASSERT_TRUE(a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, c);
}

TEST(Contexts, Import_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    ymDm_BindParcelDef(dm, "p", p_def);
    SETUP_CTX(ctx1);
    SETUP_CTX(ctx2);
    EXPECT_EQ(ymCtx_Import(ctx1, "p"), ymCtx_Import(ctx2, "p"));
}

// NOTE: See special/loading.cpp for unit tests covering loading semantics.

TEST(Contexts, Load) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    std::string name = taul::utf8_s(u8"ab魂💩cd"); // Ensure can handle UTF-8.
    std::string fullname = taul::utf8_s(u8"p:ab魂💩cd"); // Ensure can handle UTF-8.
    ymParcelDef_AddStruct(p_def, name.c_str());
    BIND_AND_IMPORT(ctx, p, p_def, "p");
    YmType* type = ymCtx_Load(ctx, fullname.c_str());
    ASSERT_NE(type, nullptr);
    EXPECT_EQ(ymType_Parcel(type), ymCtx_Import(ctx, "p"));
    EXPECT_STREQ(ymType_Fullname(type), fullname.c_str());
    EXPECT_EQ(ymType_Kind(type), YmKind_Struct);
}

TEST(Contexts, Load_Normalizes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p/q/r", p_def);
    auto a = ymCtx_Load(ctx, "p/q/r:A");
    auto b = ymCtx_Load(ctx, "p  /  q /      \r r  :A");
    auto c = ymCtx_Load(ctx, "    p/  q  \n\n\n /r  :   A ");
    ASSERT_TRUE(a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(a, c);
}

TEST(Contexts, Load_AcrossCtxBoundaries) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    ymParcelDef_AddStruct(p_def, "A");
    ymDm_BindParcelDef(dm, "p", p_def);
    SETUP_CTX(ctx1);
    SETUP_CTX(ctx2);
    EXPECT_EQ(ymCtx_Load(ctx1, "p:A"), ymCtx_Load(ctx2, "p:A"));
}

TEST(Contexts, FastPathLoadFns) {
    SETUP_ALL(ctx);
    EXPECT_EQ(ymCtx_LdNone(ctx), ymCtx_Load(ctx, "yama:None"));
    EXPECT_EQ(ymCtx_LdInt(ctx), ymCtx_Load(ctx, "yama:Int"));
    EXPECT_EQ(ymCtx_LdUInt(ctx), ymCtx_Load(ctx, "yama:UInt"));
    EXPECT_EQ(ymCtx_LdFloat(ctx), ymCtx_Load(ctx, "yama:Float"));
    EXPECT_EQ(ymCtx_LdBool(ctx), ymCtx_Load(ctx, "yama:Bool"));
    EXPECT_EQ(ymCtx_LdRune(ctx), ymCtx_Load(ctx, "yama:Rune"));
    EXPECT_EQ(ymCtx_LdType(ctx), ymCtx_Load(ctx, "yama:Type"));
}

namespace {
    inline ErrCounter* _err = nullptr;
    inline size_t observedCalls = 0;
}

static ErrCounter& getErr() noexcept {
    return *_err;
}

static void objsys_test(
    std::function<void(YmParcelDef* parceldef)> setup,
    std::function<void(YmCtx* ctx, bool called_in_fn_body)> body) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    _err = &err;
    ymParcelDef_AddFn(
        p_def, "f", "yama:None",
        [](YmCtx* ctx, YmType* type, void* body_ptr) {
            auto& _body = *(decltype(body)*)body_ptr;

            // NOTE: Test usage within fn bodies.

            ym::println("-- in fn test");
            observedCalls = 0; // Reset
            _body(ctx, true);

            ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
        },
        (void*)&body);
    ymParcelDef_AddParam(p_def, "f", "x", "yama:Int");
    ymParcelDef_AddParam(p_def, "f", "y", "yama:Float");

    ym::println("-- setup");
    setup(p_def);

    ymDm_BindParcelDef(dm, "p", p_def);
    {
        err.reset();
        SETUP_CTX(ctx);

        ym::println("-- out-of fn test");
        observedCalls = 0; // Reset
        body(ctx, false);
    }
    {
        err.reset();
        SETUP_CTX(ctx);

        auto f = ymCtx_Load(ctx, "p:f");
        ASSERT_TRUE(f);

        ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 10), YM_TRUE);
        ASSERT_EQ(ymCtx_PutFloat(ctx, YM_PUSH, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_Call(ctx, f, 2, "", YM_DISCARD), YM_TRUE);
    }
}

static void objsys_test(
    std::function<void(YmCtx* ctx, bool called_in_fn_body)> body) {
    objsys_test(
        [](YmParcelDef* parceldef) {},
        body);
}

TEST(Contexts, NewNone) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewNone(ctx));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdNone(ctx));
        });
}

TEST(Contexts, NewInt) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewInt(ctx, -50));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdInt(ctx));
        EXPECT_EQ(ymObj_ToInt(obj, nullptr), -50);
        });
}

TEST(Contexts, NewUInt) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewUInt(ctx, 50));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdUInt(ctx));
        EXPECT_EQ(ymObj_ToUInt(obj, nullptr), 50);
        });
}

TEST(Contexts, NewFloat) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewFloat(ctx, 3.14159));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdFloat(ctx));
        EXPECT_DOUBLE_EQ(ymObj_ToFloat(obj, nullptr), 3.14159);
        });
}

TEST(Contexts, NewBool) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewBool(ctx, YM_TRUE));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdBool(ctx));
        EXPECT_EQ(ymObj_ToBool(obj, nullptr), YM_TRUE);
        });
}

TEST(Contexts, NewRune) {
    objsys_test([](YmCtx* ctx, bool) {
        SETUP_OBJ(obj, ymCtx_NewRune(ctx, U'y'));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdRune(ctx));
        EXPECT_EQ(ymObj_ToRune(obj, nullptr), U'y');
        });
}

TEST(Contexts, NewType) {
    objsys_test([](YmCtx* ctx, bool) {
        auto t = ymCtx_LdInt(ctx);
        ASSERT_TRUE(t);
        SETUP_OBJ(obj, ymCtx_NewType(ctx, t));
        ASSERT_EQ(ymObj_Type(obj), ymCtx_LdType(ctx));
        EXPECT_EQ(ymObj_ToType(obj, nullptr), t);
        });
}

TEST(Contexts, CallStackHeight) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_CallStackHeight(ctx), called_in_fn_body ? 2 : 1);
        });
}

TEST(Contexts, FmtCallStack) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        // We don't assert what the contents of the call stack trace is.
        auto r = ymCtx_FmtCallStack(ctx, 0);
        ASSERT_TRUE(r);
        std::free((void*)r);
        });
}

TEST(Contexts, Args) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_Args(ctx), called_in_fn_body ? 2 : 0);
        });
}

TEST(Contexts, Arg_Borrow) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            if (auto x = ymCtx_Arg(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                EXPECT_EQ(ymObj_ToInt(x, nullptr), 10);
            }
            else ADD_FAILURE();
            if (auto y = ymCtx_Arg(ctx, 1, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(y), 1);
                EXPECT_EQ(ymObj_Type(y), ymCtx_LdFloat(ctx));
                EXPECT_DOUBLE_EQ(ymObj_ToFloat(y, nullptr), 3.14159);
            }
            else ADD_FAILURE();

            EXPECT_FALSE(ymCtx_Arg(ctx, 2, YM_BORROW));
        }
        else {
            EXPECT_FALSE(ymCtx_Arg(ctx, 0, YM_BORROW));
        }
        });
}

TEST(Contexts, Arg_Take) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            if (auto x = ymCtx_Arg(ctx, 0, YM_TAKE)) {
                EXPECT_EQ(ymObj_RefCount(x), 2);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                EXPECT_EQ(ymObj_ToInt(x, nullptr), 10);
                ymObj_Release(x);
            }
            else ADD_FAILURE();
            if (auto y = ymCtx_Arg(ctx, 1, YM_TAKE)) {
                EXPECT_EQ(ymObj_RefCount(y), 2);
                EXPECT_EQ(ymObj_Type(y), ymCtx_LdFloat(ctx));
                EXPECT_DOUBLE_EQ(ymObj_ToFloat(y, nullptr), 3.14159);
                ymObj_Release(y);
            }
            else ADD_FAILURE();

            EXPECT_FALSE(ymCtx_Arg(ctx, 2, YM_TAKE));
        }
        else {
            EXPECT_FALSE(ymCtx_Arg(ctx, 0, YM_TAKE));
        }
        });
}

TEST(Contexts, Arg_TakeIfOk) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            if (auto x = ymCtx_Arg(ctx, 0, YM_TAKE_IF_OK)) {
                EXPECT_EQ(ymObj_RefCount(x), 2);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                EXPECT_EQ(ymObj_ToInt(x, nullptr), 10);
                ymObj_Release(x);
            }
            else ADD_FAILURE();
            if (auto y = ymCtx_Arg(ctx, 1, YM_TAKE_IF_OK)) {
                EXPECT_EQ(ymObj_RefCount(y), 2);
                EXPECT_EQ(ymObj_Type(y), ymCtx_LdFloat(ctx));
                EXPECT_DOUBLE_EQ(ymObj_ToFloat(y, nullptr), 3.14159);
                ymObj_Release(y);
            }
            else ADD_FAILURE();

            EXPECT_FALSE(ymCtx_Arg(ctx, 2, YM_TAKE_IF_OK));
        }
        else {
            EXPECT_FALSE(ymCtx_Arg(ctx, 0, YM_TAKE_IF_OK));
        }
        });
}

TEST(Contexts, SetArg_Borrow) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 1, newArg, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 1);
        EXPECT_EQ(ymObj_RefCount(newArg), 2);
        });
}

TEST(Contexts, SetArg_Take) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        auto newArg = ymCtx_NewRune(ctx, U'y');
        ASSERT_TRUE(newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 1, newArg, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 1);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
}

TEST(Contexts, SetArg_TakeIfOk) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        auto newArg = ymCtx_NewRune(ctx, U'y');
        ASSERT_TRUE(newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 1, newArg, YM_TAKE_IF_OK), YM_TRUE);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 1);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
}

TEST(Contexts, SetArg_ArgNotFound) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 10'000, newArg, YM_BORROW), YM_FALSE);
        EXPECT_GE(getErr()[YmErrCode_ArgNotFound], 1);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), y);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));
        SETUP_OBJ_REF_COPY(nameArg2, newArg);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 2);

        EXPECT_EQ(ymCtx_SetArg(ctx, 10'000, newArg, YM_TAKE), YM_FALSE);
        EXPECT_GE(getErr()[YmErrCode_ArgNotFound], 1);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), y);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1); // API took ref anyway.
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (!called_in_fn_body) {
            return;
        }
        SETUP_OBJ(x, ymCtx_Arg(ctx, 0, YM_TAKE));
        SETUP_OBJ(y, ymCtx_Arg(ctx, 1, YM_TAKE));
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 10'000, newArg, YM_TAKE_IF_OK), YM_FALSE);
        EXPECT_GE(getErr()[YmErrCode_ArgNotFound], 1);

        EXPECT_EQ(ymCtx_Args(ctx), 2);
        EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), x);
        EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), y);

        EXPECT_EQ(ymObj_RefCount(x), 2);
        EXPECT_EQ(ymObj_RefCount(y), 2);
        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
}

TEST(Contexts, SetArg_FailsQuietly_InUserCallFrame) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            return;
        }
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));

        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 0, newArg, YM_BORROW), YM_FALSE); // Quiet

        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            return;
        }
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));
        SETUP_OBJ_REF_COPY(nameArg2, newArg);

        EXPECT_EQ(ymObj_RefCount(newArg), 2);

        EXPECT_EQ(ymCtx_SetArg(ctx, 0, newArg, YM_TAKE), YM_FALSE); // Quiet

        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        if (called_in_fn_body) {
            return;
        }
        SETUP_OBJ(newArg, ymCtx_NewRune(ctx, U'y'));

        EXPECT_EQ(ymObj_RefCount(newArg), 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 0, newArg, YM_TAKE_IF_OK), YM_FALSE); // Quiet

        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
}

TEST(Contexts, Ref) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "p:A"), 0);
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "yama:Float"), 1);
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "p:A"), 2);
            EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"), YM_TRUE);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (called_in_fn_body) {
                EXPECT_EQ(ymCtx_Ref(ctx, 0), ymCtx_Load(ctx, "p:A"));
                EXPECT_EQ(ymCtx_Ref(ctx, 1), ymCtx_LdFloat(ctx));
                EXPECT_EQ(ymCtx_Ref(ctx, 2), ymCtx_Load(ctx, "p:A"));
                EXPECT_EQ(ymCtx_Ref(ctx, 3), nullptr);
            }
            else {
                EXPECT_EQ(ymCtx_Ref(ctx, 0), nullptr);
                EXPECT_EQ(ymCtx_Ref(ctx, 1), nullptr);
                EXPECT_EQ(ymCtx_Ref(ctx, 2), nullptr);
                EXPECT_EQ(ymCtx_Ref(ctx, 3), nullptr);
            }
        });
}

TEST(Contexts, Locals) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_Locals(ctx), 1);
        });
}

TEST(Contexts, Local_Borrow) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_PUSH, 3.14159), YM_TRUE);

        if (auto a = ymCtx_Local(ctx, 0, YM_BORROW)) {
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
        }
        if (auto a = ymCtx_Local(ctx, 1, YM_BORROW)) {
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
        }

        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), nullptr);

        // Negative Indices

        if (auto a = ymCtx_Local(ctx, -2, YM_BORROW)) {
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
        }
        if (auto a = ymCtx_Local(ctx, -1, YM_BORROW)) {
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
        }

        EXPECT_EQ(ymCtx_Local(ctx, -3, YM_BORROW), nullptr);
        });
}

TEST(Contexts, Local_Take) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_PUSH, 3.14159), YM_TRUE);

        if (auto a = ymCtx_Local(ctx, 0, YM_TAKE)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
            ymObj_Release(a);
        }
        if (auto a = ymCtx_Local(ctx, 1, YM_TAKE)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
            ymObj_Release(a);
        }

        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_TAKE), nullptr);

        // Negative Indices

        if (auto a = ymCtx_Local(ctx, -2, YM_TAKE)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
            ymObj_Release(a);
        }
        if (auto a = ymCtx_Local(ctx, -1, YM_TAKE)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
            ymObj_Release(a);
        }

        EXPECT_EQ(ymCtx_Local(ctx, -3, YM_TAKE), nullptr);
        });
}

TEST(Contexts, Local_TakeIfOk) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_PUSH, 3.14159), YM_TRUE);

        if (auto a = ymCtx_Local(ctx, 0, YM_TAKE_IF_OK)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
            ymObj_Release(a);
        }
        if (auto a = ymCtx_Local(ctx, 1, YM_TAKE_IF_OK)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
            ymObj_Release(a);
        }

        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_TAKE_IF_OK), nullptr);

        // Negative Indices

        if (auto a = ymCtx_Local(ctx, -2, YM_TAKE_IF_OK)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdInt(ctx));
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 10);
            ymObj_Release(a);
        }
        if (auto a = ymCtx_Local(ctx, -1, YM_TAKE_IF_OK)) {
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_Type(a), ymCtx_LdFloat(ctx));
            EXPECT_DOUBLE_EQ(ymObj_ToFloat(a, nullptr), 3.14159);
            ymObj_Release(a);
        }

        EXPECT_EQ(ymCtx_Local(ctx, -3, YM_TAKE_IF_OK), nullptr);
        });
}

TEST(Contexts, Pop) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);

        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);

        EXPECT_EQ(ymCtx_Locals(ctx), 5);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
        EXPECT_EQ(ymObj_RefCount(aa), 2); // Before pop.

        ymCtx_Pop(ctx, 3);

        EXPECT_EQ(ymCtx_Locals(ctx), 2);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
        EXPECT_EQ(ymObj_RefCount(aa), 2); // Before pop.

        ymCtx_Pop(ctx, 4); // Should stop prematurely.

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), nullptr);
        EXPECT_EQ(ymObj_RefCount(aa), 1); // After pop.

        EXPECT_EQ(ymObj_Release(aa), 1);
        });
}

TEST(Contexts, Pop_FailQuietly_NIsNegative) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);
        auto bb = ymCtx_NewInt(ctx, 150);
        auto cc = ymCtx_NewInt(ctx, 200);

        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymCtx_Locals(ctx), 3);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, -1); // Fails Quietly

        EXPECT_EQ(ymCtx_Locals(ctx), 3);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        EXPECT_EQ(ymObj_Release(aa), 2);
        EXPECT_EQ(ymObj_Release(bb), 2);
        EXPECT_EQ(ymObj_Release(cc), 2);
        });
}

TEST(Contexts, PopAll) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);

        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);

        EXPECT_EQ(ymCtx_Locals(ctx), 5);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
        EXPECT_EQ(ymObj_RefCount(aa), 3); // Before pop.

        ymCtx_PopAll(ctx);

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), nullptr);
        EXPECT_EQ(ymObj_RefCount(aa), 1); // After pop.

        EXPECT_EQ(ymObj_Release(aa), 1);
        });
}

TEST(Contexts, Pull) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);
        auto bb = ymCtx_NewInt(ctx, 100);
        auto cc = ymCtx_NewInt(ctx, 150);

        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymCtx_Locals(ctx), 3);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        if (auto x = ymCtx_Pull(ctx)) {
            EXPECT_EQ(x, cc);
            ymObj_Release(x);
        }
        else ADD_FAILURE();

        EXPECT_EQ(ymCtx_Locals(ctx), 2);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), nullptr);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 1);

        if (auto x = ymCtx_Pull(ctx)) {
            EXPECT_EQ(x, bb);
            ymObj_Release(x);
        }
        else ADD_FAILURE();

        EXPECT_EQ(ymCtx_Locals(ctx), 1);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), nullptr);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), nullptr);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 1);
        EXPECT_EQ(ymObj_RefCount(cc), 1);

        if (auto x = ymCtx_Pull(ctx)) {
            EXPECT_EQ(x, aa);
            ymObj_Release(x);
        }
        else ADD_FAILURE();

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), nullptr);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), nullptr);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), nullptr);
        EXPECT_EQ(ymObj_RefCount(aa), 1);
        EXPECT_EQ(ymObj_RefCount(bb), 1);
        EXPECT_EQ(ymObj_RefCount(cc), 1);

        EXPECT_EQ(ymCtx_Pull(ctx), nullptr);

        EXPECT_EQ(ymObj_Release(aa), 1);
        EXPECT_EQ(ymObj_Release(bb), 1);
        EXPECT_EQ(ymObj_Release(cc), 1);
        });
}

TEST(Contexts, Put_Borrow) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);
        auto bb = ymCtx_NewInt(ctx, 100);
        auto cc = ymCtx_NewInt(ctx, 150);
        auto dd = ymCtx_NewInt(ctx, 200);

        // Pushing

        EXPECT_EQ(ymObj_RefCount(aa), 1);
        EXPECT_EQ(ymObj_RefCount(bb), 1);
        EXPECT_EQ(ymObj_RefCount(cc), 1);

        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, 3);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Putting

        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 1);
        EXPECT_EQ(ymObj_RefCount(bb), 1);
        EXPECT_EQ(ymObj_RefCount(cc), 1);
        EXPECT_EQ(ymObj_RefCount(dd), 7);

        EXPECT_EQ(ymCtx_Put(ctx, 0, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 1, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 2, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);
        EXPECT_EQ(ymObj_RefCount(dd), 4);

        EXPECT_EQ(ymCtx_Put(ctx, -3, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -2, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -1, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 1);

        ymCtx_Pop(ctx, 6);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        EXPECT_EQ(ymObj_RefCount(aa), 1);
        EXPECT_EQ(ymCtx_Put(ctx, YM_DISCARD, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymObj_RefCount(aa), 1);

        EXPECT_EQ(ymObj_Release(aa), 1);
        EXPECT_EQ(ymObj_Release(bb), 1);
        EXPECT_EQ(ymObj_Release(cc), 1);
        EXPECT_EQ(ymObj_Release(dd), 1);
        });
}

TEST(Contexts, Put_Take) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);
        auto bb = ymCtx_NewInt(ctx, 100);
        auto cc = ymCtx_NewInt(ctx, 150);
        auto dd = ymCtx_NewInt(ctx, 200);

        // Pushing

        ymObj_Secure(aa);
        ymObj_Secure(bb);
        ymObj_Secure(cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, 3);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Putting

        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);

        ymObj_Secure(aa);
        ymObj_Secure(aa);
        ymObj_Secure(bb);
        ymObj_Secure(bb);
        ymObj_Secure(cc);
        ymObj_Secure(cc);
        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 7);

        EXPECT_EQ(ymCtx_Put(ctx, 0, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 1, bb, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 2, cc, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 4);

        EXPECT_EQ(ymCtx_Put(ctx, -3, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -2, bb, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -1, cc, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 1);

        ymCtx_Pop(ctx, 6);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Add ref count incr that the YM_DISCARD put is to consume.
        EXPECT_EQ(ymObj_Secure(aa), 1);
        EXPECT_EQ(ymCtx_Put(ctx, YM_DISCARD, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymObj_RefCount(aa), 1);

        EXPECT_EQ(ymObj_Release(aa), 1);
        EXPECT_EQ(ymObj_Release(bb), 1);
        EXPECT_EQ(ymObj_Release(cc), 1);
        EXPECT_EQ(ymObj_Release(dd), 1);
        });
}

TEST(Contexts, Put_TakeIfOk) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);
        auto bb = ymCtx_NewInt(ctx, 100);
        auto cc = ymCtx_NewInt(ctx, 150);
        auto dd = ymCtx_NewInt(ctx, 200);

        // Pushing

        ymObj_Secure(aa);
        ymObj_Secure(bb);
        ymObj_Secure(cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_TAKE_IF_OK), YM_TRUE);

        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, 3);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Putting

        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, dd, YM_BORROW), YM_TRUE);

        ymObj_Secure(aa);
        ymObj_Secure(aa);
        ymObj_Secure(bb);
        ymObj_Secure(bb);
        ymObj_Secure(cc);
        ymObj_Secure(cc);
        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 7);

        EXPECT_EQ(ymCtx_Put(ctx, 0, aa, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 1, bb, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 2, cc, YM_TAKE_IF_OK), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 4);

        EXPECT_EQ(ymCtx_Put(ctx, -3, aa, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -2, bb, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, -1, cc, YM_TAKE_IF_OK), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 3);
        EXPECT_EQ(ymObj_RefCount(bb), 3);
        EXPECT_EQ(ymObj_RefCount(cc), 3);
        EXPECT_EQ(ymObj_RefCount(dd), 1);

        ymCtx_Pop(ctx, 6);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Add ref count incr that the YM_DISCARD put is to consume.
        EXPECT_EQ(ymObj_Secure(aa), 1);
        EXPECT_EQ(ymCtx_Put(ctx, YM_DISCARD, aa, YM_TAKE_IF_OK), YM_TRUE);
        EXPECT_EQ(ymObj_RefCount(aa), 1);

        EXPECT_EQ(ymObj_Release(aa), 1);
        EXPECT_EQ(ymObj_Release(bb), 1);
        EXPECT_EQ(ymObj_Release(cc), 1);
        EXPECT_EQ(ymObj_Release(dd), 1);
        });
}

TEST(Contexts, Put_Fail_LocalNotFound) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));

        EXPECT_EQ(ymCtx_Put(ctx, 0, a, YM_BORROW), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));

        EXPECT_EQ(ymCtx_Put(ctx, -1, a, YM_BORROW), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));
        SETUP_OBJ_REF_COPY(a2, a);

        EXPECT_EQ(ymCtx_Put(ctx, 0, a, YM_TAKE), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1); // API took ref anyway.
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));
        SETUP_OBJ_REF_COPY(a2, a);

        EXPECT_EQ(ymCtx_Put(ctx, -1, a, YM_TAKE), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1); // API took ref anyway.
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));

        EXPECT_EQ(ymCtx_Put(ctx, 0, a, YM_TAKE_IF_OK), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));

        EXPECT_EQ(ymCtx_Put(ctx, -1, a, YM_TAKE_IF_OK), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymObj_RefCount(a), 1);
        });
}

TEST(Contexts, PutXXXX) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        // Test w/ pushing.
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_PUSH, -10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, YM_PUSH, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_PUSH, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, YM_PUSH, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, YM_PUSH, ymCtx_LdInt(ctx)), YM_TRUE);

        auto num = (YmLocal)ymCtx_Locals(ctx);

        // Setup for test w/ putting.
        // Need num * 2 objects as ladder is needed to test negative indices.
        SETUP_OBJ(temp, ymCtx_NewInt(ctx, 100));
        for (YmLocal i = 0; i < num * 2; i++) {
            EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, temp, YM_BORROW), YM_TRUE) << "i==" << i;
        }
        ASSERT_EQ(ymCtx_Locals(ctx), num * 3);

        // Before puts overwrite temp locals.
        EXPECT_EQ(ymObj_RefCount(temp), num * 2 + 1);

        // Perform putting.
        EXPECT_EQ(ymCtx_PutNone(ctx, num + 0), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, num + 1, -10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, num + 2, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, num + 3, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, num + 4, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, num + 5, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, num + 6, ymCtx_LdInt(ctx)), YM_TRUE);

        // Perform putting w/ negative indices.
        EXPECT_EQ(ymCtx_PutNone(ctx, -num + 0), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, -num + 1, -10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, -num + 2, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, -num + 3, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, -num + 4, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, -num + 5, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, -num + 6, ymCtx_LdInt(ctx)), YM_TRUE);

        // After puts overwrite temp locals.
        EXPECT_EQ(ymObj_RefCount(temp), 1);

        // Now check everything (we loop thrice for each as the sequence should be the
        // same for both pushing, putting, and putting w/ negative indices.)
        for (YmLocal i = 0; i < 3; i++) {
            if (auto x = ymCtx_Local(ctx, num * i + 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdNone(ctx));
            }
            if (auto x = ymCtx_Local(ctx, num * i + 1, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                EXPECT_EQ(ymObj_ToInt(x, nullptr), -10);
            }
            if (auto x = ymCtx_Local(ctx, num * i + 2, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdUInt(ctx));
                EXPECT_EQ(ymObj_ToUInt(x, nullptr), 10);
            }
            if (auto x = ymCtx_Local(ctx, num * i + 3, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdFloat(ctx));
                EXPECT_DOUBLE_EQ(ymObj_ToFloat(x, nullptr), 3.14159);
            }
            if (auto x = ymCtx_Local(ctx, num * i + 4, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdBool(ctx));
                EXPECT_EQ(ymObj_ToBool(x, nullptr), YM_TRUE);
            }
            if (auto x = ymCtx_Local(ctx, num * i + 5, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdRune(ctx));
                EXPECT_EQ(ymObj_ToRune(x, nullptr), U'y');
            }
            if (auto x = ymCtx_Local(ctx, num * i + 6, YM_BORROW)) {
                EXPECT_EQ(ymObj_RefCount(x), 1);
                EXPECT_EQ(ymObj_Type(x), ymCtx_LdType(ctx));
                EXPECT_EQ(ymObj_ToType(x, nullptr), ymCtx_LdInt(ctx));
            }
        }
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_DISCARD), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_DISCARD, -50), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, YM_DISCARD, 50), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_DISCARD, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, YM_DISCARD, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, YM_DISCARD, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, YM_DISCARD, ymCtx_LdInt(ctx)), YM_TRUE);

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, PutXXXX_Fail_LocalNotFound) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutNone(ctx, 0), YM_FALSE);
        EXPECT_EQ(ymCtx_PutInt(ctx, 0, -50), YM_FALSE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, 0, 50), YM_FALSE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, 0, 3.14159), YM_FALSE);
        EXPECT_EQ(ymCtx_PutBool(ctx, 0, YM_TRUE), YM_FALSE);
        EXPECT_EQ(ymCtx_PutRune(ctx, 0, U'y'), YM_FALSE);
        EXPECT_EQ(ymCtx_PutType(ctx, 0, ymCtx_LdInt(ctx)), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 7);

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutNone(ctx, -1), YM_FALSE);
        EXPECT_EQ(ymCtx_PutInt(ctx, -1, -50), YM_FALSE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, -1, 50), YM_FALSE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, -1, 3.14159), YM_FALSE);
        EXPECT_EQ(ymCtx_PutBool(ctx, -1, YM_TRUE), YM_FALSE);
        EXPECT_EQ(ymCtx_PutRune(ctx, -1, U'y'), YM_FALSE);
        EXPECT_EQ(ymCtx_PutType(ctx, -1, ymCtx_LdInt(ctx)), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 7);

        EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, DefaultInit) {
    // NOTE: Don't forget to update DefaultInit_Fail_NoDefaultValue too.
    // NOTE: Also, update the YM_DISCARD tests too.
    auto setupfn =
        [](YmParcelDef* parceldef) {
        // Empty Struct
        EXPECT_EQ(ymParcelDef_AddStruct(parceldef,
            "Struct0"
        ), YM_TRUE);

        // Method (of Struct0)
        EXPECT_EQ(ymParcelDef_AddMethod(parceldef,
            "Struct0",
            "m",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_TRUE);

        // Fn
        EXPECT_EQ(ymParcelDef_AddFn(parceldef,
            "Fn0",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_TRUE);
        };
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Struct0 = load(ctx, "p:Struct0");

            // Test w/ pushing.
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdNone(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdUInt(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdFloat(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdBool(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdRune(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdType(ctx), YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, Struct0, YM_PUSH), YM_TRUE);

            auto num = (YmLocal)ymCtx_Locals(ctx);

            // Setup for test w/ putting.
            // Need num * 2 objects as ladder is needed to test negative indices.
            SETUP_OBJ(temp, ymCtx_NewInt(ctx, 100));
            for (YmLocal i = 0; i < num * 2; i++) {
                EXPECT_EQ(ymCtx_Put(ctx, YM_PUSH, temp, YM_BORROW), YM_TRUE) << "i==" << i;
            }
            ASSERT_EQ(ymCtx_Locals(ctx), num * 3);

            // Before puts overwrite temp locals.
            EXPECT_EQ(ymObj_RefCount(temp), num * 2 + 1);

            // Perform putting.
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdNone(ctx), num + 0), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), num + 1), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdUInt(ctx), num + 2), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdFloat(ctx), num + 3), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdBool(ctx), num + 4), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdRune(ctx), num + 5), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdType(ctx), num + 6), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, Struct0, num + 7), YM_TRUE);

            // Perform putting.
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdNone(ctx), -num + 0), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), -num + 1), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdUInt(ctx), -num + 2), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdFloat(ctx), -num + 3), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdBool(ctx), -num + 4), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdRune(ctx), -num + 5), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdType(ctx), -num + 6), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, Struct0, -num + 7), YM_TRUE);

            // After puts overwrite temp locals.
            EXPECT_EQ(ymObj_RefCount(temp), 1);

            // Now check everything (we loop thrice for each as the sequence should be the
            // same for both pushing, putting, and putting w/ negative indices.)
            for (YmLocal i = 0; i < 3; i++) {
                if (auto x = ymCtx_Local(ctx, num * i + 0, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdNone(ctx));
                }
                if (auto x = ymCtx_Local(ctx, num * i + 1, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                    EXPECT_EQ(ymObj_ToInt(x, nullptr), 0);
                }
                if (auto x = ymCtx_Local(ctx, num * i + 2, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdUInt(ctx));
                    EXPECT_EQ(ymObj_ToUInt(x, nullptr), 0);
                }
                if (auto x = ymCtx_Local(ctx, num * i + 3, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdFloat(ctx));
                    EXPECT_DOUBLE_EQ(ymObj_ToFloat(x, nullptr), 0.0);
                }
                if (auto x = ymCtx_Local(ctx, num * i + 4, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdBool(ctx));
                    EXPECT_EQ(ymObj_ToBool(x, nullptr), YM_FALSE);
                }
                if (auto x = ymCtx_Local(ctx, num * i + 5, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdRune(ctx));
                    EXPECT_EQ(ymObj_ToRune(x, nullptr), U'\0');
                }
                if (auto x = ymCtx_Local(ctx, num * i + 6, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), ymCtx_LdType(ctx));
                    EXPECT_EQ(ymObj_ToType(x, nullptr), ymCtx_LdNone(ctx));
                }
                if (auto x = ymCtx_Local(ctx, num * i + 7, YM_BORROW)) {
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_Type(x), Struct0);
                }
            }
        });
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Struct0 = load(ctx, "p:Struct0");

            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdNone(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdUInt(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdFloat(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdBool(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdRune(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdType(ctx), YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_DefaultInit(ctx, Struct0, YM_DISCARD), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, DefaultInit_Fail_LocalNotFound) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), 0), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            EXPECT_EQ(ymCtx_DefaultInit(ctx, ymCtx_LdInt(ctx), -1), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, DefaultInit_Fail_NoDefaultValue) {
    auto setupfn =
        [](YmParcelDef* parceldef) {
        EXPECT_EQ(ymParcelDef_AddProtocol(parceldef, "P"), YM_TRUE);
        };
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto P = load(ctx, "p:P");

            EXPECT_EQ(ymCtx_DefaultInit(ctx, P, YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_NoDefaultValue], 1);
        });
}

TEST(Contexts, StructInit_EmptyStruct) {
    auto setup =
        [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
            ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            // test w/ regular index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", 0), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);

            EXPECT_EQ(ymObj_RefCount(x), 1);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            // test w/ negative index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", -1), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);

            EXPECT_EQ(ymObj_RefCount(x), 1);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            // test w/ push

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            // test w/ discard

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_DISCARD), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, StructInit_NonEmptyStruct) {
    auto setup =
        [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
        ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
            ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ regular index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", 0), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), cc);

            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(aa), 3);
            EXPECT_EQ(ymObj_RefCount(bb), 3);
            EXPECT_EQ(ymObj_RefCount(cc), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ negative index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", -1), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), cc);

            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(aa), 3);
            EXPECT_EQ(ymObj_RefCount(bb), 3);
            EXPECT_EQ(ymObj_RefCount(cc), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ push

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", YM_PUSH), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            auto result = ymCtx_Local(ctx, 0, YM_BORROW);
            EXPECT_EQ(ymObj_Type(result), A);
            EXPECT_EQ(ymObj_RefCount(result), 1);

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, result, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), cc);

            EXPECT_EQ(ymObj_RefCount(aa), 3);
            EXPECT_EQ(ymObj_RefCount(bb), 3);
            EXPECT_EQ(ymObj_RefCount(cc), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ discard

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", YM_DISCARD), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);

            EXPECT_EQ(ymObj_RefCount(aa), 1);
            EXPECT_EQ(ymObj_RefCount(bb), 1);
            EXPECT_EQ(ymObj_RefCount(cc), 1);
        });
}

TEST(Contexts, StructInit_Fail_LocalNotFound_WhereIsOutOfBounds) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
        ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
            ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", 10'000), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);

            EXPECT_EQ(ymObj_RefCount(aa), 2);
            EXPECT_EQ(ymObj_RefCount(bb), 2);
            EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", -10'000), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);

            EXPECT_EQ(ymObj_RefCount(aa), 2);
            EXPECT_EQ(ymObj_RefCount(bb), 2);
            EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_NonStructType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(parceldef, "g", "yama:None", ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            ASSERT_EQ(ymCtx_StructInit(ctx, g, "", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_NonStructType], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, StructInit_Fail_IllegalNameList_DoesntSpecifyEveryStoredPropertyOfType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            //SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            //ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_IllegalNameList], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 2);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
            //
            ////EXPECT_EQ(ymObj_RefCount(aa), 1);
            //EXPECT_EQ(ymObj_RefCount(bb), 2);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_IllegalNameList_SpecifiesAPropertyMoreThanOnce) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_IllegalNameList], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 4);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            //EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), bb);
            //
            //EXPECT_EQ(ymObj_RefCount(aa), 2);
            //EXPECT_EQ(ymObj_RefCount(bb), 3);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_IllegalNameList_SpecifiesANameWhichDoesntNameAStoredProperty) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,missing,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_IllegalNameList], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 3);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            //
            //EXPECT_EQ(ymObj_RefCount(aa), 2);
            //EXPECT_EQ(ymObj_RefCount(bb), 2);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_IllegalNameList_SpecifiesANameWhichDoesntNameAStoredProperty_NamesComputedProperty) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,abc,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_IllegalNameList], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 3);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            //
            //EXPECT_EQ(ymObj_RefCount(aa), 2);
            //EXPECT_EQ(ymObj_RefCount(bb), 2);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_LocalNotFound_ArgObjsNeededExceedsObjStkHeight) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.414));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            //ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 2);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            //
            //EXPECT_EQ(ymObj_RefCount(aa), 2);
            //EXPECT_EQ(ymObj_RefCount(bb), 1);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, StructInit_Fail_TypeMismatch_ArgObjsAreTheWrongTypes) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
            ymParcelDef_AddComputedProperty(parceldef, "A", "abc", "yama:Int",
                ymInertCallBhvrFn, nullptr, ymInertCallBhvrFn, nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewBool(ctx, false)); // Wrong type for bb.
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "c,a,b", YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_TypeMismatch], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 3);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), cc);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), aa);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), bb);
            //
            //EXPECT_EQ(ymObj_RefCount(aa), 2);
            //EXPECT_EQ(ymObj_RefCount(bb), 2);
            //EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

TEST(Contexts, Call_WithReturnValuePuttingPushingAndDiscard) {
    auto setupfn =
        [](YmParcelDef* parceldef) {
        ymParcelDef_AddFn(
            parceldef,
            "g",
            "yama:Int",
            [](YmCtx* ctx, YmType* type, void* user) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                // Just passing observedCalls ptr via user so we can properly test that
                // impl passes correct ptr.
                EXPECT_EQ(user, (void*)&observedCalls);
            },
            (void*)&observedCalls);
        ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
        ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        };
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            // test w/ regular index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", 0), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            // test w/ negative index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", -1), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            // test w/ push

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_PUSH), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            // test w/ discard

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_DISCARD), YM_TRUE);
            
            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(y), 1);
            EXPECT_EQ(observedCalls, 1);
        });
}

namespace {
    inline bool plusWasNone = false, negateWasNone = false, timesTenWasNone = false;
}

TEST(Contexts, Call_WithNamedArgs) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddFn(parceldef, "g", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ASSERT_EQ(ymCtx_Args(ctx), 4); // Impl should insert nones for missing named args.
                auto x = ymCtx_Arg(ctx, 0, YM_BORROW);
                auto plus = ymCtx_Arg(ctx, 1, YM_BORROW);
                auto negate = ymCtx_Arg(ctx, 2, YM_BORROW);
                auto timesTen = ymCtx_Arg(ctx, 3, YM_BORROW);
                ASSERT_TRUE(x);
                ASSERT_TRUE(plus);
                ASSERT_TRUE(negate);
                ASSERT_TRUE(timesTen);
                ASSERT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                plusWasNone = ymObj_Type(plus) == ymCtx_LdNone(ctx);
                negateWasNone = ymObj_Type(negate) == ymCtx_LdNone(ctx);
                timesTenWasNone = ymObj_Type(timesTen) == ymCtx_LdNone(ctx);
                if (!plusWasNone) {
                    ASSERT_EQ(ymObj_Type(plus), ymCtx_LdInt(ctx));
                }
                if (!negateWasNone) {
                    ASSERT_EQ(ymObj_Type(negate), ymCtx_LdBool(ctx));
                }
                if (!timesTenWasNone) {
                    ASSERT_EQ(ymObj_Type(timesTen), ymCtx_LdBool(ctx));
                }
                YmInt v = ymObj_ToInt(x, nullptr);
                v += ymObj_ToInt(plus, nullptr); // Zero on fail.
                if (ymObj_ToBool(negate, nullptr) == YM_TRUE) { // YM_FALSE on fail.
                    v = -v;
                }
                if (ymObj_ToBool(timesTen, nullptr) == YM_TRUE) { // YM_FALSE on fail.
                    v *= 10;
                }
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, v), YM_TAKE);
            },
            nullptr);
        ymParcelDef_AddParam(parceldef, "g", "x", "yama:Int");
        ymParcelDef_BeginNamedParams(parceldef, "g");
        ymParcelDef_AddParam(parceldef, "g", "plus", "yama:Int");
        ymParcelDef_AddParam(parceldef, "g", "negate", "yama:Bool");
        ymParcelDef_AddParam(parceldef, "g", "timesTen", "yama:Bool");
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddMethod(parceldef, "A", "m", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ASSERT_EQ(ymCtx_Args(ctx), 4); // Impl should insert nones for missing named args.
                auto x = ymCtx_Arg(ctx, 0, YM_BORROW);
                auto plus = ymCtx_Arg(ctx, 1, YM_BORROW);
                auto negate = ymCtx_Arg(ctx, 2, YM_BORROW);
                auto timesTen = ymCtx_Arg(ctx, 3, YM_BORROW);
                ASSERT_TRUE(x);
                ASSERT_TRUE(plus);
                ASSERT_TRUE(negate);
                ASSERT_TRUE(timesTen);
                ASSERT_EQ(ymObj_Type(x), ymCtx_LdInt(ctx));
                plusWasNone = ymObj_Type(plus) == ymCtx_LdNone(ctx);
                negateWasNone = ymObj_Type(negate) == ymCtx_LdNone(ctx);
                timesTenWasNone = ymObj_Type(timesTen) == ymCtx_LdNone(ctx);
                if (!plusWasNone) {
                    ASSERT_EQ(ymObj_Type(plus), ymCtx_LdInt(ctx));
                }
                if (!negateWasNone) {
                    ASSERT_EQ(ymObj_Type(negate), ymCtx_LdBool(ctx));
                }
                if (!timesTenWasNone) {
                    ASSERT_EQ(ymObj_Type(timesTen), ymCtx_LdBool(ctx));
                }
                YmInt v = ymObj_ToInt(x, nullptr);
                v += ymObj_ToInt(plus, nullptr); // Zero on fail.
                if (ymObj_ToBool(negate, nullptr) == YM_TRUE) { // YM_FALSE on fail.
                    v = -v;
                }
                if (ymObj_ToBool(timesTen, nullptr) == YM_TRUE) { // YM_FALSE on fail.
                    v *= 10;
                }
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, v), YM_TAKE);
            },
            nullptr);
        ymParcelDef_AddParam(parceldef, "A::m", "x", "yama:Int");
        ymParcelDef_BeginNamedParams(parceldef, "A::m");
        ymParcelDef_AddParam(parceldef, "A::m", "plus", "yama:Int");
        ymParcelDef_AddParam(parceldef, "A::m", "negate", "yama:Bool");
        ymParcelDef_AddParam(parceldef, "A::m", "timesTen", "yama:Bool");
        };

    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");
            auto A_m = load(ctx, "p:A::m");

            auto test = [ctx](
                YmInt resultExpects,
                bool plusExpectsNotNone,
                bool negateExpectsNotNone,
                bool timesTenExpectsNotNone,
                std::function<void()> testfn) {
                    auto oldObservedCalls = observedCalls;
                    plusWasNone = false;
                    negateWasNone = false;
                    timesTenWasNone = false;
                    ymCtx_PopAll(ctx);
                    getErr().reset();
                    testfn();
                    EXPECT_EQ(observedCalls, oldObservedCalls + 1);
                    EXPECT_NE(plusWasNone, plusExpectsNotNone);
                    EXPECT_NE(negateWasNone, negateExpectsNotNone);
                    EXPECT_NE(timesTenWasNone, timesTenExpectsNotNone);
                    EXPECT_EQ(ymCtx_Locals(ctx), 1);
                    if (auto result = ymCtx_Local(ctx, 0, YM_BORROW)) {
                        ASSERT_EQ(ymObj_Type(result), ymCtx_LdInt(ctx));
                        EXPECT_EQ(ymObj_ToInt(result, nullptr), resultExpects);
                    }
                };

            // 0 named
            
            test(50, false, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                EXPECT_EQ(ymCtx_Call(ctx, g, 1, "", YM_PUSH), YM_TRUE);
                });
            test(50, false, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 1, "", YM_PUSH), YM_TRUE);
                });

            // 1 named, w/ each

            test(53, true, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "plus", YM_PUSH), YM_TRUE);
                });
            test(53, true, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "plus", YM_PUSH), YM_TRUE);
                });

            test(-50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "negate", YM_PUSH), YM_TRUE);
                });
            test(50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "negate", YM_PUSH), YM_TRUE);
                });
            test(-50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "negate", YM_PUSH), YM_TRUE);
                });
            test(50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "negate", YM_PUSH), YM_TRUE);
                });

            test(500, false, false, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "timesTen", YM_PUSH), YM_TRUE);
                });
            test(500, false, false, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "timesTen", YM_PUSH), YM_TRUE);
                });

            // 2 named, w/ reversed

            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "plus,negate", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "negate,plus", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "plus,negate", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "negate,plus", YM_PUSH), YM_TRUE);
                });

            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "timesTen,negate", YM_PUSH), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "timesTen,negate", YM_PUSH), YM_TRUE);
                });

            // 3 named, w/ diff orders

            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });

            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_PUSH), YM_TRUE);
                });
            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_PUSH, 50);
                ymCtx_PutBool(ctx, YM_PUSH, YM_FALSE);
                ymCtx_PutInt(ctx, YM_PUSH, 3);
                ymCtx_PutBool(ctx, YM_PUSH, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_PUSH), YM_TRUE);
                });
        });
}

TEST(Contexts, Call_WithDiffKindsOfCallableTypes) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            EXPECT_EQ(ymParcelDef_AddFn(parceldef, "g", "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 21), YM_TAKE);
                    observedCalls++;
                },
                nullptr),
                YM_TRUE);
            EXPECT_EQ(ymParcelDef_AddStruct(parceldef, "A"),
                YM_TRUE);
            EXPECT_EQ(ymParcelDef_AddMethod(parceldef, "A", "m", "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 13), YM_TAKE);
                    observedCalls++;
                },
                nullptr),
                YM_TRUE);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");
            auto A = load(ctx, "p:A");
            auto A_m = load(ctx, "p:A::m");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_Call(ctx, A_m, 0, "", YM_PUSH), YM_TRUE);

            auto g_result = ymCtx_Local(ctx, 0, YM_BORROW);
            auto A_m_result = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_TRUE(g_result);
            ASSERT_TRUE(A_m_result);
            EXPECT_EQ(ymObj_ToInt(g_result, nullptr), 21);
            EXPECT_EQ(ymObj_ToInt(A_m_result, nullptr), 13);
            EXPECT_EQ(observedCalls, 2);
        });
}

TEST(Contexts, Call_MultipleLevelsOfCalls) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:None",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
                    auto g = load(ctx, "p:g");
                    auto n_arg = ymCtx_Arg(ctx, 0, YM_BORROW);
                    ASSERT_TRUE(n_arg);
                    auto n = ymObj_ToUInt(n_arg, nullptr);
                    if (n > 1) {
                        ASSERT_EQ(ymCtx_PutUInt(ctx, YM_PUSH, n - 1), YM_TRUE);
                        ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_DISCARD), YM_TRUE);
                    }
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "n", "yama:UInt");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");
            ASSERT_EQ(ymCtx_PutUInt(ctx, YM_PUSH, 10), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_DISCARD), YM_TRUE);
            EXPECT_EQ(observedCalls, 10);
        });
}

TEST(Contexts, Call_Fail_LocalNotFound_ReturnToIsOutOfBounds) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddFn(
            parceldef,
            "g",
            "yama:Int",
            [](YmCtx* ctx, YmType* type, void* user) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
            },
            nullptr);
        ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
        ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", 10'000), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 2);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            //EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", -10'000), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 2);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            //EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_NonCallableType_FnIsNonCallable) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, ymCtx_LdInt(ctx), 2, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_NonCallableType], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 2);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            //EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_LocalNotFound_ArgsExceedsLocalObjectStackHeight) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 1);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooManyPositionalArgs) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 3, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 3);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), x);
            //EXPECT_EQ(ymObj_RefCount(x), 3);
            //EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooFewPositionalArgs) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 1);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooFewPositionalArgs_DueToNamedArgs) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "a", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "b", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(a, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -3));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, a, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            CtxState initial(*ctx);
            ASSERT_EQ(ymCtx_Call(ctx, g, 3, "a,b", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);
            EXPECT_TRUE(initial.expect(*ctx));

            //ASSERT_EQ(ymCtx_Locals(ctx), 3);
            //EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            //EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), a);
            //EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), b);
            //EXPECT_EQ(ymObj_RefCount(x), 2);
            //EXPECT_EQ(ymObj_RefCount(a), 2);
            //EXPECT_EQ(ymObj_RefCount(b), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_IllegalNameList_ArgNameIdentifierMultipleTimes) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "a", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "b", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "b,b", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_IllegalNameList], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), b);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), b);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_IllegalNameList_ArgNameUnknownIdentifier) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "a", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "b", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "missing,b", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_IllegalNameList], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), b);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), b);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_IllegalNameList_ArgNamePositionalParamIdentifier) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "a", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "b", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "x,b", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_IllegalNameList], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 4);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), b);
            EXPECT_EQ(ymCtx_Local(ctx, 3, YM_BORROW), b);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_TypeMismatch_ArgsAreWrongTypes) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_TypeMismatch], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_TypeMismatch_ArgsAreWrongTypes_DueToNamedArgs) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "x,y", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_TypeMismatch], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_NoReturnValueObjectBound) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    //ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            // Take note that, for this error, call behaviour occurs (ie. that's what resulted
            // in the error) but otherwise the usual non-consuming of passed arg objects is still
            // respected.

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_ReturnValueIsWrongType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    // Arg #1 is the Float, not the Int.
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_PUSH, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            // Take note that, for this error, call behaviour occurs (ie. that's what resulted
            // in the error) but otherwise the usual non-consuming of passed arg objects is still
            // respected.

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, Call_Fail_CallStackOverflow) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:None",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
                    auto g = load(ctx, "p:g");
                    if (ymCtx_CallStackHeight(ctx) < YM_MAX_CALL_STACK_HEIGHT) {
                        ASSERT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
                    }
                    else {
                        ASSERT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_FALSE);
                        ASSERT_EQ(getErr()[YmErrCode_CallStackOverflow], 1);
                    }
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = load(ctx, "p:g");
            ASSERT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
            if (called_in_fn_body) {
                EXPECT_EQ(observedCalls, YM_MAX_CALL_STACK_HEIGHT - 2);
            }
            else {
                EXPECT_EQ(observedCalls, YM_MAX_CALL_STACK_HEIGHT - 1);
            }
        });
}

TEST(Contexts, Ret_Borrow) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    SETUP_OBJ(x, ymCtx_NewInt(ctx, 100));

                    // The full call procedure is tested in ymCtx_Call tests, w/ us
                    // here only caring about ymCtx_Ret's immediate behaviour.
                    ymCtx_Ret(ctx, x, YM_BORROW);

                    EXPECT_EQ(ymObj_RefCount(x), 2);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_Take) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    auto x = ymCtx_NewInt(ctx, 100);

                    // The full call procedure is tested in ymCtx_Call tests, w/ us
                    // here only caring about ymCtx_Ret's immediate behaviour.
                    ymCtx_Ret(ctx, x, YM_TAKE);

                    // x's single ref is owned by API internals.
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_TakeIfOk) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    auto x = ymCtx_NewInt(ctx, 100);

                    // The full call procedure is tested in ymCtx_Call tests, w/ us
                    // here only caring about ymCtx_Ret's immediate behaviour.
                    ymCtx_Ret(ctx, x, YM_TAKE_IF_OK);

                    // x's single ref is owned by API internals.
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_Overwrite) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    SETUP_OBJ(x, ymCtx_NewInt(ctx, 100));
                    SETUP_OBJ(y, ymCtx_NewInt(ctx, 200));
                    SETUP_OBJ(z, ymCtx_NewInt(ctx, 300));

                    // The full call procedure is tested in ymCtx_Call tests, w/ us
                    // here only caring about ymCtx_Ret's immediate behaviour.
                    ymCtx_Ret(ctx, x, YM_BORROW);
                    EXPECT_EQ(ymObj_RefCount(x), 2);
                    EXPECT_EQ(ymObj_RefCount(y), 1);
                    EXPECT_EQ(ymObj_RefCount(z), 1);

                    ymCtx_Ret(ctx, y, YM_BORROW);
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_RefCount(y), 2);
                    EXPECT_EQ(ymObj_RefCount(z), 1);

                    ymCtx_Ret(ctx, z, YM_BORROW);
                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    EXPECT_EQ(ymObj_RefCount(y), 1);
                    EXPECT_EQ(ymObj_RefCount(z), 2);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_FailQuietly_InUserCallFrame) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (called_in_fn_body) {
                return;
            }
            SETUP_OBJ(a, ymCtx_NewInt(ctx, 10));

            ymCtx_Ret(ctx, a, YM_BORROW);
            EXPECT_EQ(ymObj_RefCount(a), 1);
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (called_in_fn_body) {
                return;
            }
            SETUP_OBJ(a, ymCtx_NewInt(ctx, 10));
            ymObj_Secure(a); // Ref will be consumed by API.

            ymCtx_Ret(ctx, a, YM_TAKE);
            EXPECT_EQ(ymObj_RefCount(a), 1);
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (called_in_fn_body) {
                return;
            }
            SETUP_OBJ(a, ymCtx_NewInt(ctx, 10));

            ymCtx_Ret(ctx, a, YM_TAKE_IF_OK);
            EXPECT_EQ(ymObj_RefCount(a), 1);
        });
}

TEST(Contexts, Ret_FailQuietly_WhatIsNullptr) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    ymCtx_Ret(ctx, YM_NIL, YM_BORROW);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE_IF_OK);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_FALSE);
        });
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    SETUP_OBJ(x, ymCtx_NewInt(ctx, 100));

                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    ymCtx_Ret(ctx, x, YM_BORROW);
                    EXPECT_EQ(ymObj_RefCount(x), 2);

                    // Test that impl can handle w/ already existing binding.
                    ymCtx_Ret(ctx, YM_NIL, YM_BORROW);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE_IF_OK);

                    EXPECT_EQ(ymObj_RefCount(x), 2);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = load(ctx, "p:g");

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, GetVar_StoredVar) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddReadOnlyStoredVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 113), YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            // test w/ regular index

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, 0), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, 1), YM_TRUE);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_TRUE(a);
            ASSERT_TRUE(b);

            EXPECT_EQ(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            // test w/ negative index

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, -2), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, -1), YM_TRUE);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_TRUE(a);
            ASSERT_TRUE(b);

            EXPECT_EQ(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            // test w/ push

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_TRUE(a);
            ASSERT_TRUE(b);

            EXPECT_EQ(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            // test w/ discard

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_DISCARD), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, GetVar_StoredVarLazyInits) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddReadOnlyStoredVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 113), YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW); // Lazy inits.
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            auto c = ymCtx_Local(ctx, 2, YM_BORROW);
            ASSERT_TRUE(a);
            ASSERT_TRUE(b);
            ASSERT_TRUE(c);

            EXPECT_EQ(a, b);
            EXPECT_EQ(a, c);
            EXPECT_EQ(b, c);

            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(c, nullptr), 113);

            EXPECT_EQ(observedCalls, 1); // Lazy inits only once.
        });
}

namespace {
    YmObj* compVar_obj = nullptr;
}

TEST(Contexts, GetVar_ComputedVar) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddReadOnlyComputedVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                auto result = ymCtx_NewInt(ctx, 113);
                compVar_obj = result;
                ymCtx_Ret(ctx, result, YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");
            compVar_obj = nullptr;

            // test w/ regular index

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);

            EXPECT_EQ(ymCtx_GetVar(ctx, V, 0), YM_TRUE);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(a);
            EXPECT_EQ(a, compVar_obj);

            EXPECT_EQ(ymCtx_GetVar(ctx, V, 1), YM_TRUE);

            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_TRUE(b);
            EXPECT_EQ(b, compVar_obj);

            EXPECT_NE(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_RefCount(b), 1);
            EXPECT_EQ(observedCalls, 2);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");
            compVar_obj = nullptr;

            // test w/ negative index

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);

            EXPECT_EQ(ymCtx_GetVar(ctx, V, -2), YM_TRUE);
            
            auto a = ymCtx_Local(ctx, -2, YM_BORROW);
            ASSERT_TRUE(a);
            EXPECT_EQ(a, compVar_obj);

            EXPECT_EQ(ymCtx_GetVar(ctx, V, -1), YM_TRUE);

            auto b = ymCtx_Local(ctx, -1, YM_BORROW);
            ASSERT_TRUE(b);
            EXPECT_EQ(b, compVar_obj);

            EXPECT_NE(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_RefCount(b), 1);
            EXPECT_EQ(observedCalls, 2);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");
            compVar_obj = nullptr;

            // test w/ push

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);

            auto a = ymCtx_Local(ctx, -1, YM_BORROW);
            ASSERT_TRUE(a);
            EXPECT_EQ(a, compVar_obj);

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);

            auto b = ymCtx_Local(ctx, -1, YM_BORROW);
            ASSERT_TRUE(b);
            EXPECT_EQ(b, compVar_obj);

            EXPECT_NE(a, b);
            EXPECT_EQ(ymObj_ToInt(a, nullptr), 113);
            EXPECT_EQ(ymObj_ToInt(b, nullptr), 113);
            EXPECT_EQ(ymObj_RefCount(a), 1);
            EXPECT_EQ(ymObj_RefCount(b), 1);
            EXPECT_EQ(observedCalls, 2);
        });
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");
            compVar_obj = nullptr;

            // test w/ discard

            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_DISCARD), YM_TRUE);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_DISCARD), YM_TRUE);

            // Should be non-nullptr, but we can't deref as the mem's been deallocated.
            EXPECT_TRUE(compVar_obj);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 2);
        });
}

TEST(Contexts, SetVar_StoredVar) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStoredVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 113), YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            auto x = ymCtx_NewInt(ctx, 101);
            ASSERT_TRUE(x);

            ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);

            EXPECT_EQ(observedCalls, 1);

            ASSERT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);

            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, SetVar_StoredVarLazyInits) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStoredVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 113), YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            auto x = ymCtx_NewInt(ctx, 101);
            ASSERT_TRUE(x);

            EXPECT_EQ(observedCalls, 0);

            ymCtx_Put(ctx, YM_PUSH, x, YM_BORROW);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            EXPECT_EQ(observedCalls, 1);

            ymCtx_PopAll(ctx);
            EXPECT_EQ(ymCtx_GetVar(ctx, V, YM_PUSH), YM_TRUE);

            EXPECT_EQ(x, ymCtx_Local(ctx, 0, YM_BORROW));

            EXPECT_EQ(observedCalls, 1); // Lazy inits only once.
        });
}

TEST(Contexts, SetVar_ComputedVar) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddComputedVar(parceldef, "V", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                ADD_FAILURE() << "Shouldn't reach!";
            },
            nullptr,
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                EXPECT_EQ(ymCtx_Arg(ctx, 0, YM_BORROW), compVar_obj);
                ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
            },
            nullptr);
        };
    objsys_test(setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto V = load(ctx, "p:V");

            compVar_obj = ymCtx_NewInt(ctx, 101);
            ymCtx_Put(ctx, YM_PUSH, compVar_obj, YM_TAKE);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 1);

            ymCtx_PopAll(ctx);

            compVar_obj = ymCtx_NewInt(ctx, 101);
            ymCtx_Put(ctx, YM_PUSH, compVar_obj, YM_TAKE);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 2);

            ymCtx_PopAll(ctx);

            compVar_obj = ymCtx_NewInt(ctx, 101);
            ymCtx_Put(ctx, YM_PUSH, compVar_obj, YM_TAKE);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 3);

            ymCtx_PopAll(ctx);

            compVar_obj = ymCtx_NewInt(ctx, 101);
            ymCtx_Put(ctx, YM_PUSH, compVar_obj, YM_TAKE);
            EXPECT_EQ(ymCtx_SetVar(ctx, V), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 4);
        });
}

TEST(Contexts, GetProperty_StoredProperty) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
        ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ regular index

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.41));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH), YM_TRUE);

            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, 1), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, 2), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 3);

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, 3), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);

            auto a = ymCtx_Local(ctx, 1, YM_BORROW);
            auto b = ymCtx_Local(ctx, 2, YM_BORROW);
            auto c = ymCtx_Local(ctx, 3, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);
            ASSERT_EQ(c, cc);

            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(ymObj_RefCount(c), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ negative index

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.41));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH), YM_TRUE);

            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, -3), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, -2), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, -1), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);

            auto a = ymCtx_Local(ctx, 1, YM_BORROW);
            auto b = ymCtx_Local(ctx, 2, YM_BORROW);
            auto c = ymCtx_Local(ctx, 3, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);
            ASSERT_EQ(c, cc);

            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(ymObj_RefCount(c), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ push

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.41));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH), YM_TRUE);

            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 3);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 4);

            auto a = ymCtx_Local(ctx, 1, YM_BORROW);
            auto b = ymCtx_Local(ctx, 2, YM_BORROW);
            auto c = ymCtx_Local(ctx, 3, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);
            ASSERT_EQ(c, cc);

            EXPECT_EQ(ymObj_RefCount(a), 3);
            EXPECT_EQ(ymObj_RefCount(b), 3);
            EXPECT_EQ(ymObj_RefCount(c), 3);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");
            auto A_c = load(ctx, "p:A::c");

            // test w/ discard

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.41));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH), YM_TRUE);

            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_DISCARD), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_DISCARD), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_c, YM_DISCARD), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            EXPECT_EQ(ymObj_RefCount(aa), 2);
            EXPECT_EQ(ymObj_RefCount(bb), 2);
            EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

namespace {
    inline YmObj* compProp_A_a_obj = nullptr, * compProp_A_b_obj = nullptr;
}

TEST(Contexts, GetProperty_ComputedProperty) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddReadOnlyComputedProperty(parceldef, "A", "a", "yama:Int",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, compProp_A_a_obj, YM_BORROW);
            },
            nullptr);
        ymParcelDef_AddComputedProperty(parceldef, "A", "b", "yama:Rune",
            [](YmCtx* ctx, YmType* type, void*) {
                observedCalls++;
                ymCtx_Ret(ctx, compProp_A_b_obj, YM_BORROW);
            },
            nullptr,
            [](YmCtx* ctx, YmType* type, void*) {
                //
            },
            nullptr);
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");

            // test w/ regular index

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewRune(ctx, U'y'));

            compProp_A_a_obj = aa;
            compProp_A_b_obj = bb;

            ymCtx_PutNone(ctx, YM_PUSH);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, 0), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(observedCalls, 1);

            ymCtx_PutNone(ctx, YM_PUSH);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, 1), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(observedCalls, 2);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);

            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_RefCount(b), 2);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");

            // test w/ negative index

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewRune(ctx, U'y'));

            compProp_A_a_obj = aa;
            compProp_A_b_obj = bb;

            ymCtx_PutNone(ctx, YM_PUSH);
            ymCtx_PutNone(ctx, YM_PUSH);

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, -2), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(observedCalls, 1);

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, -1), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(observedCalls, 2);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);

            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_RefCount(b), 2);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");

            // test w/ push

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewRune(ctx, U'y'));

            compProp_A_a_obj = aa;
            compProp_A_b_obj = bb;

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(observedCalls, 1);

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(observedCalls, 2);

            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            auto b = ymCtx_Local(ctx, 1, YM_BORROW);
            ASSERT_EQ(a, aa);
            ASSERT_EQ(b, bb);

            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_RefCount(b), 2);
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");
            auto A_b = load(ctx, "p:A::b");

            // test w/ discard

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewRune(ctx, U'y'));

            compProp_A_a_obj = aa;
            compProp_A_b_obj = bb;

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_DISCARD), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 1);

            ASSERT_EQ(ymCtx_StructInit(ctx, A, "", YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_DISCARD), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 0);
            EXPECT_EQ(observedCalls, 2);

            EXPECT_EQ(ymObj_RefCount(aa), 1);
            EXPECT_EQ(ymObj_RefCount(bb), 1);
        });
}

TEST(Contexts, GetProperty_Fail_LocalNotFound_WhereIsOutOfBounds) {
    auto setup = [](YmParcelDef* parceldef) {
        ymParcelDef_AddStruct(parceldef, "A");
        ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
        ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        };
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, -4);
            ymCtx_PutFloat(ctx, YM_PUSH, 10.41);
            ymCtx_PutRune(ctx, YM_PUSH, U'y');
            ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH);
            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);

            EXPECT_EQ(ymCtx_GetProperty(ctx, A_a, 10'000), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            // TODO: What about post-conditions?
        });
    objsys_test(
        setup,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, -4);
            ymCtx_PutFloat(ctx, YM_PUSH, 10.41);
            ymCtx_PutRune(ctx, YM_PUSH, U'y');
            ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH);
            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);

            EXPECT_EQ(ymCtx_GetProperty(ctx, A_a, -10'000), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, GetProperty_Fail_NonPropertyType) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            EXPECT_EQ(ymCtx_GetProperty(ctx, ymCtx_LdInt(ctx), YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_NonPropertyType], 1);
        });
}

TEST(Contexts, GetProperty_Fail_LocalNotFound_ObjStkIsEmpty) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A_a = load(ctx, "p:A::a");

            EXPECT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, GetProperty_Fail_TypeMismatch_SubjectIsWrongType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, -4);
            auto Int_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(Int_obj);

            EXPECT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_TypeMismatch], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, GetProperty_Fail_CallProcedureError_ComputedPropertyGetCallFails) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddReadOnlyComputedProperty(
                parceldef,
                "A",
                "a",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void* user) {
                    observedCalls++;
                    // No return value bound.
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_StructInit(ctx, A, "", YM_PUSH);
            auto a = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(a);

            ymCtx_Put(ctx, YM_PUSH, a, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_a, YM_PUSH), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            // Take note that, for this error, call behaviour occurs (ie. that's what resulted
            // in the error) but otherwise the usual non-consuming of passed arg objects is still
            // respected.

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), a);
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, SetProperty_StoredProperty) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
            ymParcelDef_AddStoredProperty(parceldef, "A", "b", "yama:Float");
            ymParcelDef_AddStoredProperty(parceldef, "A", "c", "yama:Rune");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_b = load(ctx, "p:A::b");

            SETUP_OBJ(xx, ymCtx_NewFloat(ctx, -0.031));

            SETUP_OBJ(aa, ymCtx_NewInt(ctx, -4));
            SETUP_OBJ(bb, ymCtx_NewFloat(ctx, 10.41));
            SETUP_OBJ(cc, ymCtx_NewRune(ctx, U'y'));

            ymCtx_Put(ctx, YM_PUSH, aa, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, bb, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, cc, YM_BORROW);
            ASSERT_EQ(ymCtx_StructInit(ctx, A, "a,b,c", YM_PUSH), YM_TRUE);

            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);
            ASSERT_EQ(ymCtx_Locals(ctx), 1);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, xx, YM_BORROW);
            ASSERT_EQ(ymCtx_SetProperty(ctx, A_b), YM_TRUE);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ASSERT_EQ(ymCtx_GetProperty(ctx, A_b, YM_PUSH), YM_TRUE);
            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), xx);

            EXPECT_EQ(ymObj_RefCount(xx), 3);
            EXPECT_EQ(ymObj_RefCount(aa), 2);
            EXPECT_EQ(ymObj_RefCount(bb), 1);
            EXPECT_EQ(ymObj_RefCount(cc), 2);
        });
}

namespace {
    inline YmObj* compProp_expected_by_setter = nullptr;
}

TEST(Contexts, SetProperty_ComputedProperty) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddComputedProperty(parceldef, "A", "a", "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    //
                },
                nullptr,
                [](YmCtx* ctx, YmType* type, void*) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
                    EXPECT_EQ(ymCtx_Arg(ctx, 1, YM_BORROW), compProp_expected_by_setter);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            SETUP_OBJ(xx, ymCtx_NewInt(ctx, -31));
            compProp_expected_by_setter = xx;

            ymCtx_StructInit(ctx, A, "", YM_PUSH);
            ymCtx_Put(ctx, YM_PUSH, xx, YM_BORROW);
            ASSERT_EQ(ymCtx_SetProperty(ctx, A_a), YM_TRUE);
            EXPECT_EQ(ymCtx_Locals(ctx), 0);

            EXPECT_EQ(ymObj_RefCount(xx), 1);
        });
}

TEST(Contexts, SetProperty_Fail_NonPropertyType) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            EXPECT_EQ(ymCtx_SetProperty(ctx, ymCtx_LdInt(ctx)), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_NonPropertyType], 1);
        });
}

TEST(Contexts, SetProperty_Fail_ReadOnlyPropertyType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddReadOnlyStoredProperty(parceldef, "A", "a", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            SETUP_OBJ(xx, ymCtx_NewInt(ctx, 14));

            ymCtx_Put(ctx, YM_PUSH, xx, YM_BORROW);
            ymCtx_StructInit(ctx, A, "a", YM_PUSH);
            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, xx, YM_BORROW);
            EXPECT_EQ(ymCtx_SetProperty(ctx, A_a), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_ReadOnlyPropertyType], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), A_obj);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), A_obj);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), xx);

            EXPECT_EQ(ymObj_RefCount(A_obj), 2);
            EXPECT_EQ(ymObj_RefCount(xx), 3);
        });
}

TEST(Contexts, SetProperty_Fail_LocalNotFound_ObjStkHasFewerThanTwoObjs) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, 14);
            ymCtx_StructInit(ctx, A, "", YM_PUSH);
            EXPECT_EQ(ymCtx_SetProperty(ctx, A_a), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_LocalNotFound], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, SetProperty_Fail_TypeMismatch_SubjectIsWrongType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, 14); // Wrong subject type!
            ymCtx_PutInt(ctx, YM_PUSH, 14);
            EXPECT_EQ(ymCtx_SetProperty(ctx, A_a), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_TypeMismatch], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, SetProperty_Fail_TypeMismatch_ValueIsWrongType) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddStoredProperty(parceldef, "A", "a", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            ymCtx_PutInt(ctx, YM_PUSH, 14);
            ymCtx_StructInit(ctx, A, "", YM_PUSH);
            ymCtx_PutFloat(ctx, YM_PUSH, 1.041); // Wrong value type!
            EXPECT_EQ(ymCtx_SetProperty(ctx, A_a), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_TypeMismatch], 1);

            // TODO: What about post-conditions?
        });
}

TEST(Contexts, SetProperty_Fail_CallProcedureError_ComputedPropertyGetCallFails) {
    objsys_test(
        [](YmParcelDef* parceldef) {
            ymParcelDef_AddStruct(parceldef, "A");
            ymParcelDef_AddComputedProperty(
                parceldef,
                "A",
                "a",
                "yama:Int",
                [](YmCtx* ctx, YmType* type, void*) {
                    //
                },
                nullptr,
                [](YmCtx* ctx, YmType* type, void*) {
                    observedCalls++;
                    // No return value bound.
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto A = load(ctx, "p:A");
            auto A_a = load(ctx, "p:A::a");

            SETUP_OBJ(xx, ymCtx_NewInt(ctx, 14));

            ymCtx_StructInit(ctx, A, "", YM_PUSH);
            auto A_obj = ymCtx_Local(ctx, 0, YM_BORROW);
            ASSERT_TRUE(A_obj);

            ymCtx_Put(ctx, YM_PUSH, A_obj, YM_BORROW);
            ymCtx_Put(ctx, YM_PUSH, xx, YM_BORROW);
            ASSERT_EQ(ymCtx_SetProperty(ctx, A_a), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            // Take note that, for this error, call behaviour occurs (ie. that's what resulted
            // in the error) but otherwise the usual non-consuming of passed arg objects is still
            // respected.

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), A_obj);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), A_obj);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), xx);
            EXPECT_EQ(ymObj_RefCount(A_obj), 2);
            EXPECT_EQ(ymObj_RefCount(xx), 2);
            EXPECT_EQ(observedCalls, 1);
        });
}

TEST(Contexts, Convert) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(UInt);

            // Test w/ regular index.

            ASSERT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE); // Result goes here.
            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, 0), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), UInt);
                YmBool success{};
                EXPECT_EQ(ymObj_ToUInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(UInt);

            // Test w/ negative index.

            ASSERT_EQ(ymCtx_PutNone(ctx, YM_PUSH), YM_TRUE); // Result goes here.
            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, -1), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), UInt);
                YmBool success{};
                EXPECT_EQ(ymObj_ToUInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(UInt);

            // Test w/ push.

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, YM_PUSH), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), UInt);
                YmBool success{};
                EXPECT_EQ(ymObj_ToUInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(UInt);

            // Test w/ discard.

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, YM_DISCARD), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, Convert_Fail_LocalNotFound_ReturnToIsOutOfBounds) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Int = ymCtx_LdInt(ctx);
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(Int);
            ASSERT_TRUE(UInt);

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, 100), YM_FALSE);
            ASSERT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            // Ensure stack was left unchanged.
            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), Int);
                YmBool success{};
                EXPECT_EQ(ymObj_ToInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Int = ymCtx_LdInt(ctx);
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(Int);
            ASSERT_TRUE(UInt);

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, -100), YM_FALSE);
            ASSERT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            // Ensure stack was left unchanged.
            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), Int);
                YmBool success{};
                EXPECT_EQ(ymObj_ToInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
}

TEST(Contexts, Convert_Fail_LocalNotFound_LocalObjectStackIsEmpty) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Int = ymCtx_LdInt(ctx);
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(Int);
            ASSERT_TRUE(UInt);

            ASSERT_EQ(ymCtx_Convert(ctx, UInt, YM_PUSH), YM_FALSE);
            ASSERT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            // Ensure stack was left unchanged.
            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, Convert_Fail_IllegalConversion) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Int = ymCtx_LdInt(ctx);
            auto Type = ymCtx_LdType(ctx);
            ASSERT_TRUE(Int);
            ASSERT_TRUE(Type);

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_PUSH, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, Type, YM_PUSH), YM_FALSE);
            ASSERT_EQ(getErr()[YmErrCode_IllegalConversion], 1);

            // Ensure stack was left unchanged.
            EXPECT_EQ(ymCtx_Locals(ctx), 1);
            if (auto x = ymCtx_Local(ctx, 0, YM_BORROW)) {
                EXPECT_EQ(ymObj_Type(x), Int);
                YmBool success{};
                EXPECT_EQ(ymObj_ToInt(x, &success), 13);
                EXPECT_EQ(success, YM_TRUE);
            }
            else ADD_FAILURE();
        });
}

