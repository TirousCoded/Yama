

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>

#include "../utils/utils.h"


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
    std::function<void(YmParcelDef* parceldef, YmTypeIndex f_ind)> setup,
    std::function<void(YmCtx* ctx, bool called_in_fn_body)> body) {
    SETUP_ERRCOUNTER;
    SETUP_DM;
    SETUP_PARCELDEF(p_def);
    _err = &err;
    auto f_ind = ymParcelDef_AddFn(
        p_def, "f", "yama:None",
        [](YmCtx* ctx, void* body_ptr) {
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
    setup(p_def, f_ind);

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

        ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
        ASSERT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_Call(ctx, f, 2, "", YM_DISCARD), YM_TRUE);
    }
}

static void objsys_test(
    std::function<void(YmCtx* ctx, bool called_in_fn_body)> body) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {},
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

        EXPECT_EQ(ymCtx_SetArg(ctx, 10'000, newArg, YM_TAKE), YM_FALSE);
        EXPECT_GE(getErr()[YmErrCode_ArgNotFound], 1);

        EXPECT_EQ(ymCtx_SetArg(ctx, 10'000, newArg, YM_BORROW), YM_FALSE);
        EXPECT_GE(getErr()[YmErrCode_ArgNotFound], 2);

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

        EXPECT_EQ(ymCtx_SetArg(ctx, 0, newArg, YM_TAKE), YM_FALSE); // Quiet
        EXPECT_EQ(ymCtx_SetArg(ctx, 0, newArg, YM_BORROW), YM_FALSE); // Quiet

        EXPECT_EQ(ymObj_RefCount(newArg), 1);
        });
}

TEST(Contexts, Ref) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "p:A"), 0);
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "yama:Float"), 1);
            EXPECT_EQ(ymParcelDef_AddRef(parceldef, "f", "p:A"), 2);
            EXPECT_NE(ymParcelDef_AddStruct(parceldef, "A"), YM_NO_TYPE_INDEX);
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
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_Locals(ctx), 1);
        });
}

TEST(Contexts, Local_Take) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);

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
        });
}

TEST(Contexts, Local_Borrow) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);

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
        });
}

TEST(Contexts, Pop) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);

        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);

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

TEST(Contexts, PopAll) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        auto aa = ymCtx_NewInt(ctx, 50);

        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);

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

        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, cc, YM_BORROW), YM_TRUE);

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

        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, 3);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Putting

        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 1);
        EXPECT_EQ(ymObj_RefCount(bb), 1);
        EXPECT_EQ(ymObj_RefCount(cc), 1);
        EXPECT_EQ(ymObj_RefCount(dd), 4);

        EXPECT_EQ(ymCtx_Put(ctx, 0, aa, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 1, bb, YM_BORROW), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 2, cc, YM_BORROW), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);
        EXPECT_EQ(ymObj_RefCount(dd), 1);

        ymCtx_Pop(ctx, 3);
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

        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, bb, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, cc, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), aa);
        EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), bb);
        EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), cc);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);

        ymCtx_Pop(ctx, 3);
        ASSERT_EQ(ymCtx_Locals(ctx), 0);

        // Putting

        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);
        ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, dd, YM_BORROW), YM_TRUE);

        ymObj_Secure(aa);
        ymObj_Secure(bb);
        ymObj_Secure(cc);
        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);
        EXPECT_EQ(ymObj_RefCount(dd), 4);

        EXPECT_EQ(ymCtx_Put(ctx, 0, aa, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 1, bb, YM_TAKE), YM_TRUE);
        EXPECT_EQ(ymCtx_Put(ctx, 2, cc, YM_TAKE), YM_TRUE);

        EXPECT_EQ(ymObj_RefCount(aa), 2);
        EXPECT_EQ(ymObj_RefCount(bb), 2);
        EXPECT_EQ(ymObj_RefCount(cc), 2);
        EXPECT_EQ(ymObj_RefCount(dd), 1);

        ymCtx_Pop(ctx, 3);
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

TEST(Contexts, Put_Fail_LocalNotFound) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        SETUP_OBJ(a, ymCtx_NewInt(ctx, 50));

        EXPECT_EQ(ymCtx_Put(ctx, 0, a, YM_BORROW), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

        EXPECT_EQ(ymCtx_Put(ctx, 0, a, YM_TAKE), YM_FALSE);
        EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 2);

        EXPECT_EQ(ymObj_RefCount(a), 1);
        });
}

