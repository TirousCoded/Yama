

#include "codegen_target.h"

#include "compiler.h"


yama::internal::codegen_target::codegen_target(translation_unit& tu)
    : tu(tu) {}

bool yama::internal::codegen_target::has_target() const noexcept {
    return (bool)_current_target;
}

yama::type_info& yama::internal::codegen_target::target() noexcept {
    return deref_assert(_current_target);
}

const yama::internal::fn_csym& yama::internal::codegen_target::target_csym() {
    const auto symbol = tu->syms.lookup(tu->root(), target().unqualified_name, 0);
    return deref_assert(symbol).as<fn_csym>();
}

std::optional<size_t> yama::internal::codegen_target::target_param_index(const str& name) {
    const auto& params = target_csym().params;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

void yama::internal::codegen_target::gen_new_target(const str& unqualified_name) {
    YAMA_ASSERT(!has_target());
    // bind bare-bones new_type to _current_target
    yama::type_info new_type{
        .unqualified_name = unqualified_name,
        .info = yama::function_info{
            .callsig = callsig_info{}, // stub
            .call_fn = yama::bcode_call_fn,
            .max_locals = 0,
        },
    };
    _current_target = std::move(new_type); // bind new target
    auto& our_type = target(); // <- replaces new_type (which we moved from)
    // build callsig of our_type (which has to be done after binding it as
    // we need to populate its constant table)
    auto new_callsig = tu->ctp.build_callsig_for_fn_type(tu->types.load(unqualified_name).value());
    // patch new_callsig onto our_type
    std::get<function_info>(our_type.info).callsig = std::move(new_callsig);
}

void yama::internal::codegen_target::upload_target(const ast_node& where) {
    YAMA_ASSERT(has_target());
    _apply_bcode_to_target(where);
    tu->output.add_type(std::move(target()));
    _current_target.reset();
    // reset cw and syms
    cw = bc::code_writer{};
    syms = bc::syms{};
}

void yama::internal::codegen_target::add_sym(taul::source_pos pos) {
    YAMA_ASSERT(has_target());
    const auto loc = tu->src.location_at(pos);
    syms.add(cw.count() - 1, loc.origin, loc.chr, loc.line);
}

void yama::internal::codegen_target::_apply_bcode_to_target(const ast_node& where) {
    YAMA_ASSERT(has_target());
    if (!std::holds_alternative<function_info>(target().info)) return;
    auto& info = std::get<function_info>(target().info);
    bool label_not_found{};
    if (auto fn_bcode = cw.done(&label_not_found)) {
        info.bcode = std::move(*fn_bcode);
        info.bsyms = std::move(syms);
    }
    else {
        if (label_not_found) { // if this, then the compiler is broken
            tu->err.error(
                where,
                dsignal::compile_impl_internal,
                "internal error; label_not_found == true!");
        }
        else { // exceeded branch dist limit
            tu->err.error(
                where,
                dsignal::compile_impl_limits,
                "fn {} contains branch which exceeds max branch offset limits!",
                target().unqualified_name);
        }
    }
}

