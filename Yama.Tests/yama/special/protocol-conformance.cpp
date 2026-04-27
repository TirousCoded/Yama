

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
    ymParcelDef_AddParam(p_def, "FN2", "param", "p:STRUCT1");

    auto METHOD1_index = ymParcelDef_AddMethod(p_def, "STRUCT1", "METHOD1", "p:STRUCT1", ymInertCallBhvrFn, nullptr);
    auto METHOD2_index = ymParcelDef_AddMethod(p_def, "STRUCT1", "METHOD2", "p:STRUCT2", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "METHOD2", "param", "p:STRUCT1");

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
    failure(FN1, Any);
    failure(FN2, Any);
    failure(METHOD1, Any);
    failure(METHOD2, Any);
}

TEST(ProtocolConformance, NonEmptyProtocol) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");
    ymParcelDef_AddStruct(p_def, "Int");

    auto Bob_index = ymParcelDef_AddStruct(p_def, "Bob");
    ymParcelDef_AddMethod(p_def, "Bob", "age", "p:Int", ymInertCallBhvrFn, nullptr);
    auto Bob_buyDrinks_index = ymParcelDef_AddMethod(p_def, "Bob", "buyDrinks", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "Bob::buyDrinks", "drinks", "p:Int");
    
    auto Sally_index = ymParcelDef_AddStruct(p_def, "Sally");
    ymParcelDef_AddMethod(p_def, "Sally", "age", "p:Int", ymInertCallBhvrFn, nullptr);
    
    auto George_index = ymParcelDef_AddStruct(p_def, "George");
    ymParcelDef_AddMethod(p_def, "George", "age", "p:Int", ymInertCallBhvrFn, nullptr);
    auto George_buyDrinks_index = ymParcelDef_AddMethod(p_def, "George", "buyDrinks", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "George::buyDrinks", "drinks", "p:Int");

    auto Person_index = ymParcelDef_AddProtocol(p_def, "Person");
    ymParcelDef_AddMethodReq(p_def, "Person", "age", "p:Int");

    auto CanDrink_index = ymParcelDef_AddProtocol(p_def, "CanDrink");
    auto CanDrink_buyDrinks_index = ymParcelDef_AddMethodReq(p_def, "CanDrink", "buyDrinks", "p:None");
    ymParcelDef_AddParam(p_def, "CanDrink::buyDrinks", "drinks", "p:Int");

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
    auto METHOD_index = ymParcelDef_AddMethod(p_def, "STRUCT", "METHOD", "p:STRUCT", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P"); // The above should all fail to conform to P.
    ymParcelDef_AddMethodReq(p_def, "P", "m", "p:None");

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
    ymParcelDef_AddMethod(p_def, "A", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "A", "bar", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "A", "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "A", "extra", "p:None", ymInertCallBhvrFn, nullptr);

    // Method order shouldn't matter.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddMethod(p_def, "B", "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "B", "extra", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "B", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "B", "bar", "p:None", ymInertCallBhvrFn, nullptr);
    
    // Missing p:C::bar.
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddMethod(p_def, "C", "baz", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "C", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    //ymParcelDef_AddMethod(p_def, "C", "bar", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddMethod(p_def, "C", "extra", "p:None", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, "P", "foo", "p:None");
    ymParcelDef_AddMethodReq(p_def, "P", "bar", "p:None");
    ymParcelDef_AddMethodReq(p_def, "P", "baz", "p:None");

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
    ymParcelDef_AddMethod(p_def, "A", "foo", "p:None", ymInertCallBhvrFn, nullptr);

    // Wrong return type.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddMethod(p_def, "B", "foo", "p:Int", ymInertCallBhvrFn, nullptr);

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, "P", "foo", "p:None");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto P = load(ctx, "p:P");

    success(A, P);
    failure(B, P);
}

TEST(ProtocolConformance, MethodsMustHaveCorrectPositionalParams_AndConformingMethodsMayHaveArbitraryNamedParams) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    ymParcelDef_AddStruct(p_def, "None");
    ymParcelDef_AddStruct(p_def, "Int");
    ymParcelDef_AddStruct(p_def, "Float");

    ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddMethod(p_def, "A", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "A::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "A::foo", "b", "p:None");
    ymParcelDef_AddParam(p_def, "A::foo", "c", "p:Float");

    // Param names shouldn't matter.
    ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddMethod(p_def, "B", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "B::foo", "x", "p:Int");
    ymParcelDef_AddParam(p_def, "B::foo", "y", "p:None");
    ymParcelDef_AddParam(p_def, "B::foo", "z", "p:Float");

    // May have arbitrary named parameters.
    ymParcelDef_AddStruct(p_def, "C");
    ymParcelDef_AddMethod(p_def, "C", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "C::foo", "x", "p:Int");
    ymParcelDef_AddParam(p_def, "C::foo", "y", "p:None");
    ymParcelDef_AddParam(p_def, "C::foo", "z", "p:Float");
    ymParcelDef_BeginNamedParams(p_def, "C::foo");
    ymParcelDef_AddParam(p_def, "C::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "C::foo", "b", "p:None");
    ymParcelDef_AddParam(p_def, "C::foo", "c", "p:Float");

    // Too many params.
    ymParcelDef_AddStruct(p_def, "D");
    ymParcelDef_AddMethod(p_def, "D", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "D::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "D::foo", "b", "p:None");
    ymParcelDef_AddParam(p_def, "D::foo", "c", "p:Float");
    ymParcelDef_AddParam(p_def, "D::foo", "d", "p:None");

    // Too few params.
    ymParcelDef_AddStruct(p_def, "E");
    ymParcelDef_AddMethod(p_def, "E", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "E::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "E::foo", "b", "p:None");

    // Wrong param type.
    ymParcelDef_AddStruct(p_def, "F");
    ymParcelDef_AddMethod(p_def, "F", "foo", "p:None", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "F::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "F::foo", "b", "p:Int");
    ymParcelDef_AddParam(p_def, "F::foo", "c", "p:Float");

    ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddMethodReq(p_def, "P", "foo", "p:None");
    ymParcelDef_AddParam(p_def, "P::foo", "a", "p:Int");
    ymParcelDef_AddParam(p_def, "P::foo", "b", "p:None");
    ymParcelDef_AddParam(p_def, "P::foo", "c", "p:Float");

    ymDm_BindParcelDef(dm, "p", p_def);
    auto A = load(ctx, "p:A");
    auto B = load(ctx, "p:B");
    auto C = load(ctx, "p:C");
    auto D = load(ctx, "p:D");
    auto E = load(ctx, "p:E");
    auto F = load(ctx, "p:F");
    auto P = load(ctx, "p:P");

    success(A, P);
    success(B, P);
    success(C, P);
    failure(D, P);
    failure(E, P);
    failure(F, P);
}

TEST(ProtocolConformance, Self) {
    SETUP_ALL(ctx);
    SETUP_PARCELDEF(p_def);

    // Protocol's '$Self' will match 'p:A' here.
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_foo_index = ymParcelDef_AddMethod(p_def, "A", "foo", "p:A", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "A::foo", "a", "p:A");

    // Protocol's '$Self' will match 'p:B' here.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_foo_index = ymParcelDef_AddMethod(p_def, "B", "foo", "p:B", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "B::foo", "a", "p:B");

    // Protocol method '$Self' doesn't match protocol type, but instead the type
    // being checked for conformance (ie. 'p:C'.)
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_foo_index = ymParcelDef_AddMethod(p_def, "C", "foo", "p:P", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "C::foo", "a", "p:P");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_foo_index = ymParcelDef_AddMethodReq(p_def, "P", "foo", "$Self");
    ymParcelDef_AddParam(p_def, "P::foo", "a", "$Self");

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
    ymParcelDef_AddTypeParam(p_def, "G", "T", "p:Any");
    auto H_index = ymParcelDef_AddStruct(p_def, "H");
    ymParcelDef_AddTypeParam(p_def, "H", "T", "p:Any");

    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, "P", "m", "p:G[p:G[p:X]]");
    ymParcelDef_AddParam(p_def, "P::m", "a", "p:G[p:G[p:Y]]");
    
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, "A", "m", "p:G[p:G[p:X]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "A::m", "a", "p:G[p:G[p:Y]]");
    
    // Nested too deep.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_m_index = ymParcelDef_AddMethod(p_def, "B", "m", "p:G[p:G[p:G[p:X]]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "B::m", "a", "p:G[p:G[p:G[p:Y]]]");

    // Nested not deep enough.
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_m_index = ymParcelDef_AddMethod(p_def, "C", "m", "p:G[p:X]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "C::m", "a", "p:G[p:Y]");

    // Incorrect generic type doing the nesting.
    auto D_index = ymParcelDef_AddStruct(p_def, "D");
    auto D_m_index = ymParcelDef_AddMethod(p_def, "D", "m", "p:G[p:H[p:X]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "D::m", "a", "p:H[p:G[p:Y]]");

    // Wrong inner-most nested type.
    auto E_index = ymParcelDef_AddStruct(p_def, "E");
    auto E_m_index = ymParcelDef_AddMethod(p_def, "E", "m", "p:G[p:G[p:Y]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "E::m", "a", "p:G[p:G[p:X]]");

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
    ymParcelDef_AddTypeParam(p_def, "G", "T", "p:Any");
    auto H_index = ymParcelDef_AddStruct(p_def, "H");
    ymParcelDef_AddTypeParam(p_def, "H", "T", "p:Any");

    // Generic protocol containing nested $Self and $T.
    auto P_index = ymParcelDef_AddProtocol(p_def, "P");
    ymParcelDef_AddTypeParam(p_def, "P", "T", "p:Any");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, "P", "m", "p:G[p:H[$T]]");
    ymParcelDef_AddParam(p_def, "P::m", "a", "p:H[p:G[$T]]");
    ymParcelDef_AddParam(p_def, "P::m", "b", "p:H[p:G[$Self]]");

    // $Self resolves to p:A (when checking for conformance w/ p:P[p:A].)
    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    auto A_m_index = ymParcelDef_AddMethod(p_def, "A", "m", "p:G[p:H[$Self]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "A::m", "a", "p:H[p:G[$Self]]");
    ymParcelDef_AddParam(p_def, "A::m", "b", "p:H[p:G[$Self]]");

    // p:A specified explicitly.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    auto B_m_index = ymParcelDef_AddMethod(p_def, "B", "m", "p:G[p:H[p:A]]", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "B::m", "a", "p:H[p:G[p:A]]");
    ymParcelDef_AddParam(p_def, "B::m", "b", "p:H[p:G[p:B]]"); // <- This one needs to be p:B though.

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
    ymParcelDef_AddTypeParam(p_def, "P", "T", "p:Any");
    auto P_m_index = ymParcelDef_AddMethodReq(p_def, "P", "m", "$T");
    ymParcelDef_AddParam(p_def, "P::m", "a", "$Self"); // Test generic protocols work w/ $Self.
    ymParcelDef_AddParam(p_def, "P::m", "b", "$T");

    auto A_index = ymParcelDef_AddStruct(p_def, "A");
    ymParcelDef_AddTypeParam(p_def, "A", "T", "p:Any");
    ymParcelDef_AddTypeParam(p_def, "A", "U", "p:Any");
    auto A_m_index = ymParcelDef_AddMethod(p_def, "A", "m", "$U", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "A::m", "a", "$Self"); // Test generic non-protocols work w/ $Self.
    ymParcelDef_AddParam(p_def, "A::m", "b", "$T");

    // p:B[T] should ALWAYS fail to conform w/ p:P[T] due to Self param missing.
    auto B_index = ymParcelDef_AddStruct(p_def, "B");
    ymParcelDef_AddTypeParam(p_def, "B", "T", "p:Any");
    ymParcelDef_AddTypeParam(p_def, "B", "U", "p:Any");
    auto B_m_index = ymParcelDef_AddMethod(p_def, "B", "m", "$U", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "B::m", "b", "$T");
    
    // p:C is non-generic and should conform to p:P[p:X] (ie. but not p:P[p:Y].)
    auto C_index = ymParcelDef_AddStruct(p_def, "C");
    auto C_m_index = ymParcelDef_AddMethod(p_def, "C", "m", "p:X", ymInertCallBhvrFn, nullptr);
    ymParcelDef_AddParam(p_def, "C::m", "a", "p:C");
    ymParcelDef_AddParam(p_def, "C::m", "b", "p:X");

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