TEST(Contexts, PutXXXX) {
    objsys_test([](YmCtx* ctx, bool called_in_fn_body) {
        // Test w/ pushing.
        EXPECT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, -10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, YM_NEWTOP, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, YM_NEWTOP, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, YM_NEWTOP, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, YM_NEWTOP, ymCtx_LdInt(ctx)), YM_TRUE);

        auto num = ymCtx_Locals(ctx);

        // Setup for test w/ putting.
        SETUP_OBJ(temp, ymCtx_NewInt(ctx, 100));
        for (YmLocal i = 0; i < num; i++) {
            EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, temp, YM_BORROW), YM_TRUE) << "i==" << i;
        }
        ASSERT_EQ(ymCtx_Locals(ctx), num * 2);

        // Before puts overwrite temp locals.
        EXPECT_EQ(ymObj_RefCount(temp), num + 1);

        // Perform putting.
        EXPECT_EQ(ymCtx_PutNone(ctx, num + 0), YM_TRUE);
        EXPECT_EQ(ymCtx_PutInt(ctx, num + 1, -10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutUInt(ctx, num + 2, 10), YM_TRUE);
        EXPECT_EQ(ymCtx_PutFloat(ctx, num + 3, 3.14159), YM_TRUE);
        EXPECT_EQ(ymCtx_PutBool(ctx, num + 4, YM_TRUE), YM_TRUE);
        EXPECT_EQ(ymCtx_PutRune(ctx, num + 5, U'y'), YM_TRUE);
        EXPECT_EQ(ymCtx_PutType(ctx, num + 6, ymCtx_LdInt(ctx)), YM_TRUE);

        // After puts overwrite temp locals.
        EXPECT_EQ(ymObj_RefCount(temp), 1);

        // Now check everything (we loop twice for each as the sequence should be the
        // same for both pushing and putting.)
        for (YmLocal i = 0; i < 2; i++) {
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
}

TEST(Contexts, PutDefault) {
    // NOTE: Don't forget to update PutDefault_Fail_NoDefaultValue too.
    // NOTE: Also, update the YM_DISCARD tests too.
    auto setupfn =
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
        // Empty Struct
        auto Struct0_ind = ymParcelDef_AddStruct(parceldef,
            "Struct0"
        );
        EXPECT_NE(Struct0_ind, YM_NO_TYPE_INDEX);

        // Method (of Struct0)
        EXPECT_NE(ymParcelDef_AddMethod(parceldef,
            "Struct0",
            "m",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_NO_TYPE_INDEX);

        // Fn
        EXPECT_NE(ymParcelDef_AddFn(parceldef,
            "Fn0",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_NO_TYPE_INDEX);
        };
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Struct0 = ymCtx_Load(ctx, "p:Struct0");
            ASSERT_TRUE(Struct0);

            // Test w/ pushing.
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdNone(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdUInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdFloat(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdBool(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdRune(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, ymCtx_LdType(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, Struct0), YM_TRUE);

            auto num = ymCtx_Locals(ctx);

            // Setup for test w/ putting.
            SETUP_OBJ(temp, ymCtx_NewInt(ctx, 100));
            for (YmLocal i = 0; i < num; i++) {
                EXPECT_EQ(ymCtx_Put(ctx, YM_NEWTOP, temp, YM_BORROW), YM_TRUE) << "i==" << i;
            }
            ASSERT_EQ(ymCtx_Locals(ctx), num * 2);

            // Before puts overwrite temp locals.
            EXPECT_EQ(ymObj_RefCount(temp), num + 1);

            // Perform putting.
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 0, ymCtx_LdNone(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 1, ymCtx_LdInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 2, ymCtx_LdUInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 3, ymCtx_LdFloat(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 4, ymCtx_LdBool(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 5, ymCtx_LdRune(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 6, ymCtx_LdType(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, num + 7, Struct0), YM_TRUE);

            // After puts overwrite temp locals.
            EXPECT_EQ(ymObj_RefCount(temp), 1);

            // Now check everything (we loop twice for each as the sequence should be the
            // same for both pushing and putting.)
            for (YmLocal i = 0; i < 2; i++) {
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
            auto Struct0 = ymCtx_Load(ctx, "p:Struct0");
            ASSERT_TRUE(Struct0);

            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdNone(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdUInt(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdFloat(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdBool(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdRune(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, ymCtx_LdType(ctx)), YM_TRUE);
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_DISCARD, Struct0), YM_TRUE);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, PutDefault_Fail_LocalNotFound) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            EXPECT_EQ(ymCtx_PutDefault(ctx, 0, ymCtx_LdInt(ctx)), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            EXPECT_EQ(ymCtx_Locals(ctx), 0);
        });
}

TEST(Contexts, PutDefault_Fail_NoDefaultValue) {
    auto setupfn =
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
        // Empty Struct
        auto Struct0_ind = ymParcelDef_AddStruct(parceldef,
            "Struct0"
        );
        EXPECT_NE(Struct0_ind, YM_NO_TYPE_INDEX);

        // Method (of Struct0)
        EXPECT_NE(ymParcelDef_AddMethod(parceldef,
            "Struct0",
            "m",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_NO_TYPE_INDEX);

        // Empty Protocol
        EXPECT_NE(ymParcelDef_AddProtocol(parceldef,
            "Protocol0"
        ), YM_NO_TYPE_INDEX);

        // Fn
        EXPECT_NE(ymParcelDef_AddFn(parceldef,
            "Fn0",
            "yama:None",
            ymInertCallBhvrFn,
            nullptr
        ), YM_NO_TYPE_INDEX);
        };
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Struct0_m = ymCtx_Load(ctx, "p:Struct0::m");
            auto Protocol0 = ymCtx_Load(ctx, "p:Protocol0");
            auto Fn0 = ymCtx_Load(ctx, "p:Fn0");
            ASSERT_TRUE(Struct0_m);
            ASSERT_TRUE(Protocol0);
            ASSERT_TRUE(Fn0);

            static_assert(YmKind_Num == 4);

            // Protocols
            EXPECT_EQ(ymCtx_PutDefault(ctx, YM_NEWTOP, Protocol0), YM_FALSE);
            EXPECT_GE(getErr()[YmErrCode_NoDefaultValue], 1);
        });
}

TEST(Contexts, Call_WithReturnValuePuttingPushingAndDiscard) {
    auto setupfn =
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
        ymParcelDef_AddFn(
            parceldef,
            "g",
            "yama:Int",
            [](YmCtx* ctx, void* user) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            // test w/ regular index

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            // test w/ newtop

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_NEWTOP), YM_TRUE);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 1);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 1);
        });
    objsys_test(
        setupfn,
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            // test w/ discard

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
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
    auto setup = [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
        ymParcelDef_AddFn(parceldef, "g", "yama:Int",
            [](YmCtx* ctx, void*) {
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
            [](YmCtx* ctx, void*) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            auto A_m = ymCtx_Load(ctx, "p:A::m");
            ASSERT_TRUE(g);
            ASSERT_TRUE(A_m);

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
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                EXPECT_EQ(ymCtx_Call(ctx, g, 1, "", YM_NEWTOP), YM_TRUE);
                });
            test(50, false, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 1, "", YM_NEWTOP), YM_TRUE);
                });

            // 1 named, w/ each

            test(53, true, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "plus", YM_NEWTOP), YM_TRUE);
                });
            test(53, true, false, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "plus", YM_NEWTOP), YM_TRUE);
                });

            test(-50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "negate", YM_NEWTOP), YM_TRUE);
                });
            test(50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "negate", YM_NEWTOP), YM_TRUE);
                });
            test(-50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "negate", YM_NEWTOP), YM_TRUE);
                });
            test(50, false, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "negate", YM_NEWTOP), YM_TRUE);
                });

            test(500, false, false, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 2, "timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(500, false, false, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 2, "timesTen", YM_NEWTOP), YM_TRUE);
                });

            // 2 named, w/ reversed

            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "negate,plus", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, false, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "negate,plus", YM_NEWTOP), YM_TRUE);
                });

            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 3, "timesTen,negate", YM_NEWTOP), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-500, false, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 3, "timesTen,negate", YM_NEWTOP), YM_TRUE);
                });

            // 3 named, w/ diff orders

            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, g, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });

            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "plus,negate,timesTen", YM_NEWTOP), YM_TRUE);
                });
            test(-530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(530, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });
            test(-53, true, true, true, [=]() {
                ymCtx_PutInt(ctx, YM_NEWTOP, 50);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_FALSE);
                ymCtx_PutInt(ctx, YM_NEWTOP, 3);
                ymCtx_PutBool(ctx, YM_NEWTOP, YM_TRUE);
                EXPECT_EQ(ymCtx_Call(ctx, A_m, 4, "timesTen,plus,negate", YM_NEWTOP), YM_TRUE);
                });
        });
}

