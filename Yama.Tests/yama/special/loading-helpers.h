

#pragma once


#include <ranges>

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>
#include <yama++/Safe.h>

#include "../../utils/utils.h"


inline YmTypeIndex setup_helper(
    YmParcelDef* def,
    std::function<YmTypeIndex()> builder,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> typeParams) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_TYPE_INDEX;
    }
    YmTypeIndex result = builder();
    for (const auto& refconst : refs) {
        ymParcelDef_AddRef(def, result, refconst.c_str());
    }
    for (const auto& [name, constraint] : typeParams) {
        ymParcelDef_AddTypeParam(def, result, name.c_str(), constraint.c_str());
    }
    if (result == YM_NO_TYPE_INDEX) {
        ADD_FAILURE();
    }
    return result;
}
inline YmTypeIndex setup_struct(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> typeParams = {}) {
    return setup_helper(
        def,
        [def, localName]() -> YmTypeIndex {
            return ymParcelDef_AddStruct(def, localName.c_str());
        },
        refs,
        typeParams);
}
inline YmTypeIndex setup_protocol(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> typeParams = {}) {
    return setup_helper(
        def,
        [def, localName]() -> YmTypeIndex {
            return ymParcelDef_AddProtocol(def, localName.c_str());
        },
        refs,
        typeParams);
}
inline YmTypeIndex setup_fn(
    YmParcelDef* def,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> typeParams = {}) {
    return setup_helper(
        def,
        [def, localName, returnTypeSymbol]() -> YmTypeIndex {
            return ymParcelDef_AddFn(def, localName.c_str(), returnTypeSymbol.c_str(), ymInertCallBhvrFn, nullptr);
        },
        refs,
        typeParams);
}
inline YmTypeIndex setup_method(
    YmParcelDef* def,
    YmTypeIndex owner,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refs) {
    return setup_helper(
        def,
        [def, owner, localName, returnTypeSymbol]() -> YmTypeIndex {
            return ymParcelDef_AddMethod(def, owner, localName.c_str(), returnTypeSymbol.c_str(), ymInertCallBhvrFn, nullptr);
        },
        refs,
        {});
}

inline void test_type_basics(
    YmType* type,
    const std::string& fullname,
    YmKind kind,
    std::initializer_list<YmType*> refs,
    std::initializer_list<YmType*> typeArgs) {
    ASSERT_NE(type, nullptr); // For safety.
    EXPECT_STREQ(ymType_Fullname(type), fullname.c_str());
    EXPECT_EQ(ymType_Kind(type), kind);
    for (YmType* ref : refs) {
        EXPECT_NE(ymType_FindRef(type, ref), YM_NO_REF)
            << "ymType_Fullname(ref)==" << ymType_Fullname(ref);
    }
    ASSERT_EQ(typeArgs.size(), ymType_TypeParams(type));
    size_t index = 0;
    for (YmType* typeArg : typeArgs) {
        EXPECT_EQ(ymType_TypeParamByIndex(type, (YmTypeParamIndex)index), typeArg)
            << "ymType_Fullname(typeArg)==" << ymType_Fullname(typeArg);
        index++;
    }
}
inline void test_struct(
    YmType* type,
    const std::string& fullname,
    std::initializer_list<YmType*> refs,
    std::initializer_list<YmType*> typeArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(type, nullptr);
    test_type_basics(type, fullname, YmKind_Struct, refs, typeArgs);
}
inline void test_protocol(
    YmType* type,
    const std::string& fullname,
    std::initializer_list<YmType*> refs,
    std::initializer_list<YmType*> typeArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(type, nullptr);
    test_type_basics(type, fullname, YmKind_Protocol, refs, typeArgs);
}
inline void test_fn(
    YmType* type,
    const std::string& fullname,
    std::initializer_list<YmType*> refs,
    std::initializer_list<YmType*> typeArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(type, nullptr);
    test_type_basics(type, fullname, YmKind_Fn, refs, typeArgs);
}
inline void test_method(
    YmType* type,
    const std::string& fullname,
    std::initializer_list<YmType*> refs,
    std::initializer_list<YmType*> typeArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(type, nullptr);
    test_type_basics(type, fullname, YmKind_Method, refs, typeArgs);
}

