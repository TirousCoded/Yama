

#pragma once


#include <optional>
#include <unordered_map>

#include "fullnamed_type.h"


namespace yama::dm {


    // res_db encapsulates a database of mappings of 'fullname' string
    // names to resources (of types compatible w/ res_db)

    template<fullnamed_type T>
    class res_db final {
    public:

        res_db() = default;


        // TODO: begin, cbegin, end, and cend, have not been unit tested

        inline auto begin() const noexcept;
        inline auto cbegin() const noexcept { return begin(); }
        inline auto end() const noexcept;
        inline auto cend() const noexcept { return end(); }


        // size returns the number of resources in the database

        inline size_t size() const noexcept;


        // exists checks if a resource exists in the database
        // under fullname

        inline bool exists(const str& fullname) const noexcept;


        // NOTE: pull is non-noexcept as copying T could throw

        // pull attempts to pull a copy of the resource under 
        // fullname, if any

        inline std::optional<T> pull(const str& fullname) const;


        // push attempts to push a new resource to the database,
        // returning if successful

        // push will fail if yama::dm::fullname_of(new_res) is 
        // already taken by another resource

        inline bool push(T new_res);


        // reset resets database state

        inline void reset() noexcept;


        // transfer attempts to transfer the resources of this
        // database to target, removing them from this database,
        // returning if successful

        // transfer will fail if there is any fullname overlap
        // between resources of *this and target

        inline bool transfer(res_db<T>& target);


    private:

        std::unordered_map<str, T> _db;


        inline bool _overlaps(const res_db<T>& other) const noexcept;
    };


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
    inline bool res_db<T>::exists(const str& fullname) const noexcept {
        return _db.contains(fullname);
    }
    
    template<fullnamed_type T>
    inline std::optional<T> res_db<T>::pull(const str& fullname) const {
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
    inline bool res_db<T>::_overlaps(const res_db<T>& other) const noexcept {
        // choose either *this or other based on which is smaller, then check
        // if the smaller one's elems are overlapping w/ the other, w/ this
        // way of doing it being fast as it minimizes iterations
        if (size() < other.size()) {
            for (const auto& I : *this) {
                if (other.exists(I.first)) return true;
            }
        }
        else {
            for (const auto& I : other) {
                if (exists(I.first)) return true;
            }
        }
        return false;
    }
}

