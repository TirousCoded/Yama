

#pragma once


#include "HAL.h"

#include "../yama++/meta.h"


namespace _ym {


    // TODO: A little write-up describing our allocator design:
    //			- A 'page' which is a block of memory, and acts like an allocator for
    //			  memory from said page.
    //              - All the memory of the page (ie. header + blocks) should all exist
    //                inline in one big memory block.
    //				- Can only alloc as many blocks as are on the page.
    //				- Allocated blocks are all the same size.
    //				- Uses a singly linked list stack of blocks to recycle deallocations.
    //					- When number of recycled blocks equals max blocks available on
    //					  the page, we can actually discard the recycling stack and just
    //					  alloc via incremental slicing again.
    //				- Incrementally *slices* new blocks from the page when nothing is
    //				  available in recycling stack.
    //			- A fixed-size block allocator.
    //				- Manages collection of pages for allocation.
    //				- Allocated blocks are all the same size.
    //				- Records pages in two doubly linked lists:
    //					1) A list of 'saturated' pages, which have no free blocks available.
    //                  2) A list of 'unsaturated' pages, which have free blocks available.
    //              - Upon allocation it gets a page from the unsaturated page list and
    //                uses it for the alloc.
    //                  - If the alloc saturates the page, it's moved to the saturated page
    //                    list.
    //                  - If no unsaturated page is available, a new page is allocated and
    //                    the alloc is retried.
    //              - Upon deallocation the block is deallocated w/ the appropriate page.
    //                  - This means blocks will need knowledge of their page.
    //                      - We can put this info in a 'header' for the memory block, w/ the
    //                        pointer given to end-user being located at a part of the block
    //                        immediately following this header (ie. where end-user data is.)
    //                      - This'll also need to be able to be used by the fixed-size block
    //                        allocator to VARY QUICKLY locate the page in bookkeeping memory.
    //                  - If the page is in saturated page list, it's to be moved to the
    //                    unsaturated page list.
    //          - A general-purpose fast memory allocator.
    //              - Employs a set of fixed-size block allocators of increasing size to handle
    //                small/medium size allocs.
    //                  - Block size should increase by something like powers-of-two.
    //              - Allocs larger than largest block size uses malloc/free.
    //                  - Block header will need to be able to indicate these blocks as not
    //                    originating from a page (maybe nullptr header?)

    // TODO: The below code is NOT AT ALL FINISHED!!!!


    template<size_t BytesPerBlock>
        requires (BytesPerBlock >= 1)
    class MemPage;
    template<size_t BytesPerBlock>
        requires (BytesPerBlock >= 1)
    class MemBlockAlloc;
    class MemAlloc;


    // Fast fixed-size block memory allocator.
    // This allocates memory from a single contiguous page of blocks, which
    // are stored inline with the memory of MemPage itself.
    template<size_t BytesPerBlock>
        requires (BytesPerBlock >= 1)
    class alignas(alignof(std::max_align_t)) MemPage final {
    public:
        MemPage() = default;


        // Number of blocks on the page.
        size_t size() const noexcept;

        // Number of blocks available to allocate.
        size_t available() const noexcept;

        void* allocate() noexcept;
        void deallocate(void* block) noexcept;


    private:
        struct alignas(alignof(std::max_align_t)) _Block final {
            union {
                // The page this block belongs to.
                // This is stored for allocated blocks.
                MemPage<BytesPerBlock>* page = nullptr;
                // The block below this on the page's recycling stack.
                // This is stored for recycled blocks.
                _Block* next;
            };
            // The memory of the block + padding.
            // Pointers allocated/deallocated by the end-user directly point to
            // this data, w/ the block header 'page' ptr above being accessed
            // via pointer subtraction.
            YmUInt8 bytes[BytesPerBlock] = { 0 };
        };


        // The block at the top of the recycling stack.
        _Block* _topRecycledBlock = nullptr;
    };


    // Fast fixed-size block memory allocator.
    // The pool of memory 
    template<size_t BytesPerBlock>
        requires (BytesPerBlock >= 1)
    class MemBlockAlloc final {
    public:
        MemBlockAlloc() = default;


        void* allocate();
        void deallocate(void* block) noexcept;
    };


    // Fast general-purpose memory allocator.
    class MemAlloc final {
    public:
        MemAlloc() = default;


        void* allocate(size_t bytes);
        void deallocate(void* block) noexcept;
    };
}

