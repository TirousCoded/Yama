

#pragma once


#include "call_frame_mem.h"


namespace yama::internal {


    // TODO: this is part of the backend for now, but we may replace it w/
    //       a frontend version later

    template<typename Allocator>
    class call_frame final {
    public:

        inline call_frame(
            Allocator al,
            context& ctx,
            size_t max_locals);

        call_frame() = delete;
        call_frame(const call_frame<Allocator>&) = delete;
        inline call_frame(call_frame<Allocator>&& other) noexcept;
        inline ~call_frame() noexcept;
        call_frame<Allocator>& operator=(const call_frame<Allocator>&) = delete;
        inline call_frame<Allocator>& operator=(call_frame<Allocator>&& other) noexcept;


        inline size_t max_locals() const noexcept;
        inline size_t locals() const noexcept;

        // in_bounds returns if ind is in-bounds of the local object stack

        inline bool in_bounds(size_t ind) const noexcept;

        // borrow_local returns std::nullopt if out-of-bounds

        inline std::optional<borrowed_ref> borrow_local(size_t ind) const;

        // put(_move)/push(_move) return if they succeeded

        // (put/push)_move do not move if they fail
        
        // put(_move) fails if ind out-of-bounds
        // push(_move) fails if locals() == max_locals()

        inline bool put(size_t ind, borrowed_ref x);
        inline bool put_move(size_t ind, stolen_ref x);
        inline bool push(borrowed_ref x);
        inline bool push_move(stolen_ref x);

        // pop returns if it actually popped anything
        // pop_all pops everything

        inline bool pop();
        inline void pop_all();

        // pop_move pops the top object, and then ownership moves it to
        // the caller via returning it, allowing for removal of the top
        // object w/out having to modify its ref count

        // pop_move returns std::nullopt if there is no top object

        inline std::optional<object_ref> pop_move();


        inline Allocator get_allocator() const noexcept;


    private:

        Allocator _al;

        // _mem is an std::optional in order to ensure that move semantics
        // don't cause dtor to attempt to deinit invalid call_frame_mem state

        std::optional<call_frame_mem> _mem; // this will handle via RAII


        inline call_frame_mem& _get_mem() noexcept;
        inline const call_frame_mem& _get_mem() const noexcept;

        inline context& _ctx() const noexcept;

        void _init_new_object(size_t ind, borrowed_ref x);
        void _init_new_object_move(size_t ind, stolen_ref x);
        void _deinit_old_object(size_t ind);
        object_ref _deinit_old_object_move(size_t ind);


        static inline call_frame_mem _create_mem(Allocator al, context& ctx, size_t max_locals);
        static inline void _destroy_mem(Allocator al, call_frame_mem x) noexcept;
    };


    template<typename Allocator>
    inline call_frame<Allocator>::call_frame(Allocator al, context& ctx, size_t max_locals) 
        : _al(al), 
        _mem(std::make_optional(_create_mem(al, ctx, max_locals))) {}

    template<typename Allocator>
    inline call_frame<Allocator>::call_frame(call_frame<Allocator>&& other) noexcept
        : _al(other.get_allocator()),
        _mem(std::move(other._mem)) {
        other._mem = std::nullopt;
    }

    template<typename Allocator>
    inline call_frame<Allocator>::~call_frame() noexcept {
        if (!_mem) return; // if _mem was removed via move semantics, exit
        pop_all(); // pop_all locals to ensure ref count behaviour is as expected
        _destroy_mem(get_allocator(), _get_mem()); // RAII cleanup of _get_mem()
    }

    template<typename Allocator>
    inline call_frame<Allocator>& call_frame<Allocator>::operator=(call_frame<Allocator>&& other) noexcept {
        if (&other == this) return *this;
        std::swap(_al, other._al);
        std::swap(_mem, other._mem);
        return *this;
    }

    template<typename Allocator>
    inline size_t call_frame<Allocator>::max_locals() const noexcept {
        return _get_mem()->max_locals;
    }

