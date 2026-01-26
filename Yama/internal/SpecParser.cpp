

#include "SpecParser.h"


_ym::SpecParser::Result _ym::SpecParser::operator()(const taul::str& x) {
    using namespace taul::string_literals;
    // TODO: Use thread-local storage to avoid reinitializing rdr, lxr, psr, etc. each time.
    taul::source_reader rdr(x);
    taul::lexer lxr(specifiersGram);
    taul::parser psr(specifiersGram);
    taul::no_recovery_error_handler err{};
    lxr.bind_source(&rdr);
    psr.bind_source(&lxr);
    psr.bind_error_handler(&err);
    psr.reset(); // Flush pipeline to prep for parse.
    return Result{
        .input = x,
        .tree = psr.parse("TopLevel"_str),
    };
}

bool _ym::SpecEval::eval(const SpecParser::Result& x) {
    _idMode = _IdMode::Root;
    _input = x.input;
    playback(x.tree);
    return x;
}

void _ym::SpecEval::on_lexical(taul::token tkn) {
    if (tkn && tkn.lpr) {
        if (tkn.lpr->name() == "IDENTIFIER") {
            auto id = tkn.str(_input);
            if (_idMode == _IdMode::Root)		    rootId(id);
            else if (_idMode == _IdMode::Slash)	    slashId(id);
            else if (_idMode == _IdMode::Colon)	    colonId(id);
            else if (_idMode == _IdMode::DblColon)  dblColonId(id);
            else								    YM_DEADEND;
        }
        else if (tkn.lpr->name() == "L_SQUARE")	    openArgs();
        else if (tkn.lpr->name() == "R_SQUARE")	    closeArgs();
        else if (tkn.lpr->name() == "SLASH")	    _idMode = _IdMode::Slash;
        else if (tkn.lpr->name() == "COLON")	    _idMode = _IdMode::Colon;
        else if (tkn.lpr->name() == "DBL_COLON")    _idMode = _IdMode::DblColon;
    }
}

void _ym::SpecEval::on_syntactic(taul::ppr_ref ppr, taul::source_pos pos) {
    if (ppr.name() == "Expr") {
        _idMode = _IdMode::Root;
    }
}

void _ym::SpecEval::on_startup() {

}

void _ym::SpecEval::on_shutdown() {

}

void _ym::SpecEval::on_close() {

}

void _ym::SpecEval::on_abort() {
    syntaxErr();
}

void _ym::SpecEval::on_terminal_error(taul::token_range ids, taul::token input) {

}

void _ym::SpecEval::on_nonterminal_error(taul::symbol_id id, taul::token input) {

}

