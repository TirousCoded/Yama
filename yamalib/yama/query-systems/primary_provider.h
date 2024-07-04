

#pragma once


#include "general.h"
#include "provider.h"


namespace yama::qs {


    // primary_provider encapsulates a repository of primary info into 
    // which new info may be 'pushed' explicitly, w/ provider operations
    // transparently forwarding to this repo of primary info, performing
    // no other computation/caching of secondary info


    template<typename Key>
    class primary_provider final : public provider<Key> {
    public:

        using key_t = typename Key;
        using result_t = result<key_t>;


        primary_provider() = default;


        // push pushes new info r under k to the primary info of the provider,
        // returning if it succeeded

        // push will fail if k already exists within primary info

        // behaviour is undefined if the mapping k onto r is inappropriate
        // according to the key/result pair's semantics

        inline bool push(key_t k, result_t r);

        // reset resets the provider's primary info, discarding all of it, 
        // and by extension resetting the provider's secondary info as well

        inline void reset() noexcept;


        inline size_t number() const noexcept override final;
        inline bool exists(const key_t& k) const noexcept override final;
        inline std::optional<result_t> query(const key_t& k) override final;
        inline std::optional<result_t> fetch(const key_t& k) override final;

        // discard and discard_all are noops in this impl, as the provider's
        // secondary info related behaviour is transparent

        inline void discard(const key_t&) override final;
        inline void discard_all() override final;


    private:

        std::unordered_map<key_t, result_t> _data;
    };


    template<typename Key>
    inline bool primary_provider<Key>::push(key_t k, result_t r) {
        if (_data.contains(k)) return false;
        // can't use operator[] here, as result_t may not have a default ctor to use
        _data.try_emplace(std::move(k), std::move(r));
        return true;
    }
    
    template<typename Key>
    inline void primary_provider<Key>::reset() noexcept {
        _data.clear();
    }
    
    template<typename Key>
    inline size_t primary_provider<Key>::number() const noexcept {
        return _data.size();
    }
    
    template<typename Key>
    inline bool primary_provider<Key>::exists(const key_t& k) const noexcept {
        return _data.contains(k);
    }
    
    template<typename Key>
    inline std::optional<typename primary_provider<Key>::result_t> primary_provider<Key>::query(const key_t& k) {
        return fetch(k);
    }
    
    template<typename Key>
    inline std::optional<typename primary_provider<Key>::result_t> primary_provider<Key>::fetch(const key_t& k) {
        auto found = _data.find(k);
        return
            found != _data.end()
            ? std::make_optional(found->second)
            : std::nullopt;
    }
    
    template<typename Key>
    inline void primary_provider<Key>::discard(const key_t&) {
        // do nothing
    }

    template<typename Key>
    inline void primary_provider<Key>::discard_all() {
        // do nothing
    }
}

