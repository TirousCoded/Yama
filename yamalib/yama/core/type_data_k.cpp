

#include "type_data_k.h"


bool yama::type_data_k::operator==(const type_data_k& other) const noexcept {
    return
        fullname == other.fullname;
}

size_t yama::type_data_k::hash() const noexcept {
    return std::hash<str>{}(fullname);
}

std::string yama::type_data_k::fmt() const {
    return std::string(fullname);
}

