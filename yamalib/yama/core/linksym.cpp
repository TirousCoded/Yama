

#include "linksym.h"


yama::linksyms_factory& yama::linksyms_factory::primitive(str fullname) {
    _result.push_back(make_linksym(fullname, kind::primitive));
    return *this;
}

yama::linksyms_factory& yama::linksyms_factory::function(str fullname, callsig_info callsig) {
    _result.push_back(make_linksym(fullname, kind::function, std::move(callsig)));
    return *this;
}

std::vector<yama::linksym> yama::linksyms_factory::done() {
    return std::move(_result);
}

