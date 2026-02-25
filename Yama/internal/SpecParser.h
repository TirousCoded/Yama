

#pragma once


#include <taul/all.h>
#include <taul/source_reader.h>
#include <taul/lexer.h>
#include <taul/listener.h>
#include <taul/no_recovery_error_handler.h>

#include "../yama++/Safe.h"

#include "codegen/sigs-codegen.h"


namespace _ym {


	inline const taul::grammar specifiersGram = taul::fetchers::sigs();


	// Used to parse a specifier into a parse tree.
	class SpecParser final {
	public:
		struct Result final {
			taul::str input;
			taul::parse_tree tree;


			inline operator bool() const noexcept {
				return !tree.is_aborted();
			}
		};


		SpecParser() = default;


		Result operator()(const taul::str& x);
	};


	// Abstract base class of specifier evaluators, which evaluate parsed specifiers.
	class SpecEval : private taul::listener {
	public:
		SpecEval() = default;


	protected:
		bool eval(const SpecParser::Result& x);


		virtual void syntaxErr() = 0;
		virtual void rootId(const taul::str& id) = 0;
		virtual void slashId(const taul::str& id) = 0;
		virtual void colonId(const taul::str& id) = 0;
		virtual void dblColonId(const taul::str& id) = 0;
		virtual void openTypeArgs() = 0;
		virtual void typeArgsArgDelimiter() = 0;
		virtual void closeTypeArgs() = 0;
		virtual void openCallSuff() = 0;
		virtual void callSuffParamDelimiter() = 0;
		virtual void callSuffReturnType() = 0;
		virtual void closeCallSuff() = 0;


	private:
		enum class _IdMode : YmUInt8 {
			Root,
			Slash,
			Colon,
			DblColon,
		};
		enum class _Stage : YmUInt8 {
			Main,
			CallSuffParams,
			CallSuffReturnType,
		};


		bool _success = {};
		_IdMode _idMode = {};
		_Stage _stage = _Stage::Main;
		taul::str _input;


		void on_startup() override;
		void on_shutdown() override;
		void on_lexical(taul::token tkn) override;
		void on_syntactic(taul::ppr_ref ppr, taul::source_pos pos) override;
		void on_close() override;
		void on_abort() override;
		void on_terminal_error(taul::token_range ids, taul::token input) override;
		void on_nonterminal_error(taul::symbol_id id, taul::token input) override;
	};
}

