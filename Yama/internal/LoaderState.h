

#pragma once


#include <optional>
#include <queue>

#include "../yama/yama.h"
#include "../yama++/Safe.h"
#include "../yama++/Variant.h"
#include "Area.h"
#include "ParcelInfo.h"
#include "PathBindings.h"
#include "SpecParser.h"
#include "YmItem.h"


namespace _ym {


    class LoaderState final {
    public:
        enum class Mode : YmUInt8 {
            Path,
            Fullname,
            RefSym,
        };

        // Info about the environment in which loading occurs.
        struct Env final {
            ym::Safe<YmItem>    item;   // The item who's ref syms are being processed.
            std::string			here;	// The %here path.
            ym::Safe<YmItem>	self;	// The $Self item.
        };

        // Info about a particular 'term' during specifier eval.
        struct Term final {
            struct PathData final {
                std::string path = "";
            };
            struct ConcreteData final {
                ym::Safe<YmItem> item;
            };
            struct GenericData final {
                std::string path;
                ym::Safe<const ItemInfo> info;
            };
            ym::Variant<
                PathData,
                ConcreteData,
                GenericData
            > data = PathData{};

            bool awaitingArgs = false;


            Term() = default;
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


            inline bool isPath() const noexcept { return data.index() == 0; }
            inline bool isItem() const noexcept { return isConcrete() || isGeneric(); }
            inline bool isConcrete() const noexcept { return data.index() == 1; }
            inline bool isGeneric() const noexcept { return data.index() == 2; }

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

            inline std::optional<std::string> path() const {
                if (auto result = data.tryAs<0>()) {
                    return result->path;
                }
                if (auto result = data.tryAs<2>()) {
                    return result->path;
                }
                return std::nullopt;
            }
            inline YmItem* concrete() const noexcept {
                if (auto result = data.tryAs<1>()) {
                    return result->item;
                }
                return nullptr;
            }
            inline const ItemInfo* generic() const noexcept {
                if (auto result = data.tryAs<2>()) {
                    return result->info;
                }
                return nullptr;
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
                    if (isPath())           return path().value();
                    else if (isConcrete())  return item()->fullname();
                    else if (isGeneric())   return std::format("{}:{}", path().value(), info()->localName);
                    else                    return "???";
                }
                else {
                    if (isPath())           return "[Path " + path().value() + "]";
                    else if (isConcrete())  return "[Concrete " + item()->fullname() + "]";
                    else if (isGeneric())   return "[Generic " + std::format("{}:{}", path().value(), info()->localName) + "]";
                    else                    return "[???]";
                }
            }
        };


        LoaderState(
            ym::Safe<Area> staging,
            ym::Safe<PathBindings> binds);


        // NOTE: Loading occurs via the following:
        //          1) An input specifier is provided by end-user w/ which to load/import an item/parcel,
        //             recursively loading dependencies if the former.
        //          2) The input specifier is parsed and is then interpreted.
        //          3) A stack machine of 'terms' is used during interpretation to compute terms encapsulating
        //             items/parcels, loading/importing incrementally as a function of stack machine operations.
        //          4) When a stack machine operation loads new item data, the following occur:
        //              a) New item data is generated.
        //              b) If a non-member, then item data is generated for each of its members.
        //              c) Performs early resolution of its constant table (this also occurs for members.)
        //              d) Adds the new item (and its members) to the late resolution queue.
        //          5) Each item in the late resolution queue are processed, performing the following for each:
        //              a) Resolves ref consts that weren't early resolvable.
        //              b) This resolution involves a version of the loading sequence described above, but w/
        //                 additional environment metadata (defining things like %here, $Self, etc.)
        //              c) This resolution may likewise add new items to the late resolution queue, which will
        //                 in turn be processed, w/ this processing occurring until the queue is empty.
        //          6) Each item newly loaded undergoes constraint checking for each of their item arguments.

        // NOTE: Terms have the following types:
        //          - Paths: encapsulating path strings (ie. no actual import has necessarily occurred.)
        //          - Items: encapsulating concrete/generic items (ie. w/ an associated fullname.)

