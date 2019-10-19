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
// This file implements a vector which uses a fixed size memory pool. 
// The bEnableOverflow template parameter allows the container to resort to
// heap allocations if the memory pool is exhausted.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_VECTOR_H
#define EASTL_FIXED_VECTOR_H


#include <EASTL/vector.h>
#include <EASTL/internal/fixed_pool.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
    /// EASTL_FIXED_VECTOR_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_VECTOR_DEFAULT_NAME
        #define EASTL_FIXED_VECTOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_vector" // Unless the user overrides something, this is "EASTL fixed_vector".
    #endif


    /// fixed_vector
    ///
    /// A fixed_vector with bEnableOverflow == true is identical to a regular 
    /// vector in terms of its behavior. All the expectations of regular vector
    /// apply to it and no additional expectations come from it. When bEnableOverflow
    /// is false, fixed_vector behaves like regular vector with the exception that 
    /// its capacity can never increase. All operations you do on such a fixed_vector
    /// which require a capacity increase will result in undefined behavior or an 
    /// C++ allocation exception, depending on the configuration of EASTL.
    ///
    /// Template parameters:
    ///     T                      The type of object the vector holds.
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the overflow heap if our object pool is exhausted.
    ///     OverflowAllocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    /// Note: The nodeCount value must be at least 1.
    ///
    /// Example usage:
    ///    fixed_vector<Widget, 128, true> fixedVector);
    ///
    ///    fixedVector.push_back(Widget());
    ///    fixedVector.resize(200);
    ///    fixedVector.clear();
    ///
    template <typename T, size_t nodeCount, bool bEnableOverflow = true, typename OverflowAllocator = typename eastl::type_select<bEnableOverflow, EASTLAllocatorType, EASTLDummyAllocatorType>::type>
    class fixed_vector : public vector<T, fixed_vector_allocator<sizeof(T), nodeCount, EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> >
    {
    public:
        typedef fixed_vector_allocator<sizeof(T), nodeCount, EASTL_ALIGN_OF(T), 
                            0, bEnableOverflow, OverflowAllocator>              fixed_allocator_type;
        typedef OverflowAllocator                                               overflow_allocator_type;
        typedef vector<T, fixed_allocator_type>                                 base_type;
        typedef fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>  this_type;
        typedef typename base_type::size_type                                   size_type;
        typedef typename base_type::value_type                                  value_type;
        typedef typename base_type::reference                                   reference;
        typedef aligned_buffer<nodeCount * sizeof(T), EASTL_ALIGN_OF(T)>        aligned_buffer_type;

        enum { kMaxSize = nodeCount };

        using base_type::mAllocator;
        using base_type::mpBegin;
        using base_type::mpEnd;
        using base_type::mpCapacity;
        using base_type::resize;
        using base_type::clear;
        using base_type::size;
        using base_type::assign;
        using base_type::npos;
        using base_type::DoAllocate;
        using base_type::DoDestroyValues;
        using base_type::DoFree;

    protected:
        aligned_buffer_type mBuffer;

    public:
        fixed_vector();
        explicit fixed_vector(const overflow_allocator_type& overflowAllocator); // Only applicable if bEnableOverflow is true.
        explicit fixed_vector(size_type n);                                      // Currently we don't support overflowAllocator specification for other constructors, for simplicity.
        fixed_vector(size_type n, const value_type& value);
        fixed_vector(const this_type& x);

        template <typename InputIterator>
        fixed_vector(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void      set_capacity(size_type n);
        void      reset_lose_memory();          // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.
        size_type max_size() const;             // Returns the max fixed size, which is the user-supplied nodeCount parameter.
        bool      full() const;                 // Returns true if the fixed space has been fully allocated. Note that if overflow is enabled, the container size can be greater than nodeCount but full() could return true because the fixed space may have a recently freed slot. 
        bool      has_overflowed() const;       // Returns true if the allocations spilled over into the overflow allocator. Meaningful only if overflow is enabled.
        bool      can_overflow() const;         // Returns the value of the bEnableOverflow template parameter.

        void*     push_back_uninitialized();

        void      push_back(const value_type& value);   // We implement push_back here because we have a specialization that's 
        reference push_back();                          // smaller for the case of overflow being disabled.

        // OverflowAllocator
        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        // Deprecated:
        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif

    protected:
        void*     DoPushBackUninitialized(true_type);
        void*     DoPushBackUninitialized(false_type);

        void      DoPushBack(true_type, const value_type& value);
        void      DoPushBack(false_type, const value_type& value);

        reference DoPushBack(false_type);
        reference DoPushBack(true_type);

    }; // fixed_vector




    ///////////////////////////////////////////////////////////////////////
    // fixed_vector
    ///////////////////////////////////////////////////////////////////////

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector()
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector(const overflow_allocator_type& overflowAllocator)
        : base_type(fixed_allocator_type(mBuffer.buffer, overflowAllocator))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector(size_type n)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        resize(n);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector(size_type n, const value_type& value)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        resize(n, value);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector(const this_type& x)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        assign(x.begin(), x.end());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_vector(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_VECTOR_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
        //assign(first, last); // Metrowerks gets confused by this.
        base_type::DoAssign(first, last, is_integral<InputIterator>());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::this_type& 
    fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            assign(x.begin(), x.end());
        }
        return *this;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::set_capacity(size_type n)
    {
        const size_type nPrevSize     = (size_type)(mpEnd - mpBegin);
        const size_type nPrevCapacity = (size_type)(mpCapacity - mpBegin);

        if(n == npos)       // If the user means to set the capacity so that it equals the size (i.e. free excess capacity)...
            n = nPrevSize;

        if(n != nPrevCapacity)  // If the request results in a capacity change...
        {
            if(can_overflow() && (((uintptr_t)mpBegin != (uintptr_t)mBuffer.buffer) || (n > kMaxSize))) // If we are or would be using dynamically allocated memory instead of our fixed-size member buffer...
            {
                T* const pNewData = (n <= kMaxSize) ? (T*)&mBuffer.buffer[0] : DoAllocate(n);
                T* const pCopyEnd = (n < nPrevSize) ? (mpBegin + n) : mpEnd;
                eastl::uninitialized_copy_ptr(mpBegin, pCopyEnd, pNewData); // Copy [mpBegin, pCopyEnd) to p.
                DoDestroyValues(mpBegin, mpEnd);
                DoFree(mpBegin, (size_type)(mpCapacity - mpBegin));

                mpEnd      = pNewData + (pCopyEnd - mpBegin);
                mpBegin    = pNewData;
                mpCapacity = mpBegin + n;
            } // Else the new capacity would be within our fixed buffer.
            else if(n < nPrevSize) // If the newly requested capacity is less than our size, we do what vector::set_capacity does and resize, even though we actually aren't reducing the capacity.
                resize(n);
        }
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
        inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset_lose_memory()
    {
        mpBegin = mpEnd = (value_type*)&mBuffer.buffer[0];
        mpCapacity = mpBegin + nodeCount;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::size_type
    fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::full() const
    {
        // If size >= capacity, then we are definitely full. 
        // Also, if our size is smaller but we've switched away from mBuffer due to a previous overflow, then we are considered full.
        return ((size_t)(mpEnd - mpBegin) >= kMaxSize) || ((void*)mpBegin != (void*)mBuffer.buffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::has_overflowed() const
    {
        // This will be incorrect for the case that bOverflowEnabled is true and the container was resized
        // down to a small size where the fixed buffer could take over ownership of the data again.
        // The only simple fix for this is to take on another member variable which tracks whether this overflow
        // has occurred at some point in the past.
        return ((void*)mpBegin != (void*)mBuffer.buffer);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline bool fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::can_overflow() const
    {
        return bEnableOverflow;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::push_back_uninitialized()
    {
        return DoPushBackUninitialized(typename type_select<bEnableOverflow, true_type, false_type>::type());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBackUninitialized(true_type)
    {
        return base_type::push_back_uninitialized();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void* fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBackUninitialized(false_type)
    {
        return mpEnd++;
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::push_back(const value_type& value)
    {
        DoPushBack(typename type_select<bEnableOverflow, true_type, false_type>::type(), value);
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBack(true_type, const value_type& value)
    {
        base_type::push_back(value);
    }


    // This template specializes for overflow NOT enabled.
    // In this configuration, there is no need for the heavy weight push_back() which tests to see if the container should grow (it never will)
    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBack(false_type, const value_type& value)
    {
        EASTL_ASSERT(mpEnd < mpCapacity);

        ::new((void*)mpEnd++) value_type(value);
    }

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::reference fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::push_back()
    {
        return DoPushBack(typename type_select<bEnableOverflow, true_type, false_type>::type());
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::reference fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBack(true_type)
    {
        return base_type::push_back();
    }


    // This template specializes for overflow NOT enabled.
    // In this configuration, there is no need for the heavy weight push_back() which tests to see if the container should grow (it never will)
    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::reference fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::DoPushBack(false_type)
    {
        EASTL_ASSERT(mpEnd < mpCapacity);

        ::new((void*)mpEnd++) value_type;    // Note that this isn't value_type() as that syntax doesn't work on all compilers for POD types.

        return *(mpEnd - 1);        // Same as return back();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::overflow_allocator_type& 
    fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void 
    fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }



    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    // operator ==, !=, <, >, <=, >= come from the vector implementations.

    template <typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void swap(fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>& a, 
                     fixed_vector<T, nodeCount, bEnableOverflow, OverflowAllocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }



} // namespace eastl



#endif // Header include guard












