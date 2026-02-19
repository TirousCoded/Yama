

#pragma once


#include <string>

#include "../yama/yama.h"
#include "../yama++/meta.h"
#include "../yama++/Safe.h"
#include "../yama++/Variant.h"
#include "Area.h"
#include "ParcelInfo.h"
#include "PathBindings.h"
#include "Redirects.h"
#include "SpecParser.h"
#include "YmItem.h"


namespace _ym {


    // NOTE: Terms have the following types:
    //          - Errors: Encapsulates error from failed operation.
    //              * These exist to make system more stable in situations where an operation fails, as
    //                instead pushing nothing to term stack leaves it malformed, which can lead to crashes.
    //          - Paths: Encapsulates path strings (ie. no actual import has necessarily occurred.)
    //          - Items: Encapsulates concrete/generic items (ie. w/ an associated fullname.)

    struct Term final {
        struct ErrorData final {
        };
        struct PathData final {
            std::string path;
        };
        struct ConcreteData final {
            ym::Safe<YmItem> item;
        };
        struct GenericData final {
            std::string path;
            ym::Safe<const ItemInfo> info;
        };
        ym::Variant<
            ErrorData,
            PathData,
            ConcreteData,
            GenericData
        > data = PathData{};

        bool awaitingArgs = false;


        // Default inits error term.
        inline Term() :
            data(ErrorData{}) {
        }

        Term(const Term&) = default;
        Term(Term&&) noexcept = default;
        ~Term() noexcept = default;
        Term& operator=(const Term&) = default;
        Term& operator=(Term&&) noexcept = default;

        // For paths.
        inline explicit Term(std::string path) :
            data(PathData(std::move(path))) {
        }
        // For concrete items.
        inline explicit Term(ym::Safe<YmItem> item) :
            data(ConcreteData(item)) {
        }
        // For generic items.
        inline Term(std::string path, ym::Safe<const ItemInfo> info) :
            data(GenericData{ std::move(path), info }) {
            ymAssert(info->isParameterized());
        }


        inline bool isErr() const noexcept { return data.index() == 0; }
        inline bool isPath() const noexcept { return data.index() == 1; }
        inline bool isItem() const noexcept { return isConcrete() || isGeneric(); }
        inline bool isConcrete() const noexcept { return data.index() == 2; }
        inline bool isGeneric() const noexcept { return data.index() == 3; }

        inline std::optional<YmKind> kind() const noexcept {
            return
                isItem()
                ? std::make_optional(info()->kind)
                : std::nullopt;
        }

        inline bool hasMembers() const noexcept {
            if (auto k = kind())    return ymKind_HasMembers(*k) == YM_TRUE;
            else                    return false;
        }
        inline bool hasMember(const std::string& name) const noexcept {
            return
                isItem()
                ? info()->membersByName.contains(name)
                : false;
        }

        inline std::optional<Spec> path() const {
            if (auto result = data.tryAs<1>())      return Spec::path(result->path);
            else if (auto result = data.tryAs<3>()) return Spec::path(result->path);
            else                                    return std::nullopt;
        }
        inline YmItem* concrete() const noexcept {
            if (auto result = data.tryAs<2>())  return result->item;
            else                                return nullptr;
        }
        inline const ItemInfo* generic() const noexcept {
            if (auto result = data.tryAs<3>())  return result->info;
            else                                return nullptr;
        }

        inline YmItem* item() const noexcept {
            return concrete();
        }
        inline const ItemInfo* info() const noexcept {
            if (isConcrete())       return concrete()->info;
            else if (isGeneric())   return generic();
            else                    return nullptr;
        }

        inline size_t itemParamCount() const noexcept {
            return
                isItem()
                ? ym::deref(info()).itemParamCount()
                : 0;
        }

