

#pragma once


#include <memory>
#include <ranges>

#include "general.h"
#include "asserts.h"
#include "concepts.h"
#include "kind.h"
#include "ptype.h"
#include "call_fn.h"
#include "bcode.h"
#include "callsig.h"
#include "const_table.h"
#include "ids.h"

#include "../internals/safeptr.h"
#include "../internals/pack.h"
#include "../internals/index_registry.h"
#include "../internals/desc_map.h"


namespace yama {


    struct item_desc final {
        str name;
        kind kind;
        const_table consts;


        bool operator==(const item_desc&) const noexcept = default;
    };
    struct owner_desc final {
        std::vector<lid_t> members;


        // TODO: is_member hasn't been unit tested.

        inline bool is_member(lid_t id) const noexcept {
            for (const auto& member : members) {
                if (member == id) return true;
            }
            return false;
        }


        bool operator==(const owner_desc&) const noexcept = default;
    };
    struct member_desc final {
        lid_t owner;


        bool operator==(const member_desc&) const noexcept = default;
    };
    struct prim_desc final {
        ptype ptype;


        bool operator==(const prim_desc&) const noexcept = default;
    };
    struct call_desc final {
        callsig callsig;
        size_t max_locals;
        call_fn call_fn;


        bool operator==(const call_desc&) const noexcept = default;
    };
    struct bcode_desc final {
        bc::code bcode;
        bc::syms bsyms = bc::syms{};


        inline bool operator==(const bcode_desc& other) const noexcept {
            // Don't compare bsyms.
            return bcode == other.bcode;
        }
    };

    namespace internal {
        using module_dmap_t = internal::desc_map<lid_t,
            item_desc,
            owner_desc,
            member_desc,
            prim_desc,
            call_desc,
            bcode_desc
        >;
    }


    template<typename T>
    concept descriptor_type =
        internal::desc_type<T> &&
        internal::module_dmap_t::supports<T>;


    class module final {
    public:
        template<descriptor_type... Ts>
        class item_iterator;


    private:
        // _state ensures yama::module::item(_iterator) are not invalidated upon move ctor/assign,
        // at the cost of additional memory indirection for yama::module::* methods.
        struct _state final {
            internal::index_registry<lid_t, str> ir;
            internal::module_dmap_t dmap;
        };


    public:
        // TODO: Below about yama::module move semantics has not been unit tested.

        // Non-owning reference to a particular yama::module item.
        // Updates which yama::module is referred to upon yama::module move ctor/assign.
        class item final {
        public:
            item() = delete;
            item(const item&) = default;
            item(item&&) noexcept = default;
            ~item() noexcept = default;
            item& operator=(const item&) = default;
            item& operator=(item&&) noexcept = default;


            // Returns if item has all descriptors in Ts.
            template<descriptor_type... Ts>
            inline bool all_of() const noexcept {
                return _s->dmap.all_of<Ts...>(id());
            }
            // Returns if item has at least one descriptor in Ts.
            template<descriptor_type... Ts>
            inline bool any_of() const noexcept {
                return _s->dmap.any_of<Ts...>(id());
            }
            // Returns if item has no descriptors in Ts.
            template<descriptor_type... Ts>
            inline bool none_of() const noexcept {
                return _s->dmap.none_of<Ts...>(id());
            }
            // Returns pointers to descriptors Ts for item.
            // Pointers will be nullptr for descriptors not bound.
            template<descriptor_type... Ts>
            inline std::tuple<const Ts*...> try_get() const noexcept {
                return _s->dmap.try_get<Ts...>(id());
            }
            // Returns references to descriptors Ts for item.
            // Throws std::out_of_range if all_of<Ts...>(id()) == false.
            template<descriptor_type... Ts>
            inline std::tuple<const Ts&...> get() const {
                return _s->dmap.get<Ts...>(id());
            }
            // TODO: try_one and one have not been unit tested.

            // Returns a pointer to descriptor T for item, if any.
            template<descriptor_type T>
            inline const T* try_one() const noexcept {
                return std::get<0>(try_get<T>());
            }
            // Returns a reference to descriptor T for item.
            // Throws std::out_of_range if all_of<T>(id()) == false.
            template<descriptor_type T>
            inline const T& one() const {
                return std::get<0>(get<T>());
            }
            // Returns the local ID of the item.
            inline lid_t id() const noexcept {
                return _id;
            }
            // Returns the name of the item.
            inline str name() const noexcept {
                return _s->ir.name(id()).value();
            }