TEST(Contexts, Call_WithDiffKindsOfCallableTypes) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            EXPECT_NE(ymParcelDef_AddFn(parceldef, "g", "yama:Int",
                [](YmCtx* ctx, void*) {
                    ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 21), YM_TAKE);
                    observedCalls++;
                },
                nullptr),
                YM_NO_TYPE_INDEX);
            auto A_ind = ymParcelDef_AddStruct(parceldef, "A");
            EXPECT_NE(A_ind, YM_NO_TYPE_INDEX);
            EXPECT_NE(ymParcelDef_AddMethod(parceldef, "A", "m", "yama:Int",
                [](YmCtx* ctx, void*) {
                    ymCtx_Ret(ctx, ymCtx_NewInt(ctx, 13), YM_TAKE);
                    observedCalls++;
                },
                nullptr),
                YM_NO_TYPE_INDEX);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            auto A = ymCtx_Load(ctx, "p:A");
            auto A_m = ymCtx_Load(ctx, "p:A::m");
            ASSERT_TRUE(g);
            ASSERT_TRUE(A);
            ASSERT_TRUE(A_m);

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_NEWTOP), YM_TRUE);
            EXPECT_EQ(ymCtx_Call(ctx, A_m, 0, "", YM_NEWTOP), YM_TRUE);

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
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:None",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
                    auto g = ymCtx_Load(ctx, "p:g");
                    ASSERT_TRUE(g);
                    auto n_arg = ymCtx_Arg(ctx, 0, YM_BORROW);
                    ASSERT_TRUE(n_arg);
                    auto n = ymObj_ToUInt(n_arg, nullptr);
                    if (n > 1) {
                        ASSERT_EQ(ymCtx_PutUInt(ctx, YM_NEWTOP, n - 1), YM_TRUE);
                        ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_DISCARD), YM_TRUE);
                    }
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "n", "yama:UInt");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);
            ASSERT_EQ(ymCtx_PutUInt(ctx, YM_NEWTOP, 10), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_DISCARD), YM_TRUE);
            EXPECT_EQ(observedCalls, 10);
        });
}