        inline std::string fmt(bool includeTermType = false) const {
            if (!includeTermType) {
                if (isErr())            return "<error>";
                else if (isPath())      return path().value();
                else if (isConcrete())  return item()->fullname();
                else if (isGeneric())   return std::format("{}:{}", path().value(), info()->localName);
                else                    return "???";
            }
            else {
                if (isErr())            return "[Error]";
                else if (isPath())      return "[Path " + path().value().fmt() + "]";
                else if (isConcrete())  return "[Concrete " + item()->fullname().string() + "]";
                else if (isGeneric())   return "[Generic " + std::format("{}:{}", path().value(), info()->localName) + "]";
                else                    return "[???]";
            }
        }
    };


    class TermStk final {
    public:
        // Index of a term in the term stack.
        // Negative values index from top-down, Lua-style, where -1 is the top term.
        using TermIndex = std::make_signed_t<size_t>;

        using GenNonMemberItemDataCallback = std::function<ym::Safe<YmItem>(YmParcel& p, const ItemInfo& info, std::vector<ym::Safe<YmItem>> itemArgs)>;


        const ym::Safe<Area> staging;
        const ym::Safe<PathBindings> binds;
        const ym::Safe<Redirects> redirects;
        const GenNonMemberItemDataCallback genNonMemberItemDataCallback;


        TermStk(
            Area& staging,
            PathBindings& binds,
            Redirects& redirects,
            GenNonMemberItemDataCallback genNonMemberItemDataCallback);


        size_t height() const noexcept;
        Term* tryTerm(TermIndex where) noexcept;
        const Term* tryTerm(TermIndex where) const noexcept;
        Term& term(TermIndex where);
        const Term& term(TermIndex where) const;
        std::span<const Term> topN(size_t n) const noexcept;


        // item is the item who's ref syms are being processed.
        // here is the %here path.
        // self is the $Self item.
        void beginSession(
            std::string errPrefix,
            YmItem& item,
            std::string here,
            YmItem& self);
        void beginSession(
            std::string errPrefix);
        void endSession();


        // Pushes x to the term stack.
        void push(Term x);
        // Pops first n terms from the term stack.
        void pop(size_t n = 1);
        void popAll();
        // Pops first n terms from the term stack, then pushes x.
        void transact(size_t n, Term x);
        // Pops top n terms, and pushes an error term.
        void transactErr(size_t n);

        // TODO: Look into making a metamethod to help w/ stack operations in terms of keeping
        //       their transactions on the term stack correct, as right now we need to manually
        //       keep track of every little edge case, w/ it crashing horribly if we don't.

        // TODO: Should below two be private?
        
        // Checks top n terms, which are supposed to be the inputs to some term stack operation, and if
        // any of them are error terms, it pops them all, and pushes an error term.
        // Returns if none of the terms were error terms.
        bool verifyInputs(size_t n);

        // Helper which ensures that transactErr gets called when raising an error.
        // Gotta remember to only call this once in an operation method though.
        template<typename... Args>
        inline void operErr(
            size_t inputs,
            YmErrCode code,
            std::format_string<Args...> fmt,
            Args&&... args) {
            transactErr(inputs);
            _err(code, fmt, std::forward<Args>(args)...);
        }


        // NOTE: The below operations use 'stack effect' notation.
        // NOTE: By default, operations fail quietly if input has error term(s).

        // TODO: Refactor our expect*** methods to instead be based on index (ie. and so move them above)
        //       and then refactor our impl code to use these for the various checks in TermStk methods.

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is an item term.
        // Returns if this check succeeds.
        bool expectItem() const;

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is a concrete (ie. non-generic) item term.
        // Returns <item> if this check succeeds.
        YmItem* expectConcrete() const;

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is a generic (ie. non-concrete) item term.
        // Returns if this check succeeds.
        bool expectGeneric() const;

        // StkFx: ... <path> -- ... <path>
        // <path> is an import path.
        // Attempts import of <path>, returning the imported parcel, if any.
        YmParcel* importParcel();

