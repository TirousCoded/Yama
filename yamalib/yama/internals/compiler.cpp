

#include "compiler.h"

#include "domain_data.h"


yama::internal::compiler::compiler(std::shared_ptr<debug> dbg, domain_data& dd)
    : api_component(dbg),
    _cs(dbg, dd) {}

bool yama::internal::compiler::compile(const taul::source_code& src, const import_path& src_path) {
    return _cs.compile(src, src_path);
}

