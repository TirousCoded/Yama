

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>

#include "../../utils/utils.h"


// NOTE: These unit tests cover when a non-protocol types conforms to protocol types.


#define success(T, P) EXPECT_TRUE(ymType_Converts((T), (P), false) == YM_TRUE)
#define failure(T, P) EXPECT_TRUE(ymType_Converts((T), (P), false) == YM_FALSE)


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
    auto STRUCT1 = load(ctx, "p:STRUCT1");
    auto STRUCT2 = load(ctx, "p:STRUCT2");
    auto FN1 = load(ctx, "p:FN1");
    auto FN2 = load(ctx, "p:FN2");
    auto METHOD1 = load(ctx, "p:STRUCT1::METHOD1");
    auto METHOD2 = load(ctx, "p:STRUCT1::METHOD2");
    auto Any = load(ctx, "p:Any");

    success(STRUCT1, Any);
    success(STRUCT2, Any);
    success(FN1, Any);
    success(FN2, Any);
    success(METHOD1, Any);
    success(METHOD2, Any);
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
    auto Bob = load(ctx, "p:Bob");
    auto Sally = load(ctx, "p:Sally");
    auto George = load(ctx, "p:George");
    auto Person = load(ctx, "p:Person");
    auto CanDrink = load(ctx, "p:CanDrink");

    success(Bob, Person);
    success(Sally, Person);
    success(George, Person);

    success(Bob, CanDrink);
    failure(Sally, CanDrink);
    success(George, CanDrink);
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
    auto STRUCT = load(ctx, "p:STRUCT");
    auto FN = load(ctx, "p:FN");
    auto METHOD = load(ctx, "p:STRUCT::METHOD");
    auto P = load(ctx, "p:P");

    failure(STRUCT, P);
    failure(FN, P);
    failure(METHOD, P);
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
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto P = load(ctx, "p:P");

    success(A, P);
    success(B, P);
    failure(C, P);
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
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto P = load(ctx, "p:P");

    success(A, P);
    failure(B, P);
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
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto D = load(ctx, "p:D");
    auto E = load(ctx, "p:E");
    auto P = load(ctx, "p:P");

    success(A, P);
    success(B, P);
    failure(C, P);
    failure(D, P);
    failure(E, P);
}

TEST(ProtocolConformance, Self) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    // Protocol's '$Self' will match 'p:A' here.
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_foo_index = ymParcelDef_AddMethod(p_def, A_index, "foo", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_foo_index, "a", "p:A");

    // Protocol's '$Self' will match 'p:B' here.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_foo_index = ymParcelDef_AddMethod(p_def, B_index, "foo", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_foo_index, "a", "p:B");

    // Protocol method '$Self' doesn't match protocol type, but instead the type
    // being checked for conformance (ie. 'p:C'.)
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_foo_index = ymParcelDef_AddMethod(p_def, C_index, "foo", "p:P", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_foo_index, "a", "p:P");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_foo_index = ymParcelDef_AddMethodReq(p_def, P_index, "foo", "$Self");
    ymParcelDef_AddParam(p_def, P_foo_index, "a", "$Self");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto P = load(ctx, "p:P");

    success(A, P);
    success(B, P);
    failure(C, P);
}

TEST(ProtocolConformance, RefSymsContainingGenericTypeNesting) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    auto X_index = ymParcelDef_AddStruct(p_def, "X");
    auto Y_index = ymParcelDef_AddStruct(p_def, "Y");

    auto G_index = ymParcelDef_AddStruct(p_def, "G");
    ymParcelDef_AddTypeParam(p_def, G_index, "T", "p:Any");
    auto H_index = ymParcelDef_AddStruct(p_def, "H");
    ymParcelDef_AddTypeParam(p_def, H_index, "T", "p:Any");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:G[p:G[p:X]]");
    ymParcelDef_AddParam(p_def, P_m_index, "a", "p:G[p:G[p:Y]]");
    
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:G[p:G[p:X]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_m_index, "a", "p:G[p:G[p:Y]]");
    
    // Nested too deep.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_m_index = ymParcelDef_AddMethod(p_def, B_index, "m", "p:G[p:G[p:G[p:X]]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_m_index, "a", "p:G[p:G[p:G[p:Y]]]");

    // Nested not deep enough.
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_m_index = ymParcelDef_AddMethod(p_def, C_index, "m", "p:G[p:X]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_m_index, "a", "p:G[p:Y]");

    // Incorrect generic type doing the nesting.
    auto D_index = ymParcelDef_AddStruct(p_def, "D");
    auto D_m_index = ymParcelDef_AddMethod(p_def, D_index, "m", "p:G[p:H[p:X]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, D_m_index, "a", "p:H[p:G[p:Y]]");

    // Wrong inner-most nested type.
    auto E_index = ymParcelDef_AddStruct(p_def, "E");
    auto E_m_index = ymParcelDef_AddMethod(p_def, E_index, "m", "p:G[p:G[p:Y]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, E_m_index, "a", "p:G[p:G[p:X]]");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto P = load(ctx, "p:P");
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto D = load(ctx, "p:D");
    auto E = load(ctx, "p:E");

    success(A, P);
    failure(B, P);
    failure(C, P);
    failure(D, P);
    failure(E, P);
}

TEST(ProtocolConformance, RefSymsContainingGenericTypeNesting_SelfAndTypeArgs) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    auto G_index = ymParcelDef_AddStruct(p_def, "G");
    ymParcelDef_AddTypeParam(p_def, G_index, "T", "p:Any");
    auto H_index = ymParcelDef_AddStruct(p_def, "H");
    ymParcelDef_AddTypeParam(p_def, H_index, "T", "p:Any");

    // Generic protocol containing nested $Self and $T.
    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddTypeParam(p_def, P_index, "T", "p:Any");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "p:G[p:H[$T]]");
    ymParcelDef_AddParam(p_def, P_m_index, "a", "p:H[p:G[$T]]");
    ymParcelDef_AddParam(p_def, P_m_index, "b", "p:H[p:G[$Self]]");

    // $Self resolves to p:A (when checking for conformance w/ p:P[p:A].)
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "p:G[p:H[$Self]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_m_index, "a", "p:H[p:G[$Self]]");
    ymParcelDef_AddParam(p_def, A_m_index, "b", "p:H[p:G[$Self]]");

    // p:A specified explicitly.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_m_index = ymParcelDef_AddMethod(p_def, B_index, "m", "p:G[p:H[p:A]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_m_index, "a", "p:H[p:G[p:A]]");
    ymParcelDef_AddParam(p_def, B_m_index, "b", "p:H[p:G[p:B]]"); // <- This one needs to be p:B though.

    ymDm_BindParcelDef(dm, "p", p_def);
    auto P_A = load(ctx, "p:P[p:A]");
    auto P_B = load(ctx, "p:P[p:B]");
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");

    success(A, P_A);
    success(B, P_A);

    failure(A, P_B);
    failure(B, P_B);
}