        // NOTE: Above 'resolution' describes the process of resolving constant table entries for newly
        //       loaded items, w/ for ref consts this entailing their parsing/interpreting.
        //
        //       Resolution is split up into two phases: 'early' and 'late':
        //          - Early resolution covers value constants, and ref consts of the forms '$Self' and
        //            '$Self::[MEMBER]' which refer to an items owner/members.
        //          - Late resolution covers ref consts not resolved during early resolution.
        //
        //       Early resolution is needed so that items can have their owner/member items queried, which is
        //       needed for certain stack machine operations to be able to execute properly, and so these need
        //       to be done up-front upon initial item data generation.
        //
        //       Originally, all resolution was done up-front like early resolution is, w/ us having a stack
        //       of 'scopes', each w/ their own isolated term stack, w/ generation/resolution of new item data
        //       occurring recursively, traversing the item dependency graph in a depth-first manner.
        //
        //       This was changed so that late resolution occurs after 100% finishing interpreting of the
        //       specifier which originally generated the item, as doing this removed the recursive 'nesting'
        //       of loading events, which was making debugging/maintaining loading system code harder due to
        //       the amount of complexity it added to debug outputs. This change is also intended to make
        //       overall loading system code easier to deal w/.


        // Performs import of normalizedPath.
        // Caller must ensure normalizedPath isn't imported yet.
        YmParcel* import(const std::string& normalizedPath);

        // Performs load of normalizedFullname.
        // Caller must ensure normalizedFullname isn't loaded yet.
        YmItem* load(const std::string& normalizedFullname);


        Mode mode() const noexcept;                         // Returns the current mode.
        const Env* env() const noexcept;                    // Returns the current environment, if any.
        const std::string& errPrefix() const noexcept;


        // NOTE: Whenever a method below w/ a stack effect fails, a 'failure flag' is set,
        //       which deems the entire load state to be a failure.
        //
        //       This flag, once set, will remain set until 'clearFlag' is called.

        bool good() const noexcept;                         // Returns if the failure flag is not set.
        void fail() noexcept;                               // Sets the failure flag.
        void clearFlag() noexcept;

        template<typename... Args>
        inline void err(
            YmErrCode code,
            std::format_string<Args...> fmt,
            Args&&... args) {
            _ym::Global::raiseErr(code, fmt, std::forward<Args>(args)...);
            fail();
        }


        void assertHeight(size_t min) const noexcept;
        size_t height() const noexcept;                     // Returns height of current term stack.

        const Term* top() const noexcept;                   // Returns view of top of current term stack.
        std::span<const Term> topN(                         // Returns view of top std::min(n, height()) terms of current term stack.
            size_t n) const noexcept;
        ym::Safe<const Term> expect() const noexcept;       // Like top, but expects not to fail.
        std::span<const Term> expectN(                      // Like topN, but expects not to fail.
            size_t n) const noexcept;


        void push(Term x);                                  // Pushes x to the term stack.
        void pop(size_t n = 1);                             // Pops first n terms from the term stack.
        void popAll();
        void transact(size_t n, Term x);                    // Pops first n terms from the term stack, then pushes x.


        // NOTE: The below operations use 'stack effect' notation.

        // StkFx: ... -- ... <result>
        // Evaluates pathOrFullname in the current scope, pushing <result>.
        void eval(const std::string& pathOrFullname);

        // TODO: normalizedPath is currently unused.

        // StkFx: ... -- ... <result>
        // Evaluates normalizedPath in the current scope, pushing <result>.
        // This does not perform any complex parsing (and %here is presumed to be unavailable.)
        // As usual, this does not perform actual import.
        void normalizedPath(const std::string& normalizedPath);

        // StkFx: ... -- ... <result>
        // Evaluates normalizedFullname in the current scope, pushing <result>.
        // Avoids complex parsing if an already loaded item is found, forwarding to eval otherwise.
        void normalizedFullname(const std::string& normalizedFullname);

        // StkFx: ... <path> -- ... <path>
        // <path> is an import path.
        // Attempts import of <path>, returning the imported parcel, if any.
        YmParcel* parcel();

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is an item term.
        // Returns if this check succeeds.
        bool item();

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is a concrete (ie. non-generic) item term.
        // Returns <item> if this check succeeds.
        YmItem* concrete();