    template<typename Allocator>
    inline size_t call_frame<Allocator>::locals() const noexcept {
        return _get_mem()->locals;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::in_bounds(size_t ind) const noexcept {
        return ind < locals();
    }

    template<typename Allocator>
    inline std::optional<borrowed_ref> call_frame<Allocator>::borrow_local(size_t ind) const {
        return
            in_bounds(ind)
            ? std::make_optional(_get_mem().elems()[ind].view())
            : std::nullopt;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::put(size_t ind, borrowed_ref x) {
        if (!in_bounds(ind)) return false; // out-of-bounds
        _deinit_old_object(ind);
        _init_new_object(ind, x);
        return true;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::put_move(size_t ind, stolen_ref x) {
        if (!in_bounds(ind)) return false; // out-of-bounds
        _deinit_old_object(ind);
        _init_new_object_move(ind, x); // move x
        return true;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::push(borrowed_ref x) {
        if (locals() == max_locals()) return false; // overflow
        _get_mem()->locals++; // add obj to stk
        _init_new_object(_get_mem()->locals - 1, x);
        return true;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::push_move(stolen_ref x) {
        if (locals() == max_locals()) return false; // overflow
        _get_mem()->locals++; // add obj to stk
        _init_new_object_move(_get_mem()->locals - 1, x); // move x
        return true;
    }

    template<typename Allocator>
    inline bool call_frame<Allocator>::pop() {
        if (locals() == 0) return false; // no locals
        _deinit_old_object(_get_mem()->locals - 1);
        _get_mem()->locals--; // remove obj from stk
        return true;
    }

    template<typename Allocator>
    inline void call_frame<Allocator>::pop_all() {
        while (locals() > 0) {
            pop();
        }
    }

    template<typename Allocator>
    inline std::optional<object_ref> call_frame<Allocator>::pop_move() {
        if (locals() == 0) return std::nullopt; // no locals
        const auto top = _deinit_old_object_move(_get_mem()->locals - 1); // move top
        _get_mem()->locals--; // remove obj from stk
        return std::make_optional(top);
    }

    template<typename Allocator>
    inline Allocator call_frame<Allocator>::get_allocator() const noexcept {
        return _al;
    }
    
    template<typename Allocator>
    inline call_frame_mem& call_frame<Allocator>::_get_mem() noexcept {
        YAMA_ASSERT(_mem);
        return *_mem;
    }

    template<typename Allocator>
    inline const call_frame_mem& call_frame<Allocator>::_get_mem() const noexcept {
        YAMA_ASSERT(_mem);
        return *_mem;
    }

    template<typename Allocator>
    inline context& call_frame<Allocator>::_ctx() const noexcept {
        YAMA_ASSERT(_get_mem()->ctx);
        return *_get_mem()->ctx;
    }

    template<typename Allocator>
    inline void call_frame<Allocator>::_init_new_object(size_t ind, borrowed_ref x) {
        YAMA_ASSERT(!_get_mem().elems()[ind].good());
        _get_mem().elems()[ind].view_unchecked() = _ctx().ll_clone_ref(x); // clone x to init new object
        YAMA_ASSERT(_get_mem().elems()[ind].good());
    }

    template<typename Allocator>
    inline void call_frame<Allocator>::_init_new_object_move(size_t ind, stolen_ref x) {
        YAMA_ASSERT(!_get_mem().elems()[ind].good());
        _get_mem().elems()[ind].view_unchecked() = x; // move x
        YAMA_ASSERT(_get_mem().elems()[ind].good());
    }

    template<typename Allocator>
    inline void call_frame<Allocator>::_deinit_old_object(size_t ind) {
        YAMA_ASSERT(_get_mem().elems()[ind].good());
        _ctx().ll_drop_ref(_get_mem().elems()[ind].view()); // drop popped object
        _get_mem().elems()[ind] = call_frame_mem_elem{}; // reset obj mem to null just to be safe
        YAMA_ASSERT(!_get_mem().elems()[ind].good());
    }

    template<typename Allocator>
    inline object_ref call_frame<Allocator>::_deinit_old_object_move(size_t ind) {
        YAMA_ASSERT(_get_mem().elems()[ind].good());
        // acquire moved object to return
        const auto obj = object_ref(_get_mem().elems()[ind].view());
        _get_mem().elems()[ind] = call_frame_mem_elem{}; // reset obj mem to null just to be safe
        YAMA_ASSERT(!_get_mem().elems()[ind].good());
        return obj;
    }

    template<typename Allocator>
    inline call_frame_mem call_frame<Allocator>::_create_mem(Allocator al, context& ctx, size_t max_locals) {
        call_frame_mem_header header{
            .ctx = &ctx,
            .max_locals = max_locals,
            .locals = 0,
        };
        return call_frame_mem::create(al, std::move(header));
    }

    template<typename Allocator>
    inline void call_frame<Allocator>::_destroy_mem(Allocator al, call_frame_mem x) noexcept {
        call_frame_mem::destroy(al, x);
    }
}

