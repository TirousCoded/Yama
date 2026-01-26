

#pragma once


#include <ranges>

#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>
#include <yama++/Safe.h>

#include "../../utils/utils.h"


inline YmItemIndex setup_helper(
    YmParcelDef* def,
    std::function<YmItemIndex()> builder,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> itemParams) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_ITEM_INDEX;
    }
    YmItemIndex result = builder();
    for (const auto& refconst : refs) {
        ymParcelDef_AddRef(def, result, refconst.c_str());
    }
    for (const auto& [name, constraint] : itemParams) {
        ymParcelDef_AddItemParam(def, result, name.c_str(), constraint.c_str());
    }
    if (result == YM_NO_ITEM_INDEX) {
        ADD_FAILURE();
    }
    return result;
}
inline YmItemIndex setup_struct(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> itemParams = {}) {
    return setup_helper(
        def,
        [def, localName]() -> YmItemIndex {
            return ymParcelDef_AddStruct(def, localName.c_str());
        },
        refs,
        itemParams);
}
inline YmItemIndex setup_protocol(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> itemParams = {}) {
    return setup_helper(
        def,
        [def, localName]() -> YmItemIndex {
            return ymParcelDef_AddProtocol(def, localName.c_str());
        },
        refs,
        itemParams);
}
inline YmItemIndex setup_fn(
    YmParcelDef* def,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refs,
    std::initializer_list<std::pair<std::string, std::string>> itemParams = {}) {
    return setup_helper(
        def,
        [def, localName, returnTypeSymbol]() -> YmItemIndex {
            return ymParcelDef_AddFn(def, localName.c_str(), returnTypeSymbol.c_str(), ymInertCallBhvrFn, nullptr);
        },
        refs,
        itemParams);
}
inline YmItemIndex setup_method(
    YmParcelDef* def,
    YmItemIndex owner,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refs) {
    return setup_helper(
        def,
        [def, owner, localName, returnTypeSymbol]() -> YmItemIndex {
            return ymParcelDef_AddMethod(def, owner, localName.c_str(), returnTypeSymbol.c_str(), ymInertCallBhvrFn, nullptr);
        },
        refs,
        {});
}

inline void test_item_basics(
    YmItem* item,
    const std::string& fullname,
    YmKind kind,
    std::initializer_list<YmItem*> refs,
    std::initializer_list<YmItem*> itemArgs) {
    ASSERT_NE(item, nullptr); // For safety.
    EXPECT_STREQ(ymItem_Fullname(item), fullname.c_str());
    EXPECT_EQ(ymItem_Kind(item), kind);
    for (YmItem* ref : refs) {
        EXPECT_NE(ymItem_FindRef(item, ref), YM_NO_REF)
            << "ymItem_Fullname(ref)==" << ymItem_Fullname(ref);
    }
    ASSERT_EQ(itemArgs.size(), ymItem_ItemParams(item));
    size_t index = 0;
    for (YmItem* itemArg : itemArgs) {
        EXPECT_EQ(ymItem_ItemParamByIndex(item, (YmItemParamIndex)index), itemArg)
            << "ymItem_Fullname(itemArg)==" << ymItem_Fullname(itemArg);
        index++;
    }
}
inline void test_struct(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refs,
    std::initializer_list<YmItem*> itemArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Struct, refs, itemArgs);
}
inline void test_protocol(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refs,
    std::initializer_list<YmItem*> itemArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Protocol, refs, itemArgs);
}
inline void test_fn(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refs,
    std::initializer_list<YmItem*> itemArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Fn, refs, itemArgs);
}
inline void test_method(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refs,
    std::initializer_list<YmItem*> itemArgs = {}) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Method, refs, itemArgs);
}

