

#include "codegen_target.h"

#include "../core/kind-features.h"

#include "compiler.h"


yama::internal::codegen_target::codegen_target(translation_unit& tu)
    : tu(tu),
    cw(syms) {}

bool yama::internal::codegen_target::has_target() const noexcept {
    return (bool)_current_target;
}

yama::type_info& yama::internal::codegen_target::target() noexcept {
    return deref_assert(_current_target);
}

std::shared_ptr<yama::internal::csymtab_entry> yama::internal::codegen_target::try_target_csym_entry() {
    return tu->syms.lookup(tu->root(), target().unqualified_name, 0);
}

yama::res<yama::internal::csymtab_entry> yama::internal::codegen_target::target_csym_entry() {
    return res(try_target_csym_entry());
}

std::optional<size_t> yama::internal::codegen_target::target_param_index(const str& name) {
    const auto& params = target_csym<fn_csym>().params;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i].name == name) {
            return i;
        }
    }
    return std::nullopt;
}

void yama::internal::codegen_target::gen_target_fn(const str& unqualified_name) {
    _bind_bare_bones_type_info(
        unqualified_name,
        function_info{
            .callsig = callsig_info{}, // stub
            .call_fn = yama::bcode_call_fn,
            .max_locals = 0,
        });
    // build callsig of new target (which has to be done after binding it as
    // we need to populate its constant table)
    auto new_callsig = tu->ctp.build_callsig_for_fn_type(tu->types.load(unqualified_name).value());
    // patch new_callsig onto new target
    std::get<function_info>(target().info).callsig = std::move(new_callsig);
}

void yama::internal::codegen_target::gen_target_struct(const str& unqualified_name) {
    _bind_bare_bones_type_info(
        unqualified_name,
        struct_info{});
}

void yama::internal::codegen_target::upload_target(const ast_node& where) {
    YAMA_ASSERT(has_target());
    _apply_bcode_to_target(where);
    tu->output.add_type(std::move(target()));
    _current_target.reset();
    // reset cw and syms
    cw = bc::code_writer(syms); // <- don't forget to reassociate w/ syms
    syms = bc::syms{};
}

void yama::internal::codegen_target::autosym(taul::source_pos pos) {
    YAMA_ASSERT(has_target());
    const auto loc = tu->src.location_at(pos);
    cw.autosym(loc.origin, loc.chr, loc.line);
}

void yama::internal::codegen_target::add_cvalue_put_instr(uint8_t reg, const cvalue& x) {
    if (x.is(tu->types.none_type()))            tu->cgt.cw.add_put_none(reg);
    else if (const auto v = x.as<int_t>())      tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_int(*v)));
    else if (const auto v = x.as<uint_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_uint(*v)));
    else if (const auto v = x.as<float_t>())    tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_float(*v)));
    else if (const auto v = x.as<bool_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_bool(*v)));
    else if (const auto v = x.as<char_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_char(*v)));
    else if (const auto v = x.to_type()) {
        if (is_primitive(v->kind()))            tu->cgt.cw.add_put_type_const(reg, uint8_t(tu->ctp.pull_type(*v)));
        else if (is_function(v->kind()))        tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_fn_type(*v)));
        else if (is_struct(v->kind()))          tu->cgt.cw.add_put_type_const(reg, uint8_t(tu->ctp.pull_type(*v)));
        else                                    YAMA_DEADEND;
    }
    else                                        YAMA_DEADEND;
}

void yama::internal::codegen_target::_bind_bare_bones_type_info(const str& unqualified_name, type_info::info_t&& info) {
    YAMA_ASSERT(!has_target());
    // bind bare-bones new_type to _current_target
    _current_target = yama::type_info{
        .unqualified_name = unqualified_name,
        .info = std::forward<decltype(info)>(info),
    };
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

