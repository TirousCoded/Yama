

#pragma once


#include <concepts>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

#include "../yama++/general.h"
#include "../yama++/Safe.h"


namespace _ym {


    template<typename T>
    concept AreaResource =
        requires (const std::remove_cvref_t<T> v)
    {
        typename T::Name;
        { v.getName() } noexcept -> std::convertible_to<const typename T::Name&>;
    };

    template<AreaResource T>
    class Area final {
    public:
        using Name = T::Name;


    private:
        using _NameMapT = std::unordered_map<Name, std::shared_ptr<T>>;


    public:
        class Iterator final {
        public:
            using value_type = T;
            using pointer = value_type*;
            using reference = value_type&;
            using difference_type = ptrdiff_t;
            using iterator_category = std::forward_iterator_tag;


            inline Iterator(_NameMapT::const_iterator it, _NameMapT::const_iterator end) :
                _it(it), _end(end) {}

            Iterator() = default;
            Iterator(const Iterator&) = default;
            Iterator(Iterator&&) noexcept = default;
            ~Iterator() noexcept = default;
            Iterator& operator=(const Iterator&) = default;
            Iterator& operator=(Iterator&&) noexcept = default;


            bool operator==(const Iterator&) const noexcept = default;

            inline Iterator& operator++() noexcept {
                if (_it != _end) {
                    ++_it;
                }
                return *this;
            }
            inline Iterator operator++(int) noexcept {
                auto old = *this;
                ++*this;
                return old;
            }
            inline pointer operator->() const {
                if (_it == _end) {
                    throw std::out_of_range("Cannot dereference past-the-end iterator!");
                }
                return _it->second.get();
            }
            // Throws std::out_of_range if past-the-end.
            inline reference operator*() const {
                if (_it == _end) {
                    throw std::out_of_range("Cannot dereference past-the-end iterator!");
                }
                return ym::deref(_it->second);
            }


        private:
            _NameMapT::const_iterator _it, _end;
        };

        using View = std::ranges::subrange<Iterator>;


        Area() = default;


        // Traversal doesn't acknowledge upstream resources.
        inline Iterator begin() const noexcept { return Iterator(_byName.begin(), _byName.end()); }
        // Traversal doesn't acknowledge upstream resources.
        inline Iterator cbegin() const noexcept { return begin(); }
        // Traversal doesn't acknowledge upstream resources.
        inline Iterator end() const noexcept { return Iterator(_byName.end(), _byName.end()); }
        // Traversal doesn't acknowledge upstream resources.
        inline Iterator cend() const noexcept { return end(); }

        // Traversal doesn't acknowledge upstream resources.
        inline View view() const noexcept { return View(begin(), end()); }

        // Traversal doesn't acknowledge upstream resources.
        inline Iterator find(const Name& name) const noexcept {
            return Iterator(_byName.find(name), _byName.end());
        }

        inline size_t count(bool localOnly = false) const noexcept {
            return
                _upstream && !localOnly
                ? _byName.size() + _upstream->count()
                : _byName.size();
        }

        inline bool exists(const Name& name, bool localOnly = false) const noexcept {
            return
                _upstream && !localOnly
                ? _byName.contains(name) || _upstream->exists(name)
                : _byName.contains(name);
        }
        
        inline std::shared_ptr<T> fetch(const Name& name, bool localOnly = false) const noexcept {
            if (const auto it = _byName.find(name); it != _byName.end()) {
                return it->second;
            }
            return
                _upstream && !localOnly
                ? _upstream->fetch(name, false)
                : nullptr;
        }

        // Sets the upstream area, or no upstream area if upstream == nullptr.
        inline void setUpstream(Area* upstream) noexcept {
            _upstream = upstream;
        }

        inline bool push(std::shared_ptr<T> resource) {
            if (!resource) return false;
            const Name& name = resource->getName();
            if (exists(name)) return false;
            _byName.try_emplace(name, resource);
            return true;
        }
        
        // Discards all resources in the area.
        inline void discard(bool propagateUpstream = false) noexcept {
            _byName.clear();
            if (propagateUpstream && _upstream) {
                _upstream->discard(true);
            }
        }
        // Transfers the resources from the area to the upstream area.
        // Fails quietly if there is no upstream area.
        // Behaviour is undefined if a name collision occurs between this area and upstream.
        inline void commit() {
            if (_upstream) {
                // Assert that no name collision occurs between this and upstream.
                ymAssert([&]() -> bool {
                    for (const auto& [key, value] : _byName) {
                        if (_upstream->exists(key)) return false;
                    }
                    return true;
                    }());
                _upstream->_byName.merge(_byName);
                ymAssert(_byName.empty());
            }
        }
        inline void commitOrDiscard(bool commit) {
            if (commit) this->commit();
            else        discard();
        }


    private:
        Area* _upstream = nullptr;
        std::unordered_map<Name, std::shared_ptr<T>> _byName;
    };
}

