

#pragma once


#include <vector>
#include <unordered_map>
#include <optional>
#include <concepts>

#include "../core/general.h"


namespace yama::internal {

    
    template<std::unsigned_integral Index, typename Name>
    class index_registry final {
    public:
        static constexpr Index max = std::numeric_limits<Index>::max();


        index_registry() = default;
        index_registry(const index_registry&) = default;
        index_registry(index_registry&&) noexcept = default;
        ~index_registry() noexcept = default;
        index_registry& operator=(const index_registry&) = default;
        index_registry& operator=(index_registry&&) noexcept = default;


        // Returns entries in registry.
        inline Index count() const noexcept {
            return Index(_index_to_name.size());
        }

        // Returns if entry exists.
        inline bool exists(Index index) const noexcept {
            return index < count();
        }
        // Returns if entry exists.
        inline bool exists(const Name& name) const noexcept {
            return _name_to_index.contains(name);
        }

        // Returns name under index, if any.
        inline std::optional<Name> name(Index index) const noexcept {
            return
                exists(index)
                ? std::make_optional(_index_to_name[index])
                : std::nullopt;
        }

        // Returns index under name, if any.
        inline std::optional<Index> index(const Name& name) const noexcept {
            const auto it = _name_to_index.find(name);
            return
                it != _name_to_index.end()
                ? std::make_optional(it->second)
                : std::nullopt;
        }

        // Returns index of existing if index under name already exists.
        // Fails if adding would surpass limit.
        inline std::optional<Index> pull_index(const Name& name) {
            if (const auto existing = index(name)) {
                return existing;
            }
            // TODO: what if vector/unordered_map have max below this one?
            if (count() == max) {
                return std::nullopt;
            }
            const auto index = count();
            _index_to_name.push_back(name);
            _name_to_index.insert({ name, index });
            return index;
        }


    private:
        std::vector<Name> _index_to_name;
        std::unordered_map<Name, Index> _name_to_index;
    };
}

