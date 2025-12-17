

#pragma once


#include <gtest/gtest.h>
#include <taul/strings.h>
#include <yama/yama.h>
#include <yama++/print.h>
#include <yama++/Safe.h>

#include "../../utils/utils.h"


inline YmItemIndex setup_struct(
    YmParcelDef* def,
    const std::string& localName,
    std::initializer_list<std::string> refconsts) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_ITEM_INDEX;
    }
    YmItemIndex result = ymParcelDef_AddStruct(def, localName.c_str());
    for (const auto& refconst : refconsts) {
        ymParcelDef_AddRef(def, result, refconst.c_str());
    }
    if (result == YM_NO_ITEM_INDEX) {
        ADD_FAILURE();
    }
    return result;
}
inline YmItemIndex setup_fn(
    YmParcelDef* def,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refconsts) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_ITEM_INDEX;
    }
    YmItemIndex result = ymParcelDef_AddFn(def, localName.c_str(), returnTypeSymbol.c_str());
    for (const auto& refconst : refconsts) {
        ymParcelDef_AddRef(def, result, refconst.c_str());
    }
    if (result == YM_NO_ITEM_INDEX) {
        ADD_FAILURE();
    }
    return result;
}
inline YmItemIndex setup_method(
    YmParcelDef* def,
    YmItemIndex owner,
    const std::string& localName,
    const std::string& returnTypeSymbol,
    std::initializer_list<std::string> refconsts) {
    if (!def) {
        ADD_FAILURE();
        return YM_NO_ITEM_INDEX;
    }
    YmItemIndex result = ymParcelDef_AddMethod(def, owner, localName.c_str(), returnTypeSymbol.c_str());
    for (const auto& refconst : refconsts) {
        ymParcelDef_AddRef(def, result, refconst.c_str());
    }
    if (result == YM_NO_ITEM_INDEX) {
        ADD_FAILURE();
    }
    return result;
}

inline void test_item_basics(
    YmItem* item,
    const std::string& fullname,
    YmKind kind,
    std::initializer_list<YmItem*> refconsts) {
    ASSERT_NE(item, nullptr); // For safety.
    EXPECT_STREQ(ymItem_Fullname(item), fullname.c_str());
    EXPECT_EQ(ymItem_Kind(item), kind);
    for (YmItem* refconst : refconsts) {
        EXPECT_NE(ymItem_FindRef(item, refconst), YM_NO_REF);
    }
}
inline void test_struct(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refconsts) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Struct, refconsts);
}
inline void test_fn(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refconsts) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Fn, refconsts);
}
inline void test_method(
    YmItem* item,
    const std::string& fullname,
    std::initializer_list<YmItem*> refconsts) {
    ym::println("-- testing {}", fullname);
    ASSERT_NE(item, nullptr);
    test_item_basics(item, fullname, YmKind_Method, refconsts);
}

