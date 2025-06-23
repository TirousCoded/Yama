

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


    template<typename T>
    concept lookup_proc_of_type =
        requires
    {
        { std::remove_cvref_t<T>::lp } noexcept -> std::convertible_to<lookup_proc>;
    };

    template<lookup_proc_of_type T>
    constexpr lookup_proc lookup_proc_of = std::remove_cvref_t<T>::lp;


    // only named imports get symbols

    struct import_csym final {
        import_path path;


        static constexpr lookup_proc lp = lookup_proc::qualifier;
    };

    struct prim_csym final {
        //


        static constexpr lookup_proc lp = lookup_proc::normal;
    };

    struct var_csym final {
        const ast_TypeSpec* annot_type; // type specified by annotation, if any
        const ast_Expr* initializer; // explicit initializer, if any
        std::optional<size_t> reg; // register index of local var (resolved in second_pass)


        // var_csym is recognized as a 'local var symbol' only once reg has been assigned

        //inline bool is_localvar() const noexcept { return reg.has_value(); }


        std::optional<ctype> get_type(compiler& cs) const; // returns deduced type of the var, if any


        static constexpr lookup_proc lp = lookup_proc::normal;
    };

    // we'll call it 'fn-like' to make clear that it conflates fns and methods

    struct fn_like_csym final {
        struct param {
            qualified_name fn_qn; // used to help get_type w/ self params
            size_t index;
            str name;
            const ast_TypeSpec* type;
            bool is_self_param = false;


            std::optional<ctype> get_type(compiler& cs) const;
        };


        bool is_method = false;
        std::vector<param> params;
        const ast_TypeSpec* return_type;

        // if all control paths (reachable from entrypoint) either reach explicit return
        // stmts, or enter infinite loops

        std::optional<bool> all_paths_return_or_loop;

        // if fn is None returning

        std::optional<bool> is_none_returning;


        std::optional<ctype> get_return_type(compiler& cs) const;
        ctype get_return_type_or_none(translation_unit& tu) const;

        std::string fmt_params(compiler& cs) const;


        static constexpr lookup_proc lp = lookup_proc::normal;
    };

    struct param_csym final {
        fn_like_csym::param* ptr = nullptr;


        bool good() const noexcept;
        fn_like_csym::param& get() const noexcept;
        std::optional<ctype> get_type(compiler& cs) const;


        static constexpr lookup_proc lp = lookup_proc::normal;
    };

    struct struct_csym final {
        //


        static constexpr lookup_proc lp = lookup_proc::normal;
    };


    struct csymtab_entry final {
        using info_t = std::variant<
            import_csym,
            prim_csym,
            var_csym,
            fn_like_csym,
            param_csym,
            struct_csym
        >;


        str                         name;
        const lookup_proc           lp;
        std::shared_ptr<ast_node>   node; // the node corresponding to this entry's declaration, if any
        taul::source_pos            starts; // where in src scope begins (continuing until end of block), should be 0 if has global scope
        info_t                      info;


        template<typename Info>
        inline bool is() const noexcept {
            return std::holds_alternative<Info>(info);
        }
        template<typename Info>
        inline Info& as() noexcept {
            YAMA_ASSERT(is<Info>());
            return std::get<Info>(info);
        }


        inline bool is_type() const noexcept {
            static_assert(std::variant_size_v<info_t> == 6); // reminder
            if (is<import_csym>())          return false;
            else if (is<prim_csym>())       return true;
            else if (is<var_csym>())        return false;
            else if (is<fn_like_csym>())    return true;
            else if (is<param_csym>())      return false;
            else if (is<struct_csym>())     return true;
            else                            YAMA_DEADEND;
            return bool{};
        }

        inline csym_key get_key() const noexcept {
            return csym_key::make(name, lp);
        }


        std::string fmt(compiler& cs, size_t tabs = 0, const char* tab = default_tab);
    };

    // csymtab defines Yama's internal repr for a symbol table
    class csymtab final {
    public:
        csymtab() = default;


        inline size_t count() const noexcept {
            return _entries.size();
        }

        // fails if entry under name already exists
        template<typename Info>
        inline bool insert(const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info) {
            if (_entries.contains(csym_key::make(name, lookup_proc_of<Info>))) {
                return false;
            }
            auto new_entry = csymtab_entry{
                .name = name,
                .lp = lookup_proc_of<Info>,
                .node = node,
                .starts = starts,
                .info = std::forward<Info>(info),
            };
            const auto key = new_entry.get_key(); // <- C++ arg eval order is undefined!
            _entries.try_emplace(key, make_res<csymtab_entry>(std::move(new_entry)));
            return true;
        }

        inline std::shared_ptr<csymtab_entry> fetch(const str& name, lookup_proc lp = lookup_proc::normal) const noexcept {
            const auto it = _entries.find(csym_key::make(name, lp));
            return
                it != _entries.end()
                ? std::shared_ptr<csymtab_entry>(it->second)
                : nullptr;
        }

        std::string fmt(compiler& cs, size_t tabs = 0, const char* tab = default_tab);


    private:
        std::unordered_map<csym_key, res<csymtab_entry>> _entries;
    };

    // csymtab_group maps Chunk/Block AST nodes (by ID) to corresponding csymtab(s)
    class csymtab_group final {
    public:
        csymtab_group() = default;


        inline size_t count() const noexcept {
            return _csymtabs.size();
        }

        // creates new, or acquires existing, symbol table for AST node under id
        inline res<csymtab> acquire(ast_id_t id) {
            auto it = _csymtabs.find(id);
            if (it == _csymtabs.end()) {
                _csymtabs.try_emplace(id, make_res<csymtab>());
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
                ? std::shared_ptr<csymtab>(it->second)
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
        inline std::shared_ptr<csymtab_entry> lookup(const ast_node& x, const str& name, taul::source_pos src_pos, lookup_proc lp = lookup_proc::normal) const noexcept {
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

        // searching from x, then through each ancestor AST node, insert inserts a new symbol
        // table entry into the first AST node found w/ an associated symbol table, if any,
        // returning if successful, failing if no symbol table could be found, or if inserting
        // would conflict w/ an existing symbol under name in that table (if no_table_found is
        // not nullptr, *no_table_found will be set to true if no table was found)
        template<typename Info>
        inline bool insert(ast_node& x, const str& name, std::shared_ptr<ast_node> node, taul::source_pos starts, Info&& info, bool* no_table_found = nullptr) {
            if (const auto table_nd = node_of_inner_most(x)) {
                const auto table = get(table_nd->id);
                YAMA_ASSERT(table);
                return table->insert(name, node, starts, std::forward<Info>(info));
            }
            if (no_table_found) {
                *no_table_found = true;
            }
            return false;
        }

        std::string fmt(compiler& cs, size_t tabs = 0, const char* tab = default_tab);


    private:
        std::unordered_map<ast_id_t, res<csymtab>> _csymtabs;
    };
}

