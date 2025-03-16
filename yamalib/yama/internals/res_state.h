

#pragma once


#include <unordered_map>

#include "../core/general.h"
#include "../core/res.h"
#include "../core/module_info.h"
#include "../core/specifiers.h"

#include "../internals/type_instance.h"


namespace yama::internal {


    template<typename T>
    concept res_area_key = yama::hashable_type<T> && std::equality_comparable<T>;


    template<res_area_key Key, typename T>
    class res_area final {
    public:
        res_area() = default;
        inline res_area(res_area<Key, T>& upstream);


        // these do not traverse upstream resources

        inline auto begin() const noexcept;
        inline auto cbegin() const noexcept { return begin(); }
        inline auto end() const noexcept;
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


        // commit transfers resources from here to upstream
        
        // commit does nothing if there is no upstream area

        inline void commit();


        inline void commit_or_discard(bool commit);


    private:
        res_area<Key, T>* _upstream = nullptr;
        std::unordered_map<Key, res<T>> _data;


        // use this to check for situation where resource was pushed to upstream area
        // such that it conflicts w/ a resource in this one

        inline void _assert_no_key_conflicts_with_upstream() const noexcept;
    };

    template<res_area_key Key, typename T>
    inline res_area<Key, T>::res_area(res_area<Key, T>& upstream)
        : _upstream(&upstream),
        _data() {}

    template<res_area_key Key, typename T>
    inline auto res_area<Key, T>::begin() const noexcept {
        return _data.begin();
    }
    
    template<res_area_key Key, typename T>
    inline auto res_area<Key, T>::end() const noexcept {
        return _data.end();
    }

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
    inline void res_area<Key, T>::commit_or_discard(bool commit) {
        if (commit) this->commit();
        else        discard();
    }

    template<res_area_key Key, typename T>
    inline void res_area<Key, T>::_assert_no_key_conflicts_with_upstream() const noexcept {
        if (_upstream) {
            TAUL_ASSERT([&]() -> bool {
                // choose either *this or *_upstream based on which is smaller, then check
                // if the smaller one's elems are overlapping w/ the *_upstream, w/ this
                // way of doing it being fast as it minimizes iterations
                if (size() < _upstream->size()) {
                    for (const auto& I : *this) {
                        if (_upstream->exists(I.first, true)) return false;
                    }
                }
                else {
                    for (const auto& I : *_upstream) {
                        if (exists(I.first, true)) return false;
                    }
                }
                return true;
                }());
        }
    }


    using type_area = res_area<yama::fullname, type_instance>;
    using module_area = res_area<yama::import_path, module_info>;


    class res_state final {
    public:
        type_area types;
        module_area modules;


        res_state() = default;


        void set_upstream(res_state& upstream) noexcept;
        void set_no_upstream() noexcept;
        void discard() noexcept;
        void commit();
        void commit_or_discard(bool commit);
    };
}

