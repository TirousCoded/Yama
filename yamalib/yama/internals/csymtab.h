

#pragma once


#include <unordered_map>

#include "ast.h"
#include "ctypesys.h"


namespace yama::internal {


    class compiler;
    class translation_unit;


    // IMPORTANT: symbols are mutable so that during compilation their state can be ammended
    //            w/ new info if it wasn't available previously


    enum class lookup_proc : uint8_t {
        normal,
        qualifier,
    };

    std::string fmt_lookup_proc(lookup_proc x);


    // symbols are looked up via a name and lookup procedure

    struct csym_key final {
        str name;
        lookup_proc lp;


        bool operator==(const csym_key&) const noexcept = default;
        inline size_t hash() const noexcept { return taul::hash(name, lp); }


        static inline csym_key make(str name, lookup_proc lp) noexcept { return { .name = std::move(name), .lp = lp }; }
    };
}

YAMA_SETUP_HASH(yama::internal::csym_key, x.hash());

namespace yama::internal {


    // forward decls

    class csym;
    class import_csym;
    class prim_csym;
    class var_csym;
    class fn_like_csym;
    class param_csym;
    class struct_csym;


    enum class csym_category : uint8_t {
        import,
        prim,
        var,
        fn_like,
        param,
        struct0,

        num, // not a valid csym_category
    };

    constexpr size_t csym_categories = size_t(csym_category::num);

    inline std::string fmt_csym_category(csym_category x) {
        static_assert(csym_categories == 6);
        switch (x) {
        case csym_category::import:     return "import";
        case csym_category::prim:       return "prim";
        case csym_category::var:        return "var";
        case csym_category::fn_like:    return "fn-like";
        case csym_category::param:      return "param";
        case csym_category::struct0:    return "struct";
        default: YAMA_DEADEND; break;
        }
        return {};
    }


    template<typename T>
    struct csym_traits final {};
    template<>
    struct csym_traits<import_csym> final {
        static constexpr auto category = csym_category::import;
        static constexpr auto lp = lookup_proc::qualifier;
        static constexpr bool is_type = false;
    };
    template<>
    struct csym_traits<prim_csym> final {
        static constexpr auto category = csym_category::prim;
        static constexpr auto lp = lookup_proc::normal;
        static constexpr bool is_type = true;
    };
    template<>
    struct csym_traits<var_csym> final {
        static constexpr auto category = csym_category::var;
        static constexpr auto lp = lookup_proc::normal;
        static constexpr bool is_type = false;
    };
    template<>
    struct csym_traits<fn_like_csym> final {
        static constexpr auto category = csym_category::fn_like;
        static constexpr auto lp = lookup_proc::normal;
        static constexpr bool is_type = true;
    };
    template<>
    struct csym_traits<param_csym> final {
        static constexpr auto category = csym_category::param;
        static constexpr auto lp = lookup_proc::normal;
        static constexpr bool is_type = false;
    };
    template<>
    struct csym_traits<struct_csym> final {
        static constexpr auto category = csym_category::struct0;
        static constexpr auto lp = lookup_proc::normal;
        static constexpr bool is_type = true;
    };


    template<typename T>
    concept with_csym_traits =
        requires
    {
        { csym_traits<T>::category } noexcept -> std::convertible_to<csym_category>;
        { csym_traits<T>::lp } noexcept -> std::convertible_to<lookup_proc>;
        { csym_traits<T>::is_type } noexcept -> std::convertible_to<bool>;
    };
    template<typename T>
    concept csym_type =
        std::derived_from<T, csym> &&
        with_csym_traits<T>;


    class csym : public std::enable_shared_from_this<csym> {
    public:
        const csym_category             category;
        const lookup_proc               lp;
        const bool                      is_type;
        const str                       name;
        const std::shared_ptr<ast_node> node;       // the node corresponding to this entry's declaration, if any
        const taul::source_pos          starts;     // where in src scope begins (continuing until end of block), should be 0 if has global scope
        const safeptr<translation_unit> tu;
        
        
        inline csym(
            csym_category category,
            lookup_proc lp,
            bool is_type,
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu)
            : category(category),
            lp(lp),
            is_type(is_type),
            name(name),
            node(node),
            starts(starts),
            tu(tu) {}

        virtual ~csym() noexcept = default;


        // below 'is', 'as' and 'expect' methods let us use custom RTTI to discern types

        inline bool is(csym_category x) const noexcept {
            return category == x;
        }
        template<csym_type T>
        inline bool is() const noexcept {
            return is(csym_traits<T>::category);
        }
        template<csym_type T>
        inline std::shared_ptr<T> as() noexcept {
            return
                is<T>()
                ? std::static_pointer_cast<T>(shared_from_this())
                : nullptr;
        }
        template<csym_type T>
        inline std::shared_ptr<const T> as() const noexcept {
            return
                is<const T>()
                ? std::static_pointer_cast<const T>(shared_from_this())
                : nullptr;
        }
        template<csym_type T>
        inline res<T> expect() {
            return res(as<T>());
        }
        template<csym_type T>
        inline res<const T> expect() const {
            return res(as<T>());
        }


