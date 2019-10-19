/*
Copyright (C) 2009,2010,2012 Electronic Arts, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
3.  Neither the name of Electronic Arts, Inc. ("EA") nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ELECTRONIC ARTS AND ITS CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS OR ITS CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


///////////////////////////////////////////////////////////////////////////////
// This file implements a list which uses a fixed size memory pool for its nodes. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_LIST_H
#define EASTL_FIXED_LIST_H


#include <EASTL/list.h>
#include <EASTL/internal/fixed_pool.h>
#include <EASTL/sort.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
    /// EASTL_FIXED_LIST_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_LIST_DEFAULT_NAME
        #define EASTL_FIXED_LIST_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_list" // Unless the user overrides something, this is "EASTL fixed_list".
    #endif



    /// fixed_list
    ///
    /// fixed_list is a list which uses a single block of contiguous memory 
    /// for its nodes. The purpose of this is to reduce memory usage relative
    /// to a conventional memory allocation system (with block headers), to 
    /// increase allocation speed (often due to avoidance of mutex locks),
    /// to increase performance (due to better memory locality), and to decrease
    /// memory fragmentation due to the way that fixed block allocators work.
    ///
    /// The primary downside to a fixed_list is that the number of nodes it
    /// can contain is fixed upon its declaration. If you want a fixed_list
    /// that doesn't have this limitation, then you probably don't want a
    /// fixed_list. You can always create your own memory allocator that works
    /// the way you want.
    ///
    /// Template parameters:
    ///     T                      The type of object the list holds.
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the overflow heap if our object pool is exhausted.
    ///     OverflowAllocator      Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename T, size_t nodeCount, bool bEnableOverflow = true, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_list : public list<T, fixed_node_allocator<sizeof(typename list<T>::node_type), 
                                   nodeCount, EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> >
    {
    public:
        typedef fixed_node_allocator<sizeof(typename list<T>::node_type), nodeCount, 
                     EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> fixed_allocator_type;
        typedef OverflowAllocator                                              overflow_allocator_type;
        typedef list<T, fixed_allocator_type>                                  base_type;
        typedef fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>   this_type;
        typedef typename base_type::size_type                                  size_type;
        typedef typename base_type::value_type                                 value_type;
        typedef typename base_type::node_type                                  node_type;
        typedef typename base_type::iterator                                   iterator;

        enum { kMaxSize = nodeCount };

        using base_type::assign;
        using base_type::resize;
        using base_type::insert;
        using base_type::size;
        using base_type::get_allocator;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        fixed_list();
        explicit fixed_list(const overflow_allocator_type& overflowAllocator); // Only applicable if bEnableOverflow is true.
        explicit fixed_list(size_type n);                                      // Currently we don't support overflowAllocator specification for other constructors, for simplicity.
        fixed_list(size_type n, const value_type& value);
        fixed_list(const this_type& x);

        template <typename InputIterator>
        fixed_list(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void      swap(this_type& x);
        void      reset_lose_memory();      // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.
        size_type max_size() const;         // Returns the max fixed size, which is the user-supplied nodeCount parameter.
        bool      full() const;             // Returns true if the fixed space has been fully allocated. Note that if overflow is enabled, the container size can be greater than nodeCount but full() could return true because the fixed space may have a recently freed slot. 
        bool      has_overflowed() const;   // Returns true if the allocations spilled over into the overflow allocator. Meaningful only if overflow is enabled.
        bool      can_overflow() const;     // Returns the value of the bEnableOverflow template parameter.

        template<typename Compare>
        void sort(Compare compare);
        void sort();

        template <typename Compare>
        void merge(this_type& x, Compare compare);
        void merge(this_type& x);

        void splice(iterator position, this_type& x);
        void splice(iterator position, this_type& x, iterator i);
        void splice(iterator position, this_type& x, iterator first, iterator last);

        // OverflowAllocator
        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        // Deprecated:
        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif
    }; // fixed_list



    ///////////////////////////////////////////////////////////////////////
    // fixed_list
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list()
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list(const overflow_allocator_type& overflowAllocator)
        : base_type(fixed_allocator_type(NULL, overflowAllocator))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list(size_type n)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        resize(n);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list(size_type n, const value_type& value)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        resize(n, value);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list(const this_type& x)
        : base_type(fixed_allocator_type(NULL))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mAllocator.reset(mBuffer);
        assign(x.begin(), x.end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_list(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_LIST_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        assign(first, last);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::this_type&
    fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            base_type::clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            base_type::assign(x.begin(), x.end()); // It would probably be better to implement this like list::operator=.
        }
        return *this;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
        inline void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset_lose_memory()
    {
        base_type::reset_lose_memory();
        get_allocator().reset(mBuffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::size_type
    fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::full() const
    {
        // Note: This implementation isn't right in the case of bEnableOverflow = true because it will return 
        // false for the case that  there are free nodes from the buffer but also nodes from the dynamic heap. 
        // This can happen if the container exceeds the fixed size and then frees some of the nodes from the fixed buffer.
        // The only simple fix for this is to take on another member variable which tracks whether this overflow
        // has occurred at some point in the past.
        return !mAllocator.can_allocate(); // This is the quickest way of detecting this. has_overflowed uses a different method because it can't use this quick method.
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::has_overflowed() const
    {
        #if EASTL_FIXED_SIZE_TRACKING_ENABLED // If we can use this faster pathway (as size() may be slow)...
            return (mAllocator.mPool.mnPeakSize > kMaxSize);
        #else
            return (size() > kMaxSize);
        #endif
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::can_overflow() const
    {
        return bEnableOverflow;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::sort()
    {
        eastl::insertion_sort(base_type::begin(), base_type::end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    template <typename Compare>
    inline void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::sort(Compare compare)
    {
        eastl::insertion_sort(base_type::begin(), base_type::end(), compare);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::merge(this_type& /*x*/)
    {
        // To do.
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    template <typename Compare>
    void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::merge(this_type& /*x*/, Compare /*compare*/)
    {
        // To do.
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::splice(iterator position, this_type& x)
    {
        // fixed-size containers use local memory instead of a shared heap, and so we can't move
        // element references between containers like we can with list::splice. We need to copy
        // the elements as with regular element insertion.
        insert(position, x.begin(), x.end());
        x.clear();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::splice(iterator position, this_type& x, iterator i)
    {
        insert(position, *i);
        x.erase(i);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    void fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::splice(iterator position, this_type& x, iterator first, iterator last)
    {
        insert(position, first, last);
        x.erase(first, last);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::overflow_allocator_type& 
    fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void 
    fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void swap(fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>& a, 
                     fixed_list<T, nodeCount, bEnableOverflow, OverflowAllocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // Header include guard












