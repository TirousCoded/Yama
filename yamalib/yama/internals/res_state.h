

#pragma once


#include <unordered_map>
#include <shared_mutex>

#include "../core/general.h"
#include "../core/concepts.h"
#include "../core/res.h"
#include "../core/ids.h"
#include "../core/module.h"

#include "specifiers.h"
#include "type_instance.h"
#include "imported_module.h"


namespace yama::internal {


    template<typename T>
    concept res_area_key = yama::hashable_type<T> && std::equality_comparable<T>;


    template<res_area_key Key, typename T>
    class res_area final {
    public:
        res_area() = default;
        inline res_area(res_area<Key, T>& upstream);


        inline auto begin() const noexcept { return _data.begin(); }
        inline auto cbegin() const noexcept { return begin(); }
        inline auto end() const noexcept { return _data.end(); }
        inline auto cend() const noexcept { return end(); }


        // set_(no_)upstream sets which area is upstream of this one, if any

        inline void set_upstream(res_area<Key, T>& upstream) noexcept;
        inline void set_no_upstream() noexcept;


        // size returns the number of resources in the area

        inline size_t size() const noexcept;


        // exists checks if a resource exists in the area under key

        // exists includes checking upstream (unless local_only == true)

        inline bool exists(const Key& key, bool local_only = false) const noexcept;


        // NOTE: pull is non-noexcept as copying T could throw

        // pull attempts to pull a copy of the resource under key, if any

        // pull includes pulling from upstream (unless local_only == true)

        inline std::shared_ptr<T> pull(const Key& key, bool local_only = false) const;


        // push attempts to push x under key to the area, returning if successful

        // push fails if an existing resource exists under key already, w/ this
        // including it checking upstream

        // behaviour is undefined if push causes a resource to be added under a
        // key taken by a resource pushed to a downstream area

        inline bool push(const Key& key, res<T> x);


        // discard resets local area state, but upstream is not reset

        inline void discard() noexcept;


        // IMPORTANT: protects_upstream tells fn to *exclusive lock* it before commit
        //
        //            beyond this, ALL sync is expected to be handled by caller
        //
        //            also, remember to NOT be holding inclusive lock when using these
        //            overloads

        // commit transfers resources from here to upstream
        
        // commit does nothing if there is no upstream area

        inline void commit();
        inline void commit(std::shared_mutex& protects_upstream);


        inline void commit_or_discard(bool commit);
        inline void commit_or_discard(bool commit, std::shared_mutex& protects_upstream);


    private:
        res_area<Key, T>* _upstream = nullptr;
        std::unordered_map<Key, res<T>> _data;


        // use this to check for situation where resource was pushed to upstream area
        // such that it conflicts w/ a resource in this one

        inline void _assert_no_key_conflicts_with_upstream() const noexcept;
    };

    template<res_area_key Key, typename T>
    inline res_area<Key, T>::res_area(res_area<Key, T>& upstream)
        : _upstream(&upstream) {}

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::set_upstream(res_area<Key, T>& upstream) noexcept {
        YAMA_ASSERT(this != &upstream);
        _upstream = &upstream;
        _assert_no_key_conflicts_with_upstream();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::set_no_upstream() noexcept {
        _upstream = nullptr;
    }
    
    template<res_area_key Key, typename T>
    inline size_t res_area<Key, T>::size() const noexcept {
        return _data.size();
    }

    template<res_area_key Key, typename T>
    inline bool res_area<Key, T>::exists(const Key& key, bool local_only) const noexcept {
        return
            _upstream && !local_only
            ? _upstream->exists(key) || _data.contains(key)
            : _data.contains(key);
    }
    
    template<res_area_key Key, typename T>
    inline std::shared_ptr<T> res_area<Key, T>::pull(const Key& key, bool local_only) const {
        if (_upstream && !local_only) {
            if (const auto result = _upstream->pull(key)) {
                return result;
            }
        }
        const auto found = _data.find(key);
        return
            found != _data.end()
            ? found->second.base()
            : nullptr;
    }
    
    template<res_area_key Key, typename T>
    inline bool res_area<Key, T>::push(const Key& key, res<T> x) {
        if (exists(key)) return false;
        _data.insert({ key, x });
        return true;
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::discard() noexcept {
        _data.clear();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::commit() {
        if (_upstream) {
            _assert_no_key_conflicts_with_upstream();
            _upstream->_data.merge(_data);
        }
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::commit(std::shared_mutex& protects_upstream) {
        std::unique_lock lk(protects_upstream);
        commit();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::commit_or_discard(bool commit) {
        if (commit) this->commit();
        else        discard();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::commit_or_discard(bool commit, std::shared_mutex& protects_upstream) {
        if (commit) this->commit(protects_upstream);
        else        discard();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::_assert_no_key_conflicts_with_upstream() const noexcept {
        TAUL_ASSERT([&]() -> bool {
            if (_upstream) {
                for (const auto& [key, value] : _data) {
                    if (_upstream->exists(key, true)) return false;
                }
            }
            return true;
            }());
    }


    // TODO: I'm really conflicted as to whether below should use fullname/import_path as
    //       keys, or if they should just use yama::str (describing the fullname/import path
    //       in domain env)
    //
    //       I dislike using non-str for key as fullname may later require heap alloc to
    //       be properly parsed, and that means having complex parsing and heap allocs upon
    //       end-user trying to load something (even if load would otherwise be *trivial*)

    // TODO: if we do try and make below use yama::str as key, please take into consideration
    //       that we'll run into an issue in ~::loader::_check where we'll not have a clear
    //       way of discerning the env to use to parse the fullname str

    using type_area = res_area<fullname, type_instance>;
    using module_area = res_area<import_path, imported_module>;


    class res_state final {
    public:
        type_area types;
        module_area modules;


        res_state() = default;


        void set_upstream(res_state& upstream) noexcept;
        void set_no_upstream() noexcept;
        void discard() noexcept;
        void commit();
        void commit(std::shared_mutex& protects_upstream);
        void commit_or_discard(bool commit);
        void commit_or_discard(bool commit, std::shared_mutex& protects_upstream);
    };
}

