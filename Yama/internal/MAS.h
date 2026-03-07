

#pragma once


#include "../yama++/meta.h"
#include "../yama++/Safe.h"
#include "../yama++/Variant.h"


// TODO: The below code unfinished, but is REALLY GOOD, and is for when we want to commit
//       to switching from abstract base class based MASs, to template based ones.
#if 0
namespace _ym {


    // Models Memory Allocation Systems (MASs).
    template<typename T>
    concept MAS =
        std::move_constructible<T> &&
        std::destructible<T> &&
        requires (T v, size_t bytes, void* block)
    {
        // Returns nullptr upon alloc fail.
        // Allocated blocks must have alignment alignof(std::max_align_t).
        { v.allocate(bytes) } -> std::convertible_to<void*>;

        // NOTE: Must be able to dealloc blocks w/out explicitly being told what its size is.

        // Fails quietly if p == nullptr.
        { v.deallocate(block) } noexcept;
    };

    // Models MASs which retain ownership of all memory they allocate.
    template<typename T>
    concept OwningMAS =
        MAS<T> &&
        requires (T v)
    {
        // NOTE: 'reset' must be called by dtor.
        
        // Releases all allocated memory.
        { v.reset() } noexcept;
    };


    class DummyMAS final {
    public:
        DummyMAS() = default;
        ~DummyMAS() noexcept = default;
        DummyMAS(const DummyMAS&) = default;
        DummyMAS(DummyMAS&) noexcept = default;
        DummyMAS& operator=(const DummyMAS&) = default;
        DummyMAS& operator=(DummyMAS&) noexcept = default;


        constexpr void* allocate(size_t) { return nullptr; }
        constexpr void deallocate(void*) noexcept {}
        constexpr void release() noexcept {}
    };

    static_assert(OwningMAS<DummyMAS>);


    class HeapMAS final {
    public:
        HeapMAS() = default;
        ~HeapMAS() noexcept = default;
        HeapMAS(const HeapMAS&) = default;
        HeapMAS(HeapMAS&) noexcept = default;
        HeapMAS& operator=(const HeapMAS&) = default;
        HeapMAS& operator=(HeapMAS&) noexcept = default;


        inline void* allocate(size_t bytes) {
            return std::malloc(bytes);
        }
        inline void deallocate(void* block) noexcept {
            if (block) {
                std::free(block);
            }
        }
    };

    static_assert(MAS<HeapMAS>);


    // C++ allocator used to allocate/deallocate using a MAS in a type-erased manner.
    template<MAS MAST, typename T>
    class AllocFor final {
    public:
        using value_type = T;

        template<typename U>
        struct rebind final {
            using other = AllocFor<MAST, U>;
        };


        inline AllocFor(MAST& mas) :
            _mas(mas) {
        }

        AllocFor() = delete;
        ~AllocFor() noexcept = default;
        AllocFor(const AllocFor&) = default;
        AllocFor(AllocFor&) noexcept = default;
        AllocFor& operator=(const AllocFor&) = default;
        AllocFor& operator=(AllocFor&) noexcept = default;


        bool operator==(const AllocFor&) const noexcept = default;

        inline value_type* allocate(size_t n) {
            return (value_type*)_mas->allocate(n * sizeof(value_type));
        }
        // The size n is not actually used by the MAS.
        inline void deallocate(value_type* block, size_t n) noexcept {
            _mas->deallocate((void*)block);
        }


    private:
        ym::Safe<MAST> _mas;
    };

    static_assert(ym::Allocator<AllocFor<HeapMAS, int>>);


    template<MAS... Ts>
    class ChoiceMAS final {
    public:
        template<ym::PackParam<Ts...> T>
        inline ChoiceMAS(T&& mas) :
            _mas(std::forward<T>(mas)) {
        }

        ChoiceMAS() = delete;
        ~ChoiceMAS() noexcept = default;
        ChoiceMAS(const ChoiceMAS&) = default;
        ChoiceMAS(ChoiceMAS&) noexcept = default;
        ChoiceMAS& operator=(const ChoiceMAS&) = default;
        ChoiceMAS& operator=(ChoiceMAS&) noexcept = default;


        inline void* allocate(size_t bytes) {
            return _alloc(bytes);
        }
        inline void deallocate(void* block) noexcept {
            _dealloc(block);
        }
        inline void reset() noexcept {
            // TODO
        }


    private:
        using _Variant = ym::Variant<Ts...>;

        _Variant _mas;


        template<size_t I>
            requires ym::validPackIndex<I, Ts...>
        inline bool _attemptAllocWithAltAtIndex(void*& result, size_t bytes) {
            if (auto mas = _mas.tryAs<I>()) {
                result = mas->allocate(bytes);
                return true;
            }
            return false;
        }
        template<size_t... Is>
        inline void _attemptAllocWithEachAlt(void*& result, size_t bytes, std::index_sequence<Is...>) {
            // '||' means it'll stop at first success.
            (_attemptAllocWithAltAtIndex<Is>(result, bytes) || ...);
        }
        inline void* _alloc(size_t bytes) {
            void* result = nullptr;
            _attemptAllocWithEachAlt(result, bytes, std::make_index_sequence<_Variant::size>{});
            return result;
        }