        inline csym_key get_key() const noexcept {
            return csym_key::make(name, lp);
        }


        virtual std::string fmt(size_t tabs = 0, const char* tab = default_tab) = 0;
    };


    // helper

    template<with_csym_traits T>
    class csym_base : public csym {
    public:
        using Traits = csym_traits<T>;


        inline csym_base(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu)
            : csym(Traits::category, Traits::lp, Traits::is_type, name, node, starts, tu) {}
    };


    // IMPORTANT: only named imports get symbols

    class import_csym final : public csym_base<import_csym> {
    public:
        import_path path;


        inline import_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu,
            import_path path)
            : csym_base(name, node, starts, tu),
            path(std::move(path)) {}


        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };

    class prim_csym final : public csym_base<prim_csym> {
    public:
        //


        inline prim_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu)
            : csym_base(name, node, starts, tu) {}


        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };

    class var_csym final : public csym_base<var_csym> {
    public:
        const ast_TypeSpec* annot_type = nullptr; // type specified by annotation, if any
        const ast_Expr* initializer = nullptr; // explicit initializer, if any
        std::optional<size_t> reg; // register index of local var (resolved in second_pass)


        inline var_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu,
            const ast_TypeSpec* annot_type,
            const ast_Expr* initializer)
            : csym_base(name, node, starts, tu),
            annot_type(annot_type),
            initializer(initializer) {}


        std::optional<ctype> get_type() const; // returns deduced type of the var, if any


        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };

    // we'll call it 'fn-like' to make clear that it conflates fns and methods

    class fn_like_csym final : public csym_base<fn_like_csym> {
    public:
        bool is_method = false;
        std::vector<safeptr<param_csym>> params;
        const ast_TypeSpec* return_type = nullptr;

        // if all control paths (reachable from entrypoint) either reach explicit return
        // stmts, or enter infinite loops

        std::optional<bool> all_paths_return_or_loop;

        // if fn is None returning

        std::optional<bool> is_none_returning;


        inline fn_like_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu,
            bool is_method,
            const ast_TypeSpec* return_type)
            : csym_base(name, node, starts, tu),
            is_method(is_method),
            return_type(return_type) {}


        fullname get_owner_fln() const;

        std::optional<ctype> get_owner_type() const;
        std::optional<ctype> get_return_type() const;
        ctype get_return_type_or_none() const;

        std::string fmt_params() const;
        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };

    class param_csym final : public csym_base<param_csym> {
    public:
        const fn_like_csym* fn = nullptr; // for get_type w/ self params
        size_t index;
        const ast_TypeSpec* type = nullptr;
        bool self_param = false;


        inline param_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu,
            const fn_like_csym* fn,
            size_t index,
            const ast_TypeSpec* type,
            bool self_param)
            : csym_base(name, node, starts, tu),
            fn(fn),
            index(index),
            type(type),
            self_param(self_param) {}


        std::optional<ctype> get_type() const;


        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };

    class struct_csym final : public csym_base<struct_csym> {
    public:
        //


        inline struct_csym(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            safeptr<translation_unit> tu)
            : csym_base(name, node, starts, tu) {}


        std::string fmt(size_t tabs = 0, const char* tab = default_tab) override;
    };


    // csymtab defines Yama's internal repr for a symbol table
    class csymtab final {
    public:
        const safeptr<translation_unit> tu;


        inline csymtab(safeptr<translation_unit> tu)
            : tu(tu) {}


        inline size_t count() const noexcept {
            return _entries.size();
        }

        inline std::shared_ptr<csym> fetch(const str& name, lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto it = _entries.find(csym_key::make(name, lp));
            return
                it != _entries.end()
                ? it->second.base()
                : nullptr;
        }
        template<csym_type T>
        inline std::shared_ptr<T> fetch_as(const str& name, lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto result = fetch(name, lp);
            return
                (bool)result
                ? result->as<T>()
                : nullptr;
        }
        template<csym_type T>
        inline std::shared_ptr<T> fetch_expect(const str& name, lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto result = fetch(name, lp);
            return
                (bool)result
                ? result->expect<T>().base()
                : nullptr;
        }

        // fails if entry under name already exists
        template<csym_type T, typename... Args>
        inline std::shared_ptr<T> insert(
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            Args&&... args) {
            using Traits = csym_traits<T>;
            const auto key = csym_key::make(name, Traits::lp);
            if (_entries.contains(key)) {
                return nullptr;
            }
            const auto result = make_res<T>(name, node, starts, tu, std::forward<Args>(args)...);
            _entries.insert({ key, result });
            return result;
        }

        std::string fmt(size_t tabs = 0, const char* tab = default_tab);


    private:
        std::unordered_map<csym_key, res<csym>> _entries;
    };

    // csymtab_group maps Chunk/Block AST nodes (by ID) to corresponding csymtab(s)
    class csymtab_group final {
    public:
        const safeptr<translation_unit> tu;


        inline csymtab_group(safeptr<translation_unit> tu)
            : tu(tu) {}


        inline size_t count() const noexcept {
            return _csymtabs.size();
        }

        // creates new, or acquires existing, symbol table for AST node under id
        inline res<csymtab> acquire(ast_id_t id) {
            auto it = _csymtabs.find(id);
            if (it == _csymtabs.end()) {
                _csymtabs.insert({ id, make_res<csymtab>(tu) });
                it = _csymtabs.find(id);
            }
            YAMA_ASSERT(it != _csymtabs.end());
            return it->second;
        }

        // fetches existing symbol table for AST node under id, if any
        inline std::shared_ptr<csymtab> get(ast_id_t id) const noexcept {
            const auto it = _csymtabs.find(id);
            return
                it != _csymtabs.end()
                ? it->second.base()
                : nullptr;
        }

        // searching from x, then through each ancestor AST node, node_of_inner_most returns
        // the node corresponding to the inner-most nested symbol table relative to x, if any,
        // including if x itself has a symbol table
        inline std::shared_ptr<ast_node> node_of_inner_most(ast_node& x) const noexcept {
            // recursively search for inner-most nested csymtab relative to x
            if (const auto table = get(x.id)) {
                return x.shared_from_this();
            }
            // below code will recursively skip past interim parent nodes w/out symbol tables
            else if (const auto upstream = x.try_parent()) {
                return node_of_inner_most(*upstream);
            }
            else {
                return nullptr;
            }
        }

        // performs a search of the symbol table for a symbol under name, if any, starting
        // from the symbol table of x, if any, and propagating to upstream symbol tables
        // of ancestor AST nodes of x (note how these rules let x be nodes w/out symbol tables),
        // w/ symbols which haven't entered scope by src_pos not being visible
        inline std::shared_ptr<csym> lookup(
            const ast_node& x,
            const str& name,
            taul::source_pos src_pos,
            lookup_proc lp = lookup_proc::normal) const noexcept {
            // IMPORTANT: we CAN'T use node_of_inner_most here, as we care about visibility throughout all csymtabs relative to x
            // below code will recursively skip past interim parent nodes w/out symbol tables
            if (const auto table = get(x.id)) {
                if (const auto result = table->fetch(name, lp)) {
                    if (src_pos >= result->starts) {
                        return result;
                    }
                }
            }
            const auto upstream = x.try_parent();
            return
                upstream
                ? lookup(*upstream, name, src_pos, lp)
                : nullptr;
        }

        template<csym_type T>
        inline std::shared_ptr<T> lookup_as(
            const ast_node& x,
            const str& name,
            taul::source_pos src_pos,
            lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto result = lookup(x, name, src_pos, lp);
            return
                (bool)result
                ? result->as<T>()
                : nullptr;
        }
        template<csym_type T>
        inline std::shared_ptr<T> lookup_expect(
            const ast_node& x,
            const str& name,
            taul::source_pos src_pos,
            lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto result = lookup(x, name, src_pos, lp);
            return
                (bool)result
                ? result->expect<T>().base()
                : nullptr;
        }

        // searching from x, then through each ancestor AST node, insert inserts a new symbol table
        // entry into the first AST node found w/ an associated symbol table, if any, returning
        // new entry if successful, failing if no symbol table could be found, or if inserting
        // would conflict w/ an existing symbol under name in that table (if no_table_found
        // is not nullptr, *no_table_found will be set to true if no table was found)
        template<csym_type T, typename... Args>
        inline std::shared_ptr<T> insert(
            ast_node& x,
            bool* no_table_found,
            const str& name,
            const std::shared_ptr<ast_node>& node,
            taul::source_pos starts,
            Args&&... args) {
            if (const auto table_nd = node_of_inner_most(x)) {
                const auto table = get(table_nd->id);
                YAMA_ASSERT(table);
                return table->insert<T>(name, node, starts, std::forward<Args>(args)...);
            }
            if (no_table_found) {
                *no_table_found = true;
            }
            return nullptr;
        }

        std::string fmt(size_t tabs = 0, const char* tab = default_tab);


    private:
        std::unordered_map<ast_id_t, res<csymtab>> _csymtabs;
    };
}

