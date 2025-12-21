

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../../utils/utils.h"


// NOTE: These unit tests cover when a non-protocol types conforms to protocol types.


#define succeed(T, P) EXPECT_TRUE(ymItem_Converts((T), (P), false) == YM_TRUE)
#define fail(T, P) EXPECT_TRUE(ymItem_Converts((T), (P), false) == YM_FALSE)


TEST(ProtocolConformance, EmptyProtocol_IsATopType) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    auto STRUCT1_index = ymParcelDef_AddStruct(p_def, "STRUCT1");
    auto STRUCT2_index = ymParcelDef_AddStruct(p_def, "STRUCT2");
    
    auto FN1_index = ymParcelDef_AddFn(p_def, "FN1", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto FN2_index = ymParcelDef_AddFn(p_def, "FN2", "p:STRUCT2", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, FN2_index, "param", "p:STRUCT1");

    auto METHOD1_index = ymParcelDef_AddMethod(p_def, STRUCT1_index, "METHOD1", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto METHOD2_index = ymParcelDef_AddMethod(p_def, STRUCT1_index, "METHOD2", "p:STRUCT2", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, METHOD2_index, "param", "p:STRUCT1");

    ymParcelDef_AddProtocol(p_def, "Any"); // Our empty protocol top type.
    
    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT1 = ymCtx_Load(ctx, "p:STRUCT1");
    auto STRUCT2 = ymCtx_Load(ctx, "p:STRUCT2");
    auto FN1 = ymCtx_Load(ctx, "p:FN1");
    auto FN2 = ymCtx_Load(ctx, "p:FN2");
    auto METHOD1 = ymCtx_Load(ctx, "p:STRUCT1::METHOD1");
    auto METHOD2 = ymCtx_Load(ctx, "p:STRUCT1::METHOD2");
    auto Any = ymCtx_Load(ctx, "p:Any");
    ASSERT_TRUE(STRUCT1);
    ASSERT_TRUE(STRUCT2);
    ASSERT_TRUE(FN1);
    ASSERT_TRUE(FN2);
    ASSERT_TRUE(METHOD1);
    ASSERT_TRUE(METHOD2);
    ASSERT_TRUE(Any);

    succeed(STRUCT1, Any);
    succeed(STRUCT2, Any);
    succeed(FN1, Any);
    succeed(FN2, Any);
    succeed(METHOD1, Any);
    succeed(METHOD2, Any);
}

TEST(ProtocolConformance, NonEmptyProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");
    ymParcelDef_AddStruct(p_def, "Int");

    auto Bob_index = ymParcelDef_AddStruct(p_def, "Bob");
    ymParcelDef_AddMethod(p_def, Bob_index, "age", "p:Int", ymInertCallBhvrFn, nullptr);
    auto Bob_buyDrinks_index = ymParcelDef_AddMethod(p_def, Bob_index, "buyDrinks", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, Bob_buyDrinks_index, "drinks", "p:Int");
    
    auto Sally_index = ymParcelDef_AddStruct(p_def, "Sally");
    ymParcelDef_AddMethod(p_def, Sally_index, "age", "p:Int", ymInertCallBhvrFn, nullptr);
    
    auto George_index = ymParcelDef_AddStruct(p_def, "George");
    ymParcelDef_AddMethod(p_def, George_index, "age", "p:Int", ymInertCallBhvrFn, nullptr);
    auto George_buyDrinks_index = ymParcelDef_AddMethod(p_def, George_index, "buyDrinks", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, George_buyDrinks_index, "drinks", "p:Int");

    auto Person_index = ymParcelDef_AddProtocol(p_def, "Person");
    ymParcelDef_AddMethodReq(p_def, Person_index, "age", "p:Int");

    auto CanDrink_index = ymParcelDef_AddProtocol(p_def, "CanDrink");
    auto CanDrink_buyDrinks_index = ymParcelDef_AddMethodReq(p_def, CanDrink_index, "buyDrinks", "p:None");
    ymParcelDef_AddParam(p_def, CanDrink_buyDrinks_index, "drinks", "p:Int");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto Bob = ymCtx_Load(ctx, "p:Bob");
    auto Sally = ymCtx_Load(ctx, "p:Sally");
    auto George = ymCtx_Load(ctx, "p:George");
    auto Person = ymCtx_Load(ctx, "p:Person");
    auto CanDrink = ymCtx_Load(ctx, "p:CanDrink");
    ASSERT_TRUE(Bob);
    ASSERT_TRUE(Sally);
    ASSERT_TRUE(George);
    ASSERT_TRUE(Person);
    ASSERT_TRUE(CanDrink);

    succeed(Bob, Person);
    succeed(Sally, Person);
    succeed(George, Person);

    succeed(Bob, CanDrink);
    fail(Sally, CanDrink);
    succeed(George, CanDrink);
}

// NOTE: This test is here just to sanity check that failure is *generally* detected
//       correctly for ALL kinds.

TEST(ProtocolConformance, AllKindsGenerallyFailAsExpected) {
    static_assert(YmKind_Num == 4);
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");

    auto STRUCT_index = ymParcelDef_AddStruct(p_def, "STRUCT");
    auto FN_index = ymParcelDef_AddFn(p_def, "FN", "p:STRUCT", ymInertCallBhvrFn, nullptr);
    auto METHOD_index = ymParcelDef_AddMethod(p_def, STRUCT_index, "METHOD", "p:STRUCT", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P"); // The above should all fail to conform to P.
    ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:None");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto STRUCT = ymCtx_Load(ctx, "p:STRUCT");
    auto FN = ymCtx_Load(ctx, "p:FN");
    auto METHOD = ymCtx_Load(ctx, "p:STRUCT::METHOD");
    auto P = ymCtx_Load(ctx, "p:P");
    ASSERT_TRUE(STRUCT);
    ASSERT_TRUE(FN);
    ASSERT_TRUE(METHOD);
    ASSERT_TRUE(P);

    fail(STRUCT, P);
    fail(FN, P);
    fail(METHOD, P);
}

TEST(ProtocolConformance, MustHaveAllMethodsNamed) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, A_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_index, "bar", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_index, "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, A_index, "extra", "p:None", ymInertCallBhvrFn, nullptr);

    // Method order shouldn't matter.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddMethod(p_def, B_index, "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, B_index, "extra", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, B_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, B_index, "bar", "p:None", ymInertCallBhvrFn, nullptr);
    
    // Missing p:C::bar.
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddMethod(p_def, C_index, "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, C_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    //ymParcelDef_AddMethod(p_def, C_index, "bar", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, C_index, "extra", "p:None", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, P_index, "foo", "p:None");
    ymParcelDef_AddMethodReq(p_def, P_index, "bar", "p:None");
    ymParcelDef_AddMethodReq(p_def, P_index, "baz", "p:None");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto P = ymCtx_Load(ctx, "p:P");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(P);

    succeed(A, P);
    succeed(B, P);
    fail(C, P);
}

TEST(ProtocolConformance, MethodsMustHaveCorrectReturnTypes) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");
    ymParcelDef_AddStruct(p_def, "Int");

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, A_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);

    // Wrong return type.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddMethod(p_def, B_index, "foo", "p:Int", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, P_index, "foo", "p:None");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto P = ymCtx_Load(ctx, "p:P");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(P);

    succeed(A, P);
    fail(B, P);
}

