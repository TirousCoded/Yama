

#pragma once


#include "SpecParser.h"

#include "../yama++/print.h"
namespace _ym {


    // Used to 'solve' specifiers, substituting %here, $Self, $T, etc. w/ specific specifiers, if possible.
    // This also performs 'normalization' of input specifiers.
    class SpecSolver final : public SpecEval {
    public:
        enum class MustBe : YmUInt8 {
            Path,
            Item,
            Either,
        };


        // NOTE: Callbacks fire REGARDLESS of whether %here, $Self, $T, etc. are otherwise defined as part of
        //       the environment in which solving is occurring.

        std::function<void()> hereCallback;
        std::function<void()> selfCallback;
        std::function<void(taul::str id, bool rootOfEntireTree)> itemParamCallback;


        SpecSolver(
            YmParcel* here = nullptr,
            YmItem* itemParamsCtx = nullptr,
            YmItem* self = nullptr
        ) noexcept;


        std::optional<std::string> operator()(const SpecParser::Result& specifier, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const taul::str& specifier, MustBe mustBe = MustBe::Either);
        std::optional<std::string> operator()(const std::string& specifier, MustBe mustBe = MustBe::Either);


    private:
        // NOTE: Take note how getting specifiers from _here, _itemParamCtx, and _self,
        //       rather than user provided strings, guarantees they'll be normalized.

        YmParcel* _here = nullptr;
        YmItem* _itemParamCtx = nullptr, * _self = nullptr;
        std::optional<std::string> _output;

        // TODO: Maybe use thread-local field to optimize _expectItemNotPath.

        bool _firstId = false;
        bool _atRootOfEntireTree = false;
        std::vector<bool> _expectItemNotPath;


        bool _isPath() const noexcept;
        bool _isItem() const noexcept;

        bool _expectPath();
        bool _expectItem();

        void _shouldExpectPath() noexcept;
        void _shouldExpectItem() noexcept;

        bool _good() const;
        void _fail();

        template<typename... Args>
        inline void _emit(std::format_string<Args...> fmt, Args&&... args) {
            if (_output) {
                *_output += std::format(fmt, std::forward<Args>(args)...);
                //ym::println("_emit -> {}", *_output);
            }
        }

        void _beginSolve();
        std::optional<std::string> _endSolve(MustBe mustBe);

        void _beginScope();
        void _endScope();


        void syntaxErr() override;
        void rootId(const taul::str& id) override;
        void openArgs() override;
        void closeArgs() override;
        void slashId(const taul::str& id) override;
        void colonId(const taul::str& id) override;
        void dblColonId(const taul::str& id) override;
    };


    // ymAssert(s) that specifier is normal.
    void assertNormal(const std::string& specifier) noexcept;

    // Returns if specifier contains $Self anywhere within it.
    bool specifierHasSelf(const std::string& specifier);
}