            // Compares by reference.
            bool operator==(const item&) const noexcept = default;

            // TODO: Below methods have not been unit tested.

            // Returns the kind of the item.
            inline kind kind() const noexcept {
                return one<item_desc>().kind;
            }
            // Returns the constant table of the item.
            inline const const_table& consts() const noexcept {
                return one<item_desc>().consts;
            }
            // Returns the members of the item, if any.
            inline std::span<const lid_t> members() const noexcept {
                const auto owner = try_one<owner_desc>();
                return
                    owner
                    ? std::span{ owner->members }
                    : std::span<const lid_t>{};
            }
            // Returns if id is a member of the item.
            inline bool is_member(lid_t id) const noexcept {
                const auto owner = try_one<owner_desc>();
                return
                    owner
                    ? owner->is_member(id)
                    : false;
            }
            // Returns the owner of the item, if any.
            inline std::optional<lid_t> owner() const noexcept {
                const auto member = try_one<member_desc>();
                return
                    member
                    ? std::make_optional(member->owner)
                    : std::nullopt;
            }
            // Returns the ptype of the item, if any.
            inline std::optional<ptype> ptype() const noexcept {
                const auto prim = try_one<prim_desc>();
                return
                    prim
                    ? std::make_optional(prim->ptype)
                    : std::nullopt;
            }
            // Returns the callsig of the item, if any.
            inline const callsig* callsig() const noexcept {
                const auto call = try_one<call_desc>();
                return
                    call
                    ? &call->callsig
                    : nullptr;
            }
            // Returns the max locals of the item, if any.
            // Returns 0 upon failure.
            inline size_t max_locals() const noexcept {
                const auto call = try_one<call_desc>();
                return
                    call
                    ? call->max_locals
                    : 0;
            }
            // Returns the call behaviour function of the item, if any.
            inline call_fn call_fn() const noexcept {
                const auto call = try_one<call_desc>();
                return
                    call
                    ? call->call_fn
                    : nullptr;
            }
            // Returns the bcode of the item, if any.
            inline const bc::code* bcode() const noexcept {
                const auto bcode = try_one<bcode_desc>();
                return
                    bcode
                    ? &bcode->bcode
                    : nullptr;
            }
            // Returns the bsyms of the item, if any.
            inline const bc::syms* bsyms() const noexcept {
                const auto bcode = try_one<bcode_desc>();
                return
                    bcode
                    ? &bcode->bsyms
                    : nullptr;
            }


        private:
            friend class module;
            template<descriptor_type... Ts>
            friend class module::item_iterator;

            
            internal::safeptr<const _state> _s;
            lid_t _id = lid_t(-1);


            inline item(const _state& s, lid_t id)
                : _s(s),
                _id(id) {}
        };

        // TODO: Below about yama::module move semantics has not been unit tested.

        // Updates which yama::module is referred to upon yama::module move ctor/assign.
        template<descriptor_type... Ts>
        class item_iterator final {
        public:
            using value_type = item;
            using pointer = void;
            using reference = value_type;
            using difference_type = ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;


            item_iterator() = default;
            item_iterator(const item_iterator&) = default;
            item_iterator(item_iterator&&) noexcept = default;
            ~item_iterator() noexcept = default;
            item_iterator& operator=(const item_iterator&) = default;
            item_iterator& operator=(item_iterator&&) noexcept = default;


            inline item_iterator& operator++() noexcept {
                std::advance(_it, 1);
                return *this;
            }
            inline item_iterator operator++(int) noexcept {
                auto old = *this;
                ++*this;
                return old;
            }
            // Throws std::out_of_range if past-the-end.
            inline value_type operator*() const {
                return value_type(*_s, std::get<lid_t>(*_it));
            }

            bool operator==(const item_iterator&) const noexcept = default;


        private:
            friend class module;


            const _state* _s = nullptr; // Need to use raw ptr so it's default initializable.
            internal::module_dmap_t::view_iterator<true, Ts...> _it;


            inline item_iterator(const _state& s, decltype(_it) it)
                : _s(&s),
                _it(it) {}
        };


