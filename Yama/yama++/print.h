

#pragma once


#include <format>
#include <iostream>


namespace ym {


    template<typename... Args>
    inline void print(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    inline void println(std::format_string<Args...> fmt, Args&&... args) {
        std::cout << std::format(fmt, std::forward<Args>(args)...) << "\n";
    }
}