        template<size_t I>
            requires ym::validPackIndex<I, Ts...>
        inline bool _attemptDeallocWithAltAtIndex(void* block) noexcept {
            if (auto mas = _mas.tryAs<I>()) {
                mas->deallocate(block);
                return true;
            }
            return false;
        }
        template<size_t... Is>
        inline void _attemptDeallocWithEachAlt(void* block, std::index_sequence<Is...>) noexcept {
            // '||' means it'll stop at first success.
            (_attemptDeallocWithAltAtIndex<Is>(block) || ...);
        }
        inline void _dealloc(void* block) noexcept {
            if (block) {
                _attemptDeallocWithEachAlt(block, std::make_index_sequence<_Variant::size>{});
            }
        }

        template<MAS T>
        struct _Releaser final {
            inline void operator()(T& mas) noexcept {}
        };
        template<OwningMAS T>
        struct _Releaser final {
            inline void operator()(T& mas) noexcept {
                mas.release();
            }
        };
    };

    static_assert(OwningMAS<ChoiceMAS<DummyMAS, HeapMAS>>);
}
#endif

namespace _ym {


    // TODO: Maybe replace vtable stuff w/ generics? We could then impl a 'ChoiceMAS' monadic
    //       class which lets us have multiple different MAS's be available for init at runtime
    //       based on what index value is chosen at init time.

    // NOTE: One of the reasons I'm using MAS over memory_resource is that the ladder forces
    //		 us to use a memory_resource impl middle-man in order to do things like tracking
    //		 memory usage stats when using something like unsynchronized_pool_resource.

    
    // Base class of fast Memory Allocation Systems (MASs).
    class MAS {
    public:
        template<typename T>
        class Allocator;


        virtual ~MAS() noexcept = default;


        inline void* allocate(size_t bytes) {
            return doAllocate(bytes);
        }
        inline void deallocate(void* block) noexcept {
            if (block) {
                doDeallocate(block);
            }
        }

        template<typename T>
        inline Allocator<T> allocator() noexcept {
            return Allocator<T>(*this);
        }


    protected:
        // TODO: I'm really not 100% sure how I feel about using virtual methods for the
        //		 central allocation/deallocation of memory for Yama objects.
        //
        //		 The reason is that vtable-based dynamic dispatch adds more overhead to
        //		 EVERY allocate/deallocate in the form of memory indirection, which in turn
        //		 may lead to data cache misses.
        //
        //		 MAYBE the fact that the same vtable will be tapped over-and-over again by
        //		 the object system will lead to better cache performance, but this too is
        //		 at present just a hypothesis.
        //
        //		 For now we'll just use these virtual methods, but later on we should do
        //		 benchmarking to see their performance impact.

        // NOTE: MAS impls must be able to dealloc blocks w/out explicitly being told what
        //		 its size is.
        
        // NOTE: Allocated blocks must have alignment alignof(std::max_align_t).

        virtual void* doAllocate(size_t bytes) = 0;
        virtual void doDeallocate(void* block) noexcept = 0;
    };

    // C++ allocator used to allocate/deallocate using a MAS in a type-erased manner.
    template<typename T>
    class MAS::Allocator final {
    public:
        using value_type = T;

        template<typename U>
        struct rebind final {
            using other = Allocator<U>;
        };


        inline Allocator(MAS& mas) :
            _mas(mas) {
        }
        template<typename Other>
        inline Allocator(const Allocator<Other>& other) noexcept :
            Allocator(*other._mas) {
        }

        Allocator() = delete;
        ~Allocator() noexcept = default;
        Allocator(const Allocator&) = default;
        Allocator(Allocator&) noexcept = default;
        Allocator& operator=(const Allocator&) = default;
        Allocator& operator=(Allocator&) noexcept = default;


        bool operator==(const Allocator&) const noexcept = default;

        inline value_type* allocate(size_t n) {
            return (value_type*)_mas->allocate(n * sizeof(value_type));
        }
        // The size n is not actually used by the MAS.
        inline void deallocate(value_type* block, size_t n) noexcept {
            _mas->deallocate(block);
        }


    private:
        template<typename Other>
        friend class Allocator;


        ym::Safe<MAS> _mas;
    };

    static_assert(ym::Allocator<MAS::Allocator<int>>);

    // Base class of MASs which retain knowledge of all memory they allocate, and can release it all at once.
    // All allocated memory is released upon MAS destruction.
    class OwningMAS : public MAS {
    public:
        inline ~OwningMAS() noexcept {
            reset();
        }


        // Releasing all allocated memory.
        inline void reset() noexcept {
            doReset();
        }


    protected:
        virtual void doReset() noexcept = 0;
    };


    class DummyMAS final : public MAS {
    public:
        DummyMAS() = default;


    protected:
        inline void* doAllocate(size_t bytes) override {
            return nullptr;
        }
        inline void doDeallocate(void* block) noexcept override {
            //
        }
    };


    class HeapMAS final : public MAS {
    public:
        HeapMAS() = default;


    protected:
        inline void* doAllocate(size_t bytes) override {
            return std::malloc(bytes);
        }
        inline void doDeallocate(void* block) noexcept override {
            std::free(block);
        }
    };
}

