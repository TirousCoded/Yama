

#include "register_stk.h"

#include "compiler.h"


#define _LOG_REG_AND_SCOPE_STK_MANIP 0


yama::internal::register_stk::register_stk(translation_unit& tu)
    : tu(tu) {}

size_t yama::internal::register_stk::regs() const noexcept {
    return _regstk.size();
}

size_t yama::internal::register_stk::reg_abs_index(ssize_t index) {
    return index >= 0 ? size_t(index) : regs() + index;
}

yama::internal::register_stk::reg_t& yama::internal::register_stk::reg_abs(size_t index) {
    return _regstk.at(index);
}

yama::internal::register_stk::reg_t& yama::internal::register_stk::reg(ssize_t index) {
    return reg_abs(reg_abs_index(index));
}

yama::internal::register_stk::reg_t& yama::internal::register_stk::top_reg() {
    return _regstk.back();
}

yama::internal::register_stk::scope_t& yama::internal::register_stk::top_scope() {
    return _scopestk.back();
}

void yama::internal::register_stk::push_temp(const ast_node& x, ctype type) {
    if (regs() >= reg_limit) { // register count exceeds impl limit
        tu->err.error(
            x,
            dsignal::compile_impl_limits,
            "fn {} contains parts requiring >{} registers to store all temporaries and local vars, exceeding limit!",
            tu->cgt.target().unqualified_name(),
            reg_limit);
        return;
    }
    const auto ind = regs();
    _regstk.push_back(reg_t{ .type = type, .index = ind });
    _update_max_locals_of_codegen_target(); // can't forget to propagate this to code gen target
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    std::string list_of_regs{};
    {
        bool not_first = false;
        for (const auto& I : _regstk) {
            if (not_first) {
                list_of_regs += ", ";
            }
            list_of_regs += I.type.fmt(tu->e());
            not_first = true;
        }
    }
    YAMA_LOG(
        tu->cs->dbg(), general_c,
        "*push* reg {: <15} ({} -> {}) ~> [ {} ]",
        type.fmt(tu->e()),
        ind,
        ind + 1,
        list_of_regs);
#endif
}

void yama::internal::register_stk::pop_temp(size_t n, bool write_pop_instr) {
    if (n > regs()) n = regs(); // <- avoid undefined behaviour for pop_back
    if (write_pop_instr && n >= 1) { // don't write pop instr w/ 0 oprand
        tu->cgt.cw.add_pop(uint8_t(n));
    }
    for (auto nn = n; nn >= 1; nn--) {
        _regstk.pop_back();
    }
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    std::string list_of_regs{};
    {
        bool not_first = false;
        for (const auto& I : _regstk) {
            if (not_first) {
                list_of_regs += ", ";
            }
            list_of_regs += I.type.fmt(tu->e());
            not_first = true;
        }
    }
    YAMA_LOG(
        tu->cs->dbg(), general_c,
        "*pop* reg {: <16} ({} -> {}) ~> [ {} ]",
        n,
        regs() + n,
        regs(),
        list_of_regs);
#endif
}

void yama::internal::register_stk::push_scope() {
    _scopestk.push_back(scope_t{ .first_reg = regs() });
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "\n*enter* scope #{}\n", _scopestk.size());
#endif
}

void yama::internal::register_stk::pop_scope(bool write_pop_instr) {
    const size_t regs_to_pop = regs() - top_scope().first_reg;
    pop_temp(regs_to_pop, write_pop_instr); // unwind temporaries and local vars
    _scopestk.pop_back();
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "\n*exit* scope #{}\n", _scopestk.size() + 1);
#endif
}

void yama::internal::register_stk::reinit_temp(ssize_t index, ctype new_type) {
    reg(index).type = new_type;
#if _LOG_REG_AND_SCOPE_STK_MANIP == 1
    YAMA_LOG(tu->cs->dbg(), general_c, "*reinit* reg {} {}", reg_abs_index(index), new_type.fmt(tu->e()));
#endif
}

void yama::internal::register_stk::promote_to_localvar(ast_VarDecl& x) {
    YAMA_ASSERT(top_reg().is_temp());
    const auto localvar_name = x.name.str(tu->src);
    top_reg().localvar = true; // tell the register it's now a local var
    // update symbol table entry
    if (const auto symbol = tu->syms.lookup_as<var_csym>(x, localvar_name, x.high_pos())) {
        // when we promote a temporary to a local var, its symbol is expected to not
        // yet have an associated register index, w/ this assigning one
        YAMA_ASSERT(!symbol->reg);
        symbol->reg = top_reg().index;
    }
    YAMA_ASSERT(top_reg().is_localvar());
}

bool yama::internal::register_stk::type_check_reg(ssize_t index, ctype expected) {
    return reg(index).type == expected;
}

void yama::internal::register_stk::_update_max_locals_of_codegen_target() {
    auto& info = tu->cgt.target();
    info.change_max_locals(std::max(info.max_locals(), regs()));
}