TEST(ProtocolConformance, MethodsMustHaveCorrectParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_foo_index = ymParcelDef_AddMethod(p_def, A_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_foo_index, "a", "p:Int");
    ymParcelDef_AddParam(p_def, A_foo_index, "b", "p:None");
    ymParcelDef_AddParam(p_def, A_foo_index, "c", "p:Float");

    // Param names shouldn't matter.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_foo_index = ymParcelDef_AddMethod(p_def, B_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_foo_index, "x", "p:Int");
    ymParcelDef_AddParam(p_def, B_foo_index, "y", "p:None");
    ymParcelDef_AddParam(p_def, B_foo_index, "z", "p:Float");

    // Too many params.
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_foo_index = ymParcelDef_AddMethod(p_def, C_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_foo_index, "a", "p:Int");
    ymParcelDef_AddParam(p_def, C_foo_index, "b", "p:None");
    ymParcelDef_AddParam(p_def, C_foo_index, "c", "p:Float");
    ymParcelDef_AddParam(p_def, C_foo_index, "d", "p:None");

    // Too few params.
    auto D_index = ymParcelDef_AddStruct(p_def, "D");
    auto D_foo_index = ymParcelDef_AddMethod(p_def, D_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, D_foo_index, "a", "p:Int");
    ymParcelDef_AddParam(p_def, D_foo_index, "b", "p:None");

    // Wrong param type.
    auto E_index = ymParcelDef_AddStruct(p_def, "E");
    auto E_foo_index = ymParcelDef_AddMethod(p_def, E_index, "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, E_foo_index, "a", "p:Int");
    ymParcelDef_AddParam(p_def, E_foo_index, "b", "p:Int");
    ymParcelDef_AddParam(p_def, E_foo_index, "c", "p:Float");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_foo_index = ymParcelDef_AddMethodReq(p_def, P_index, "foo", "p:None");
    ymParcelDef_AddParam(p_def, P_foo_index, "a", "p:Int");
    ymParcelDef_AddParam(p_def, P_foo_index, "b", "p:None");
    ymParcelDef_AddParam(p_def, P_foo_index, "c", "p:Float");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto D = ymCtx_Load(ctx, "p:D");
    auto E = ymCtx_Load(ctx, "p:E");
    auto P = ymCtx_Load(ctx, "p:P");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(D);
    ASSERT_TRUE(E);
    ASSERT_TRUE(P);

    succeed(A, P);
    succeed(B, P);
    fail(C, P);
    fail(D, P);
    fail(E, P);
}

TEST(ProtocolConformance, Self) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    // Protocol's 'Self' will match 'p:A' here.
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_foo_index = ymParcelDef_AddMethod(p_def, A_index, "foo", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_foo_index, "a", "p:A");

    // Protocol's 'Self' will match 'p:B' here.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_foo_index = ymParcelDef_AddMethod(p_def, B_index, "foo", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_foo_index, "a", "p:B");

    // Protocol method 'Self' doesn't match protocol type, but instead the type
    // being checked for conformance (ie. 'p:C'.)
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_foo_index = ymParcelDef_AddMethod(p_def, C_index, "foo", "p:P", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_foo_index, "a", "p:P");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_foo_index = ymParcelDef_AddMethodReq(p_def, P_index, "foo", "Self");
    ymParcelDef_AddParam(p_def, P_foo_index, "a", "Self");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = ymCtx_Load(ctx, "p:A");
    auto B = ymCtx_Load(ctx, "p:B");
    auto C = ymCtx_Load(ctx, "p:C");
    auto P = ymCtx_Load(ctx, "p:P");
    ASSERT_TRUE(A);
    ASSERT_TRUE(B);
    ASSERT_TRUE(C);
    ASSERT_TRUE(P);

    succeed(A, P);
    succeed(B, P);
    fail(C, P);
}