        // TODO: generic is currently unused.

        // StkFx: ... <item> -- ... <item>
        // <item> is an item.
        // Checks that <item> is a generic (ie. non-concrete) item term.
        // Returns if this check succeeds.
        bool generic();

        // StkFx: ... -- ... <here>
        // <here> is the path resolved by %here.
        void here();

        // StkFx: ... -- ... <root>
        // <root> is a root import path.
        void root(const std::string& id);

        // StkFx: ... <mod> -- ... <submod>
        // <mod> is a path specifying a module.
        // <submod> is a path to a submodule within <mod> (ie. '<mod>/<id>'.)
        void submod(const std::string& id);

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
        class _Interp final : public SpecEval {
        public:
            _Interp(LoaderState& client);


            void operator()(const std::string& pathOrFullname);


        private:
            ym::Safe<LoaderState> _client;
            const std::string* _pathOrFullnamePtr = nullptr;


            const std::string& _pathOrFullname() const noexcept;


            void syntaxErr() override;
            void rootId(const taul::str& id) override;
            void openArgs() override;
            void closeArgs() override;
            void slashId(const taul::str& id) override;
            void colonId(const taul::str& id) override;
            void dblColonId(const taul::str& id) override;
        };


        ym::Safe<Area> _staging;
        ym::Safe<PathBindings> _binds;

        bool _failureFlag = false;
        Mode _mode = Mode{};
        std::optional<Env> _env;
        std::string _errPrefix;
        std::vector<Term> _terms;

        std::queue<ym::Safe<YmItem>> _lateResolveQueue;


        void _beginLoadOrImport(const std::string& normalizedSpecifier, bool loadNotImport);
        void _endLoadOrImport();

        void _setupForSpecifier(Mode mode, std::optional<Env> env, std::string errPrefix);
        void _setupForImportPath(const std::string& normalizedPath);
        void _setupForFullname(const std::string& normalizedFullname);
        void _setupForRefSym(const std::string& normalizedRefSym, Env env);

        struct _GenItemDataResult final {
            ym::Safe<YmItem> item; // Item either newly init, or existing one that was fetched.
            bool original; // If item was newly init, and thus in need of further setup.
        };
        // This does not perform resolution.
        _GenItemDataResult _genItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            YmItem* owner,
            std::vector<ym::Safe<YmItem>> itemArgs = {});
        // All item data generation originates from non-members, which in turn generate for members.
        ym::Safe<YmItem> _genNonMemberItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            std::vector<ym::Safe<YmItem>> itemArgs = {});
        // Call this prior to early resolution so this info is available for it.
        void _genItemDataForMembers(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            ym::Safe<YmItem> self);
        void _genMemberItemData(
            ym::Safe<YmParcel> parcel,
            ym::Safe<const _ym::ItemInfo> info,
            ym::Safe<YmItem> self);

        const _ym::ItemInfo* _lookupMemberInfo(
            ym::Safe<YmParcel> parcel,
            const _ym::ItemInfo& ownerInfo,
            const std::string& memberName) const;
        static std::string _localNameOfMember(
            const _ym::ItemInfo& ownerInfo,
            const std::string& memberName);

        bool _isEarlyResolveConst(const ConstInfo& constInfo) const;
        void _earlyResolveItem(YmItem& x, ym::Safe<YmItem> self);
        void _earlyResolveConsts(YmItem& x, ym::Safe<YmItem> self);
        void _scheduleLateResolve(YmItem& x);
        void _processLateResolveQueue();
        void _lateResolveItem(YmItem& x);
        void _lateResolveConsts(YmItem& x);
        void _lateResolveRefConst(YmItem& x, ConstIndex index);

        void _checkConstraintTypeLegality();
        void _enforceConstraints();

        YmParcel* _import(const std::string& normalizedPath);

        std::optional<size_t> _countInputsToEndArgs() const noexcept;


        void _printTermStk(size_t oldHeight);
    };
}