TEST(Contexts, Call_Fail_LocalNotFound_ReturnToIsOutOfBounds) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", 10'000), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_NonCallableType_FnIsNonCallable) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, ymCtx_LdInt(ctx), 2, "", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_NonCallableType], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_LocalNotFound_ArgsExceedsLocalObjectStackHeight) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_LocalNotFound], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooManyPositionalArgs) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 3, "", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), x);
            EXPECT_EQ(ymObj_RefCount(x), 3);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooFewPositionalArgs) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 1, "", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 1);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_TooFewPositionalArgs_DueToNamedArgs) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(a, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -3));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, a, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 3, "a,b", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 3);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), a);
            EXPECT_EQ(ymCtx_Local(ctx, 2, YM_BORROW), b);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(a), 2);
            EXPECT_EQ(ymObj_RefCount(b), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_ArgNameIdentifierMultipleTimes) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "b,b", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

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

TEST(Contexts, Call_Fail_CallProcedureError_ArgNameUnknownIdentifier) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "missing,b", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

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

TEST(Contexts, Call_Fail_CallProcedureError_ArgNamePositionalParamIdentifier) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -3));
            SETUP_OBJ(b, ymCtx_NewInt(ctx, -13));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, b, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 4, "x,b", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

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

TEST(Contexts, Call_Fail_CallProcedureError_ArgsAreWrongTypes) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

            ASSERT_EQ(ymCtx_Locals(ctx), 2);
            EXPECT_EQ(ymCtx_Local(ctx, 0, YM_BORROW), x);
            EXPECT_EQ(ymCtx_Local(ctx, 1, YM_BORROW), y);
            EXPECT_EQ(ymObj_RefCount(x), 2);
            EXPECT_EQ(ymObj_RefCount(y), 2);
            EXPECT_EQ(observedCalls, 0);
        });
}

