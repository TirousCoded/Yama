

#pragma once


#include <unordered_map>

#include "ast.h"
#include "ctypesys.h"


namespace yama::internal {


    // IMPORTANT: symbols are mutable so that during compilation their state can be ammended
    //            w/ new info if it wasn't available previously


    struct prim_csym final {
        //
    };
    struct var_csym final {
        const ast_TypeSpec* annot_type; // type specified by annotation, if any
        std::optional<ctype> deduced_type; // type deduced from initializer, if any (resolved in second_pass)
        std::optional<size_t> reg; // register index of local var (resolved in second_pass)


        // var_csym is recognized as a 'local var symbol' only once reg has been assigned

        inline bool is_localvar() const noexcept { return reg.has_value(); }
    };
    struct fn_csym final {
        struct param final {
            str name;
            const ast_TypeSpec* type;
        };


        std::vector<param> params;
        const ast_TypeSpec* return_type;

        // if all control paths (reachable from entrypoint) either reach explicit return
        // stmts, or enter infinite loops

        std::optional<bool> all_paths_return_or_loop;

        // if fn is None returning

        std::optional<bool> is_none_returning;


        std::string fmt_params(const taul::source_code& src) const;
    };
    struct param_csym final {
        const ast_TypeSpec* type;
    };

    struct csymtab_entry final {
        using info_t = std::variant<
            prim_csym,
            var_csym,
            fn_csym,
            param_csym
        >;


        str                         name;
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
            static_assert(std::variant_size_v<info_t> == 4); // reminder
            return
                is<prim_csym>() ||
                is<fn_csym>();
        }


        std::string fmt(const taul::source_code& src, size_t tabs = 0, const char* tab = "    ");
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
            if (_entries.contains(name)) {
                return false;
            }
            auto new_entry = csymtab_entry{
                .name = name,
                .node = node,
                .starts = starts,
                .info = std::forward<Info>(info),
            };
            _entries.try_emplace(name, make_res<csymtab_entry>(std::move(new_entry)));
            return true;
        }

        inline std::shared_ptr<csymtab_entry> fetch(const str& name) const noexcept {
            const auto it = _entries.find(name);
            return
                it != _entries.end()
                ? std::shared_ptr<csymtab_entry>(it->second)
                : nullptr;
        }

        std::string fmt(const taul::source_code& src, size_t tabs = 0, const char* tab = "    ");


    private:
        std::unordered_map<str, res<csymtab_entry>> _entries;
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
            else if (const auto upstream = x.parent.lock()) {
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
        inline std::shared_ptr<csymtab_entry> lookup(const ast_node& x, const str& name, taul::source_pos src_pos) const noexcept {
            // IMPORTANT: we CAN'T use node_of_inner_most here, as we care about visibility throughout all csymtabs relative to x
            // below code will recursively skip past interim parent nodes w/out symbol tables
            if (const auto table = get(x.id)) {
                if (const auto result = table->fetch(name)) {
                    if (src_pos >= result->starts) {
                        return result;
                    }
                }
            }
            const auto upstream = x.parent.lock();
            return
                upstream
                ? lookup(*upstream, name, src_pos)
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

        std::string fmt(const taul::source_code& src, size_t tabs = 0, const char* tab = "    ");


    private:
        std::unordered_map<ast_id_t, res<csymtab>> _csymtabs;
    };
}

