

#pragma once


#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

#include "../yama++/general.h"
#include "../yama++/Safe.h"
#include "Resource.h"


namespace _ym {


    template<Resource T>
    class Section final {
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
                _it(it),
                _end(end) {}

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


        Section() = default;


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

        // Sets the upstream section, or no upstream section if upstream == nullptr.
        inline void setUpstream(Section* upstream) noexcept {
            _upstream = upstream;
        }

        inline bool push(std::shared_ptr<T> resource) {
            if (!resource) return false;
            const Name& name = resource->getName();
            if (exists(name)) return false;
            _byName.try_emplace(name, resource);
            return true;
        }
        
        // Discards all resources in the section.
        inline void discard(bool propagateUpstream = false) noexcept {
            _byName.clear();
            if (propagateUpstream && _upstream) {
                _upstream->discard(true);
            }
        }

        // TODO: A cool feature to add would be to given commit/commitOrDiscard an optional
        //       'root' parameter, then having it be that ONLY resources which are referenced
        //       directly/indirectly by root are committed, and EVERYTHING ELSE IS DISCARDED!
        //
        //       This would let us be *sloppy*, adding items to an section during loading which
        //       are needed to check things DURING the load, but which aren't depended upon by
        //       the thing being loaded itself, and so could be discarded.
        //
        //       For this we could use a visitor pattern setup akin to what we'd use for a GC.
        //
        //       This impl would need to be able to detect, and avoid traversing, items which
        //       are already upstream, and so 100% don't ref downstream stuff, and so it'd be
        //       wasteful to scan those.
        //
        //       Also, this impl would need to be able to avoid rescanning already traversed
        //       items. For example, method types and their owner types will need to have a
        //       dep graph cycle between them so that traversing from either one GUARANTEED
        //       will visit BOTH, and this graph cycle would need to be handled correctly.

        // Transfers the resources from the section to the upstream section.
        // Fails quietly if there is no upstream section.
        // Behaviour is undefined if a name collision occurs between this section and upstream.
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
        Section* _upstream = nullptr;
        std::unordered_map<Name, std::shared_ptr<T>> _byName;
    };
}

