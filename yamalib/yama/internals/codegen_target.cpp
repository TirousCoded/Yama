

#include "codegen_target.h"

#include "../core/kind-features.h"

#include "compiler.h"


yama::internal::codegen_target::codegen_target(translation_unit& tu)
    : tu(tu),
    cw(syms) {}

void yama::internal::codegen_target::start(const ast_FnDecl& decl) {
    YAMA_ASSERT(!has_target());
    where = &decl;
    kind = decl.is_fn() ? kind::function : kind::method;
    owner_name = decl.name.value().str(tu->src);
    member_name =
        decl.is_method()
        ? std::make_optional(decl.method_name.value().str(tu->src))
        : std::nullopt;
    unqualified_name = str(decl.fmt_unqualified_name(tu->src).value());
    consts = const_table{}; // Reset this.
    callsig.reset();
    max_locals = 0; // Reset this.
    // Build callsig of new item (which has to be done AFTER above as
    // we need to populate its constant table.)
    callsig = tu->ctp.build_callsig_for_fn_type(tu->types.load(unqualified_name).value());
}

void yama::internal::codegen_target::start(const ast_StructDecl& decl) {
    YAMA_ASSERT(!has_target());
    where = &decl;
    kind = kind::struct0;
    owner_name = decl.name.str(tu->src);
    member_name.reset();
    unqualified_name = owner_name;
    consts = const_table{}; // Reset this.
    callsig.reset();
    max_locals = 0; // Reset this.
}

void yama::internal::codegen_target::finish() {
    YAMA_ASSERT(has_target());
    if (kind == kind::function) {
        tu->output.add_function(
            owner_name,
            std::move(consts),
            std::move(callsig.value()),
            max_locals,
            yama::bcode_call_fn);
        _apply_bcode_to_output();
    }
    else if (kind == kind::method) {
        tu->output.add_method(
            owner_name,
            member_name.value(),
            std::move(consts),
            std::move(callsig.value()),
            max_locals,
            yama::bcode_call_fn);
        _apply_bcode_to_output();
    }
    else if (kind == kind::struct0) {
        tu->output.add_struct(
            owner_name,
            std::move(consts));
    }
    else YAMA_DEADEND;
    // Reset cw and syms.
    cw = bc::code_writer(syms); // <- Can't forget to reassociate w/ syms.
    syms = bc::syms{};
    // Declare has no current target.
    where = nullptr;
}

std::shared_ptr<yama::internal::csym> yama::internal::codegen_target::try_target_csym_entry() {
    YAMA_ASSERT(has_target());
    return tu->syms.lookup(tu->root(), unqualified_name, 0);
}

yama::res<yama::internal::csym> yama::internal::codegen_target::target_csym_entry() {
    return res(try_target_csym_entry());
}

std::optional<size_t> yama::internal::codegen_target::target_param_index(const str& name) {
    const auto& params = target_csym<fn_like_csym>().params;
    for (size_t i = 0; i < params.size(); i++) {
        if (params[i]->name == name) {
            return i;
        }
    }
    return std::nullopt;
}

void yama::internal::codegen_target::autosym(taul::source_pos pos) {
    YAMA_ASSERT(has_target());
    const auto loc = tu->src.location_at(pos);
    cw.autosym(loc.origin, loc.chr, loc.line);
}

void yama::internal::codegen_target::add_cvalue_put_instr(uint8_t reg, const cvalue& x, bool reinit) {
    YAMA_ASSERT(has_target());
    if (x.is(tu->types.none_type()))            tu->cgt.cw.add_put_none(reg, reinit);
    else if (const auto v = x.as<int_t>())      tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_int(*v)), reinit);
    else if (const auto v = x.as<uint_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_uint(*v)), reinit);
    else if (const auto v = x.as<float_t>())    tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_float(*v)), reinit);
    else if (const auto v = x.as<bool_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_bool(*v)), reinit);
    else if (const auto v = x.as<char_t>())     tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_char(*v)), reinit);
    else if (const auto v = x.to_type()) {
        static_assert(kinds == 4); // reminder
        // IMPORTANT: Test w/ the original cvalue x, and NOT the ctype v!
        if (is_primitive(x.t.kind()))           tu->cgt.cw.add_put_type_const(reg, uint8_t(tu->ctp.pull_type(*v)), reinit);
        else if (is_function(x.t.kind()))       tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_fn_type(*v)), reinit);
        else if (is_method(x.t.kind()))         tu->cgt.cw.add_put_const(reg, uint8_t(tu->ctp.pull_method_type(*v)), reinit);
        else if (is_struct(x.t.kind()))         tu->cgt.cw.add_put_type_const(reg, uint8_t(tu->ctp.pull_type(*v)), reinit);
        else                                    YAMA_DEADEND;
    }
    else                                        YAMA_DEADEND;
}

void yama::internal::codegen_target::_apply_bcode_to_output() {
    YAMA_ASSERT(has_target());
    bool label_not_found{};
    if (auto fn_bcode = cw.done(&label_not_found)) {
        tu->output.bind_bcode(unqualified_name, std::move(*fn_bcode), std::move(syms));
    }
    else {
        if (label_not_found) { // If this, then the compiler is broken.
            tu->err.error(
                deref_assert(where),
                dsignal::compile_impl_internal,
                "internal error; label_not_found == true!");
        }
        else { // Exceeded branch dist limit.
            tu->err.error(
                deref_assert(where),
                dsignal::compile_impl_limits,
                "fn {} contains branch which exceeds max branch offset limits!",
                unqualified_name);
        }
    }
}