        // StkFx: ... -- ... <result>
        // Evaluates specifier in the current scope, pushing <result>.
        void specifier(const Spec& specifier);

        // StkFx: ... -- ... <result>
        // Evaluates path in the current scope, pushing <result>.
        // This does not perform any complex parsing (and %here is presumed to be unavailable.)
        // As usual, this does not perform actual import.
        void path(const Spec& path);

        // StkFx: ... -- ... <result>
        // Evaluates fullname in the current scope, pushing <result>.
        // Avoids complex parsing if an already loaded item is found, forwarding to eval otherwise.
        void fullname(const Spec& fullname);

        // StkFx: ... -- ... <here>
        // <here> is the path resolved by %here.
        void here();

        // StkFx: ... -- ... <root>
        // <root> is a root import path.
        void root(const std::string& id);

        // StkFx: ... <file> -- ... <subdir>
        // <file> is a path specifying a file/directory.
        // <subdir> is a path to a subdirectory within <file> (ie. '<file>/<id>'.)
        void subdir(const std::string& id);

        // StkFx: ... -- ... <self>
        // <self> is the item resolved by $Self.
        void self();

        // StkFx: ... -- ... <itemParam>
        // <itemParam> is the item resolved by the specified item parameter (ie. '$<id>'.)
        void itemParam(const std::string& id);

        // StkFx: ... <path> -- ... <item>
        // <path> is an import path.
        // <item> is an item within the parcel at <path> (ie. '<path>:<id>'.)
        void itemInParcel(const std::string& id);

        // StkFx: ... <owner> -- ... <member>
        // <owner> is a non-member item.
        // <member> is a member item within <owner> (ie. '<owner>::<id>'.)
        void member(const std::string& id);

        // StkFx: ... <generic> -- ... <generic>
        // <generic> is the top-most generic item term.
        // Marks <generic> as the item who's item arguments are being defined.
        void beginArgs();

        // StkFx: ... <generic> <args> -- ... <concrete>
        // <generic> is the top-most generic item term, marked by beginArgs.
        // <args> is an array of all items above <generic>.
        // <concrete> is the concrete item constructed by applying <args> to <generic>.
        void endArgs();


    private:
        struct _Env final {
            ym::Safe<YmItem> item;
            std::string here;
            ym::Safe<YmItem> self;
        };
        struct _Sess final {
            std::string errPrefix;
            std::optional<_Env> env;
        };

        class _Interp final : public SpecEval {
        public:
            _Interp(TermStk& client);


            void operator()(const std::string& specifier);


        private:
            ym::Safe<TermStk> _client;
            const std::string* _specifierPtr = nullptr;


            const std::string& _specifier() const noexcept;


            void syntaxErr() override;
            void rootId(const taul::str& id) override;
            void slashId(const taul::str& id) override;
            void colonId(const taul::str& id) override;
            void dblColonId(const taul::str& id) override;
            void openItemArgs() override;
            void itemArgsArgDelimiter() override;
            void closeItemArgs() override;
            void openCallSuff() override;
            void callSuffParamDelimiter() override;
            void callSuffReturnType() override;
            void closeCallSuff() override;
        };


        std::optional<_Sess> _session;
        std::vector<Term> _terms;


        size_t _absInd(TermIndex index) const noexcept;

        void _assertSess() const noexcept;
        const _Sess& _sess() const noexcept;
        const std::string& _errPrefix() const noexcept;
        const _Env* _env() const noexcept;

        template<typename... Args>
        inline void _err(
            YmErrCode code,
            std::format_string<Args...> fmt,
            Args&&... args) const {
            _ym::Global::raiseErr(code, fmt, std::forward<Args>(args)...);
        }

        YmParcel* _import(const Spec& path);

        std::optional<size_t> _countInputsToEndArgs() const noexcept;
        static std::vector<ym::Safe<YmItem>> _assembleItemArgs(std::span<const Term> args);

        void _printTermStk(size_t oldHeight);
    };
}

