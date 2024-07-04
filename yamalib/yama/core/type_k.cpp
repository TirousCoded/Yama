

#include "type_k.h"


bool yama::type_k::operator==(const type_k& other) const noexcept {
    return
        fullname == other.fullname;
}

size_t yama::type_k::hash() const noexcept {
    return std::hash<str>{}(fullname);
}

std::string yama::type_k::fmt() const {
    return std::string(fullname);
}