TEST(ProtocolConformance, Generics_BothOfTheProtocolTypeAndOfTheTypesCheckedForConformanceAgainstIt) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddProtocol(p_def, "Any");

    ymParcelDef_AddStruct(p_def, "X");
    ymParcelDef_AddStruct(p_def, "Y");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddTypeParam(p_def, P_index, "T", "p:Any");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, P_index, "m", "$T");
    ymParcelDef_AddParam(p_def, P_m_index, "a", "$Self"); // Test generic protocols work w/ $Self.
    ymParcelDef_AddParam(p_def, P_m_index, "b", "$T");

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddTypeParam(p_def, A_index, "T", "p:Any");
    ymParcelDef_AddTypeParam(p_def, A_index, "U", "p:Any");
    auto A_m_index = ymParcelDef_AddMethod(p_def, A_index, "m", "$U", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, A_m_index, "a", "$Self"); // Test generic non-protocols work w/ $Self.
    ymParcelDef_AddParam(p_def, A_m_index, "b", "$T");

    // p:B[T] should ALWAYS fail to conform w/ p:P[T] due to Self param missing.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddTypeParam(p_def, B_index, "T", "p:Any");
    ymParcelDef_AddTypeParam(p_def, B_index, "U", "p:Any");
    auto B_m_index = ymParcelDef_AddMethod(p_def, B_index, "m", "$U", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, B_m_index, "b", "$T");
    
    // p:C is non-generic and should conform to p:P[p:X] (ie. but not p:P[p:Y].)
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_m_index = ymParcelDef_AddMethod(p_def, C_index, "m", "p:X", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, C_m_index, "a", "p:C");
    ymParcelDef_AddParam(p_def, C_m_index, "b", "p:X");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto Any = load(ctx, "p:Any");
    auto P_X = load(ctx, "p:P[p:X]");
    auto P_Y = load(ctx, "p:P[p:Y]");
    auto P_A_X_X = load(ctx, "p:P[p:A[p:X, p:X]]");
    auto A_X_X = load(ctx, "p:A[p:X, p:X]");
    auto A_Y_Y = load(ctx, "p:A[p:Y, p:Y]");
    auto A_X_Y = load(ctx, "p:A[p:X, p:Y]");
    auto A_A_X_X_A_X_X = load(ctx, "p:A[p:A[p:X, p:X], p:A[p:X, p:X]]");
    auto B_X_X = load(ctx, "p:B[p:X, p:X]");
    auto B_Y_Y = load(ctx, "p:B[p:Y, p:Y]");
    auto B_X_Y = load(ctx, "p:B[p:X, p:Y]");
    auto C = load(ctx, "p:C");

    success(A_X_X, Any);
    success(A_Y_Y, Any);
    success(A_X_Y, Any);
    success(A_A_X_X_A_X_X, Any);
    success(B_X_X, Any);
    success(B_Y_Y, Any);
    success(B_X_Y, Any);
    success(C, Any);

    success(A_X_X, P_X);
    failure(A_Y_Y, P_X);
    failure(A_X_Y, P_X);
    failure(A_A_X_X_A_X_X, P_X);
    failure(B_X_X, P_X);
    failure(B_Y_Y, P_X);
    failure(B_X_Y, P_X);
    success(C, P_X);

    failure(A_X_X, P_Y);
    success(A_Y_Y, P_Y);
    failure(A_X_Y, P_Y);
    failure(A_A_X_X_A_X_X, P_Y);
    failure(B_X_X, P_Y);
    failure(B_Y_Y, P_Y);
    failure(B_X_Y, P_Y);
    failure(C, P_Y);

    failure(A_X_X, P_A_X_X);
    failure(A_Y_Y, P_A_X_X);
    failure(A_X_Y, P_A_X_X);
    success(A_A_X_X_A_X_X, P_A_X_X);
    failure(B_X_X, P_A_X_X);
    failure(B_Y_Y, P_A_X_X);
    failure(B_X_Y, P_A_X_X);
    failure(C, P_A_X_X);
}