TEST(Contexts, Call_Fail_CallProcedureError_ArgsAreWrongTypes_DueToNamedArgs) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_BeginNamedParams(parceldef, "g");
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewRune(ctx, U'y'));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "x,y", YM_NEWTOP), YM_FALSE);
            EXPECT_EQ(getErr()[YmErrCode_CallProcedureError], 1);

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
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    //ymCtx_Ret(ctx, ymCtx_Arg(ctx, 1, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_NEWTOP), YM_FALSE);
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
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    // Arg #1 is the Float, not the Int.
                    ymCtx_Ret(ctx, ymCtx_Arg(ctx, 0, YM_BORROW), YM_BORROW);
                },
                nullptr);
            ymParcelDef_AddParam(parceldef, "g", "x", "yama:Float");
            ymParcelDef_AddParam(parceldef, "g", "y", "yama:Int");
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            SETUP_OBJ(x, ymCtx_NewFloat(ctx, 3.14159));
            SETUP_OBJ(y, ymCtx_NewInt(ctx, -50));

            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, x, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Put(ctx, YM_NEWTOP, y, YM_BORROW), YM_TRUE);
            ASSERT_EQ(ymCtx_Call(ctx, g, 2, "", YM_NEWTOP), YM_FALSE);
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
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:None",
                [](YmCtx* ctx, void* user) {
                    observedCalls++;
                    ymCtx_Ret(ctx, ymCtx_NewNone(ctx), YM_TAKE);
                    auto g = ymCtx_Load(ctx, "p:g");
                    ASSERT_TRUE(g);
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);
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
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void*) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_Take) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void*) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Ret_Overwrite) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void*) {
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
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

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

            ymCtx_Ret(ctx, a, YM_TAKE);
            EXPECT_EQ(ymObj_RefCount(a), 1);
        });
}

TEST(Contexts, Ret_FailQuietly_WhatIsNullptr) {
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void*) {
                    ymCtx_Ret(ctx, YM_NIL, YM_BORROW);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_FALSE);
        });
    objsys_test(
        [](YmParcelDef* parceldef, YmTypeIndex f_ind) {
            ymParcelDef_AddFn(
                parceldef,
                "g",
                "yama:Int",
                [](YmCtx* ctx, void*) {
                    SETUP_OBJ(x, ymCtx_NewInt(ctx, 100));

                    EXPECT_EQ(ymObj_RefCount(x), 1);
                    ymCtx_Ret(ctx, x, YM_BORROW);
                    EXPECT_EQ(ymObj_RefCount(x), 2);

                    // Test that impl can handle w/ already existing binding.
                    ymCtx_Ret(ctx, YM_NIL, YM_BORROW);
                    ymCtx_Ret(ctx, YM_NIL, YM_TAKE);

                    EXPECT_EQ(ymObj_RefCount(x), 2);
                },
                nullptr);
        },
        [](YmCtx* ctx, bool called_in_fn_body) {
            if (!called_in_fn_body) {
                return;
            }
            auto g = ymCtx_Load(ctx, "p:g");
            ASSERT_TRUE(g);

            EXPECT_EQ(ymCtx_Call(ctx, g, 0, "", YM_DISCARD), YM_TRUE);
        });
}

TEST(Contexts, Convert) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(UInt);

            // Test w/ regular indices.

            ASSERT_EQ(ymCtx_PutNone(ctx, YM_NEWTOP), YM_TRUE); // Result goes here.
            ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 13), YM_TRUE);
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

            // Test w/ newtop.

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, UInt, YM_NEWTOP), YM_TRUE);

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

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 13), YM_TRUE);
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

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 13), YM_TRUE);
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
}

TEST(Contexts, Convert_Fail_LocalNotFound_LocalObjectStackIsEmpty) {
    objsys_test(
        [](YmCtx* ctx, bool called_in_fn_body) {
            auto Int = ymCtx_LdInt(ctx);
            auto UInt = ymCtx_LdUInt(ctx);
            ASSERT_TRUE(Int);
            ASSERT_TRUE(UInt);

            ASSERT_EQ(ymCtx_Convert(ctx, UInt, YM_NEWTOP), YM_FALSE);
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

            ASSERT_EQ(ymCtx_PutInt(ctx, YM_NEWTOP, 13), YM_TRUE);
            ASSERT_EQ(ymCtx_Convert(ctx, Type, YM_NEWTOP), YM_FALSE);
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