        module() = default;
        inline module(const module& other)
            : _s(_clone_s(other)) {}
        module(module&&) noexcept = default;
        ~module() noexcept = default;
        inline module& operator=(const module& other) {
            _s = _clone_s(other);
            return *this;
        }
        module& operator=(module&&) noexcept = default;


        // Returns number of items.
        uint16_t count() const noexcept;

        // Returns number of items with all descriptors Ts.
        template<descriptor_type... Ts>
        inline uint16_t count_with() const noexcept {
            return _try_get_s() ? _get_s().dmap.count_with<Ts...>() : 0;
        }

        // Returns if item exists.
        bool exists(lid_t id) const noexcept;
        // Returns if item exists.
        bool exists(const str& name) const noexcept;

        // Returns if item has all descriptors in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool all_of(lid_t id) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            return exists(id) ? _get_s().dmap.all_of<Ts...>(id) : false;
        }
        // Returns if item has all descriptors in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool all_of(const str& name) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            const auto _id = id(name);
            return _id ? _get_s().dmap.all_of<Ts...>(*_id) : false;
        }

        // Returns if item has at least one descriptor in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool any_of(lid_t id) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            return exists(id) ? _get_s().dmap.any_of<Ts...>(id) : false;
        }
        // Returns if item has at least one descriptor in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool any_of(const str& name) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            const auto _id = id(name);
            return _id ? _get_s().dmap.any_of<Ts...>(*_id) : false;
        }

        // Returns if item has no descriptors in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool none_of(lid_t id) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            return exists(id) ? _get_s().dmap.none_of<Ts...>(id) : false;
        }
        // Returns if item has no descriptors in Ts.
        // Fails if no item exists.
        template<descriptor_type... Ts>
        inline bool none_of(const str& name) const noexcept {
            if (!_try_get_s()) {
                return false;
            }
            const auto _id = id(name);
            return _id ? _get_s().dmap.none_of<Ts...>(*_id) : false;
        }

        // Returns pointers to descriptors Ts for item.
        // Pointers will be nullptr for descriptors not bound.
        // All pointers will be nullptr if no item exists.
        template<descriptor_type... Ts>
        inline std::tuple<const Ts*...> try_get(lid_t id) const noexcept {
            if (!_try_get_s()) {
                return _empty_tuple<Ts...>();
            }
            return _get_s().dmap.try_get<Ts...>(id);
        }
        // Returns pointers to descriptors Ts for item.
        // Pointers will be nullptr for descriptors not bound.
        // All pointers will be nullptr if no item exists.
        template<descriptor_type... Ts>
        inline std::tuple<const Ts*...> try_get(const str& name) const noexcept {
            if (!_try_get_s()) {
                return _empty_tuple<Ts...>();
            }
            const auto _id = id(name);
            return _id ? _get_s().dmap.try_get<Ts...>(*_id) : _empty_tuple<Ts...>();
        }

        // Returns references to descriptors Ts for item.
        // Throws std::invalid_argument if no item exists.
        // Throws std::out_of_range if all_of<Ts...>(id) == false.
        template<descriptor_type... Ts>
        inline std::tuple<const Ts&...> get(lid_t id) const {
            if (!_try_get_s()) {
                throw std::invalid_argument("Item not found!");
            }
            if (!exists(id)) {
                throw std::invalid_argument("Item not found!");
            }
            return _get_s().dmap.get<Ts...>(id);
        }
        // Returns references to descriptors Ts for item.
        // Throws std::invalid_argument if no item exists.
        // Throws std::out_of_range if all_of<Ts...>(name) == false.
        template<descriptor_type... Ts>
        inline std::tuple<const Ts&...> get(const str& name) const {
            if (!_try_get_s()) {
                throw std::invalid_argument("Item not found!");
            }
            const auto _id = id(name);
            if (!_id) {
                throw std::invalid_argument("Item not found!");
            }
            return _get_s().dmap.get<Ts...>(*_id);
        }

        // Returns a non-owning reference to an item, if any.
        // Fails if all_of<Ts...>(id) == false.
        template<descriptor_type... Ts>
        inline std::optional<item> get_item(lid_t id) const noexcept {
            if (!_try_get_s()) {
                return std::nullopt;
            }
            if (_get_s().ir.exists(id)) {
                return item(_get_s(), id);
            }
            return std::nullopt;
        }
        // Returns a non-owning reference to an item, if any.
        // Fails if all_of<Ts...>(name) == false.
        template<descriptor_type... Ts>
        inline std::optional<item> get_item(const str& name) const noexcept {
            if (!_try_get_s()) {
                return std::nullopt;
            }
            if (const auto id = _get_s().ir.index(name)) {
                return item(_get_s(), *id);
            }
            return std::nullopt;
        }

        // Returns a view over all items with descriptors Ts, if any.
        template<descriptor_type... Ts>
        inline auto view() const noexcept {
            if (!_try_get_s()) {
                return internal::module_dmap_t::empty_view<true, Ts...>();
            }
            return _get_s().dmap.view<Ts...>();
        }

        // Returns a view over all items with descriptors Ts, if any.
        template<descriptor_type... Ts>
        inline auto view_items() const noexcept {
            using Iter = item_iterator<Ts...>;
            if (!_try_get_s()) {
                return std::ranges::subrange<Iter>{};
            }
            const auto view_ = view<Ts...>();
            return std::ranges::subrange<Iter>(Iter(_get_s(), view_.begin()), Iter(_get_s(), view_.end()));
        }

        // Returns the name of item under id, if any.
        std::optional<str> name(lid_t id) const noexcept;

        // Returns ID of item under name, if any.
        std::optional<lid_t> id(const str& name) const noexcept;

        // TODO: Equality compare has not been unit tested.

        // Compares by value.
        bool operator==(const module& other) const noexcept;

        // TODO: Haven't really unit tested failing due to exceeding ID limit.
        // TODO: Haven't added, let alone unit tested, if charset used in name/owner_name/member_name
        //       passed to below is valid.
        // TODO: What is below methods' exception safety policy? Is it unit tested?
        //          * also for yama::module::item::*

        static_assert(kinds == 4);

        // Adds a new primitive type item, returning if successful.
        // Fails if name is already taken.
        // Fails if new item's ID would exceed max item ID.
        bool add_primitive(
            const str& name,
            const_table consts,
            ptype ptype);

        // Adds a new function type item, returning if successful.
        // Fails if name is already taken.
        // Fails if new item's ID would exceed max item ID.
        bool add_function(
            const str& name,
            const_table consts,
            callsig callsig,
            size_t max_locals,
            call_fn call_fn);

        // Adds a new method type item, returning if successful.
        // Fails if name is already taken.
        // Fails if new item's ID would exceed max item ID.
        bool add_method(
            const str& owner_name,
            const str& member_name,
            const_table consts,
            callsig callsig,
            size_t max_locals,
            call_fn call_fn);

        // Adds a new struct type item, returning if successful.
        // Fails if name is already taken.
        // Fails if new item's ID would exceed max item ID.
        bool add_struct(
            const str& name,
            const_table consts);

        // Binds a bcode descriptor to an item, returning if successful.
        // Fails if no item exists.
        // Fails if bcode descriptor is already bound.
        bool bind_bcode(
            lid_t id,
            bc::code bcode,
            bc::syms bsyms = bc::syms{});
        // Binds a bcode descriptor to an item, returning if successful.
        // Fails if no item exists.
        // Fails if bcode descriptor is already bound.
        bool bind_bcode(
            const str& name,
            bc::code bcode,
            bc::syms bsyms = bc::syms{});


    private:
        // _s heap alloc is lazy to keep default init of empty yama::module(s) cheap.
        std::unique_ptr<_state> _s;


        inline _state* _try_get_s() noexcept { return _s.get(); }
        inline const _state* _try_get_s() const noexcept { return _s.get(); }
        inline _state& _get_s() noexcept { return deref_assert(_try_get_s()); }
        inline const _state& _get_s() const noexcept { return deref_assert(_try_get_s()); }
        inline _state& _load_s() {
            if (!_s) _s = std::make_unique<_state>();
            return _get_s();
        }

        void _handle_if_owner_already_exists(lid_t member, const str& owner_name);
        void _handle_if_members_already_exist(lid_t owner);


        static str _member_name(const str& owner, const str& member);
        template<descriptor_type... Ts>
        static constexpr auto _empty_tuple() noexcept {
            return std::make_tuple((const Ts*)nullptr...);
        }
        static std::unique_ptr<_state> _clone_s(const module& other);
    };
}

