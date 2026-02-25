

#pragma once


#include "Redirects.h"
#include "SpecParser.h"


namespace _ym {


    // Used to 'solve' specifiers, substituting %here, $Self, $T, etc. w/ specific specifiers, if possible.
    // This also performs 'normalization' of input specifiers.
    class SpecSolver final : public SpecEval {
    public:
        enum class Type : YmUInt8 {
            Path,
            Type,
        };
        enum class MustBe : YmUInt8 {
            Path,
            Type,
            Either,
        };


        // NOTE: Callbacks fire REGARDLESS of whether %here, $Self, $T, etc. are otherwise defined as part of
        //       the environment in which solving is occurring.

        std::function<void()> hereCallback;
        std::function<void()> selfCallback;
        std::function<void(taul::str id, bool rootOfEntireTree)> typeParamCallback;


        SpecSolver(
            YmParcel* here = nullptr,
            YmType* typeParamsCtx = nullptr,
            YmType* self = nullptr,
            RedirectSet* redirects = nullptr
        ) noexcept;


        std::optional<std::string> operator()(const SpecParser::Result& specifier, Type& type, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const SpecParser::Result& specifier, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const taul::str& specifier, Type& type, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const taul::str& specifier, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const std::string& specifier, Type& type, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const std::string& specifier, MustBe mustBe = MustBe::Either);


    private:
        struct _Scope final {
            // Latest expr encountered in this scope, if any.
            std::optional<Type> latest = {};
            std::string output = "";
        };


        // NOTE: Take note how getting specifiers from _here, _typeParamCtx, and _self,
        //       rather than user provided strings, guarantees they'll be normalized.

        YmParcel* _here = nullptr;
        YmType* _typeParamCtx = nullptr, * _self = nullptr;
        RedirectSet* _redirects = nullptr;

        // TODO: Maybe use thread-local field to optimize below.

        bool _failFlag = false;
        bool _firstId = false;
        bool _atRootOfEntireTree = false;
        std::vector<_Scope> _scopes;

        bool _hasScope() const noexcept;
        _Scope& _scope() noexcept;
        const _Scope& _scope() const noexcept;

        bool _hasExpr() const noexcept;
        bool _isPath() const noexcept;
        bool _isType() const noexcept;

        bool _expectPath();
        bool _expectPathOrNone();
        bool _expectType();
        bool _expectTypeOrNone();

        void _markAsPath() noexcept;
        void _markAsType() noexcept;

        void _handleRedirectsIfPath();

        bool _good() const;
        void _fail();

        template<typename... Args>
        inline void _emit(std::format_string<Args...> fmt, Args&&... args) {
            if (_good()) {
                _scope().output += std::format(fmt, std::forward<Args>(args)...);
            }
        }

        void _beginSolve();
        std::optional<std::string> _endSolve(Type& type, MustBe mustBe);

        void _beginScope();
        std::string _endScope();
        void _endScopeAndEmit();


        void syntaxErr() override;
        void rootId(const taul::str& id) override;
        void slashId(const taul::str& id) override;
        void colonId(const taul::str& id) override;
        void dblColonId(const taul::str& id) override;
        void openTypeArgs() override;
        void typeArgsArgDelimiter() override;
        void closeTypeArgs() override;
        void openCallSuff() override;
        void callSuffParamDelimiter() override;
        void callSuffReturnType() override;
        void closeCallSuff() override;
    };


    // ymAssert(s) that specifier is normal.
    void assertNormal(const std::string& specifier) noexcept;

    // Returns if specifier contains $Self anywhere within it.
    bool specifierHasSelf(const std::string& specifier);

    std::pair<std::string_view, std::optional<std::string_view>> seperateCallSuff(const std::string& normalizedSpecifier) noexcept;

    void assertNormalNonCallSig(const std::string& specifier) noexcept;
    void assertNormalCallSig(const std::string& specifier) noexcept;
}

