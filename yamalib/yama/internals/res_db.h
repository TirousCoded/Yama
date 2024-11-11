

#pragma once


#include <optional>
#include <unordered_map>

#include "fullnamed_type.h"


namespace yama::internal {


    // res_db encapsulates a database of mappings of 'fullname' string
    // names to resources (of types compatible w/ res_db)

    template<fullnamed_type T>
    class res_db final {
    public:

        res_db() = default;
        inline res_db(res_db<T>& upstream);


        // these do not traverse upstream resources

        inline auto begin() const noexcept;
        inline auto cbegin() const noexcept { return begin(); }
        inline auto end() const noexcept;
        inline auto cend() const noexcept { return end(); }


        // size returns the number of resources in the database

        // size doesn't include upstream resources

        inline size_t size() const noexcept;


        // exists checks if a resource exists in the database
        // under fullname

        // exists includes checking upstream (unless local_only == true)

        inline bool exists(const str& fullname, bool local_only = false) const noexcept;


        // NOTE: pull is non-noexcept as copying T could throw

        // pull attempts to pull a copy of the resource under 
        // fullname, if any

        // pull includes pulling from upstream (unless local_only == true)

        inline std::optional<T> pull(const str& fullname, bool local_only = false) const;


        // push attempts to push a new resource to the database,
        // returning if successful

        // push will fail if yama::dm::fullname_of(new_res) is 
        // already taken by another resource

        inline bool push(T new_res);


        // reset resets database state

        // upstream resources are not reset

        inline void reset() noexcept;


        // transfer attempts to transfer the resources of this
        // database to target, removing them from this database,
        // returning if successful

        // transfer will fail if there is any fullname overlap
        // between resources of *this and target

        inline bool transfer(res_db<T>& target);


        // commit transfers resources from here to upstream,
        // doing nothing if there is no upstream

        inline bool commit();


    private:

        std::unordered_map<str, T> _db;
        res_db<T>* _upstream = nullptr;


        inline bool _overlaps(const res_db<T>& other) const noexcept;
    };


    template<fullnamed_type T>
    inline res_db<T>::res_db(res_db<T>& upstream)
        : _upstream(&upstream) {}

    template<fullnamed_type T>
    inline auto res_db<T>::begin() const noexcept {
        return _db.begin();
    }

    template<fullnamed_type T>
    inline auto res_db<T>::end() const noexcept {
        return _db.end();
    }

    template<fullnamed_type T>
    inline size_t res_db<T>::size() const noexcept {
        return _db.size();
    }

    template<fullnamed_type T>
    inline bool res_db<T>::exists(const str& fullname, bool local_only) const noexcept {
        return
            ((_upstream && !local_only) ? _upstream->exists(fullname) : false) ||
            _db.contains(fullname);
    }
    
    template<fullnamed_type T>
    inline std::optional<T> res_db<T>::pull(const str& fullname, bool local_only) const {
        if (_upstream && !local_only) {
            if (const auto result = _upstream->pull(fullname)) {
                return result;
            }
        }
        const auto found = _db.find(fullname);
        return
            found != _db.end()
            ? std::make_optional(found->second)
            : std::nullopt;
    }
    
    template<fullnamed_type T>
    inline bool res_db<T>::push(T new_res) {
        const auto fullname = fullname_of(new_res);
        if (exists(fullname)) return false;
        _db.try_emplace(fullname, std::move(new_res));
        return true;
    }

    template<fullnamed_type T>
    inline void res_db<T>::reset() noexcept {
        _db.clear();
    }

    template<fullnamed_type T>
    inline bool res_db<T>::transfer(res_db<T>& target) {
        if (_overlaps(target)) return false;
        target._db.merge(_db);
        return true;
    }

    template<fullnamed_type T>
    inline bool res_db<T>::commit() {
        return
            _upstream
            ? transfer(*_upstream)
            : false;
    }

    template<fullnamed_type T>
    inline bool res_db<T>::_overlaps(const res_db<T>& other) const noexcept {
        // choose either *this or other based on which is smaller, then check
        // if the smaller one's elems are overlapping w/ the other, w/ this
        // way of doing it being fast as it minimizes iterations
        if (size() < other.size()) {
            for (const auto& I : *this) {
                if (other.exists(I.first, true)) return true;
            }
        }
        else {
            for (const auto& I : other) {
                if (exists(I.first, true)) return true;
            }
        }
        return false;
    }
}

