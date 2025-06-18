

#pragma once


#include "../core/api_component.h"
#include "../core/type.h"

#include "safeptr.h"
#include "res_state.h"


#define _YAMA_DUMP_LOADER_LOG 0


namespace yama::internal {


    class domain_data;


    class loader final : public api_component {
    public:
        res_state state;


        loader(std::shared_ptr<debug> dbg, domain_data& dd, res_state& upstream);


        std::optional<yama::type> load(const str& fullname);

        // IMPORTANT: see res_area::commit comment in res_state.h for info on protects_upstream

        // this MUST be called IMMEDIATELY after a call to load finishes in order to cleanup
        // properly and, if successful, commit state upstream

        void commit_or_discard();
        void commit_or_discard(std::shared_mutex& protects_upstream);


    private:
        safeptr<domain_data> _dd;

        bool _last_was_success = false;


        // locks _dd->new_data_mtx upon detecting need to generate new data, releasing lock
        // during commit_or_discard call

        std::atomic_bool _holds_lock = false;

        void _lock();
        void _unlock();


        std::optional<fullname> _resolve_fullname(const str& fullname);

        std::optional<type> _pull_type(const fullname& fullname) const noexcept;

        bool _load(const fullname& fullname);
        bool _check_already_loaded(const fullname& fullname) const;
        bool _add_type(const fullname& fullname);

        std::shared_ptr<type_info> _acquire_type_info(const fullname& fullname);
        bool _create_and_link_instance(const fullname& fullname, res<type_info> info);
        res<type_instance> _create_instance(const fullname& fullname, res<type_info> info);
        void _add_instance_to_state(const fullname& fullname, res<type_instance> instance);

        // herein, 'resolving' refers to the act of linking new types in the batch, w/
        // new types being added recursively as needed

        // resolving does not check the validity of links (ie. whether the type linked
        // against a given type constant symbol is reasonable to do so w/)
        
        bool _resolve_consts(const env& e, type_instance& instance, res<type_info> info);
        bool _resolve_const(const env& e, type_instance& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _resolve_scalar_const(type_instance& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _resolve_type_const(const env& e, type_instance& instance, res<type_info> info, const_t index);

        // herein, 'checking' refers to the act of checking to see whether or not the
        // type constant links established during resolving are reasonable

        // resolving is performed first, and fully, to ensure that there's no ambiguity
        // as to whether checking will have access to any given piece of linkage info
        // (ie. it will never encounter partially linked type instances)

        bool _check();
        bool _check_instance(const env& e, type_instance& instance);
        bool _check_consts(const env& e, type_instance& instance, res<type_info> info);
        bool _check_const(const env& e, type_instance& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _check_scalar_const(type_instance& instance, res<type_info> info, const_t index);
        template<const_type C>
        inline bool _check_type_const(const env& e, type_instance& instance, res<type_info> info, const_t index);
        bool _check_no_kind_mismatch(type_instance& instance, res<type_info> info, const_t index, const type& resolved);
        bool _check_no_callsig_mismatch(type_instance& instance, res<type_info> info, const_t index, const type& resolved);


        str _str_fullname(const fullname& fullname) const;
        std::string _fmt_fullname(const fullname& fullname) const;
    };

    template<const_type C>
    inline bool loader::_resolve_scalar_const(type_instance& instance, res<type_info> info, const_t index) {
        if (const auto x = info->consts().get<C>(index)) {
            instance.put<C>(index, x->v);
        }
        else YAMA_DEADEND;
        return true;
    }

    template<const_type C>
    inline bool loader::_resolve_type_const(const env& e, type_instance& instance, res<type_info> info, const_t index) {
        // TODO: when we add *incomplete type cloning*-using stuff, this
        //       method may need to be revised
        YAMA_ASSERT(create_type(instance).consts().is_stub(index));
        const fullname fullname = fullname::parse(e, info->consts().qualified_name(index).value()).value();
        if (!_add_type(fullname)) {
            return false;
        }
        const type resolved = _pull_type(fullname).value();
#if _YAMA_DUMP_LOADER_LOG
        std::cerr << std::format(">> {} type const. {} => {}\n", instance.fullname(), index, resolved);
#endif
        instance.put<C>(index, resolved);
        return true;
    }

    template<const_type C>
    inline bool loader::_check_scalar_const(type_instance& instance, res<type_info> info, const_t index) {
        return true; // nothing to check
    }
    
    template<const_type C>
    inline bool loader::_check_type_const(const env& e, type_instance& instance, res<type_info> info, const_t index) {
        YAMA_ASSERT(!create_type(instance).consts().is_stub(index));
        const fullname fullname = fullname::parse(e, info->consts().qualified_name(index).value()).value();
        const type resolved = _pull_type(fullname).value();
        return
            _check_no_kind_mismatch(instance, info, index, resolved) &&
            _check_no_callsig_mismatch(instance, info, index, resolved);
    }
}

