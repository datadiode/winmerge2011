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
// This file implements a string which uses a fixed size memory pool. 
// The bEnableOverflow template parameter allows the container to resort to
// heap allocations if the memory pool is exhausted.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_STRING_H
#define EASTL_FIXED_STRING_H


#include <EASTL/internal/config.h>
#if EASTL_ABSTRACT_STRING_ENABLED
    #include <EASTL/bonus/fixed_string_abstract.h>
#else // 'else' encompasses the entire rest of this file.
#include <EASTL/string.h>
#include <EASTL/internal/fixed_pool.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
    /// EASTL_FIXED_STRING_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_STRING_DEFAULT_NAME
        #define EASTL_FIXED_STRING_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_string" // Unless the user overrides something, this is "EASTL fixed_string".
    #endif



    /// fixed_string
    ///
    /// A fixed_string with bEnableOverflow == true is identical to a regular 
    /// string in terms of its behavior. All the expectations of regular string
    /// apply to it and no additional expectations come from it. When bEnableOverflow
    /// is false, fixed_string behaves like regular string with the exception that 
    /// its capacity can never increase. All operations you do on such a fixed_string
    /// which require a capacity increase will result in undefined behavior or an 
    /// C++ allocation exception, depending on the configuration of EASTL.
    ///
    /// Note: The nodeCount value is the amount of characters to allocate, which needs to 
    /// take into account a terminating zero. Thus if you want to store strings with a strlen
    /// of 30, the nodeCount value must be at least 31.
    ///
    /// Template parameters:
    ///     T                      The type of object the string holds (char, wchar_t, char8_t, char16_t, char32_t).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the overflow heap if our object pool is exhausted.
    ///     OverflowAllocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    /// Notes: 
    ///     The nodeCount value must be at least 2, one for a character and one for a terminating 0.
    ///
    ///     As of this writing, the string class necessarily reallocates when an insert of 
    ///     self is done into self. As a result, the fixed_string class doesn't support
    ///     inserting self into self unless the bEnableOverflow template parameter is true. 
    ///
    /// Example usage:
    ///    fixed_string<char, 128 + 1, true> fixedString("hello world"); // Can hold up to a strlen of 128.
    ///
    ///    fixedString = "hola mundo";
    ///    fixedString.clear();
    ///    fixedString.resize(200);
    ///    fixedString.sprintf("%f", 1.5f);
    ///
    template <typename T, int nodeCount, bool bEnableOverflow = true, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_string : public basic_string<T, fixed_vector_allocator<sizeof(T), nodeCount, EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> >
    {
    public:
        typedef fixed_vector_allocator<sizeof(T), nodeCount, EASTL_ALIGN_OF(T), 
                            0, bEnableOverflow, OverflowAllocator>              fixed_allocator_type;
        typedef typename fixed_allocator_type::overflow_allocator_type          overflow_allocator_type;
        typedef basic_string<T, fixed_allocator_type>                           base_type;
        typedef fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>  this_type;
        typedef typename base_type::size_type                                   size_type;
        typedef typename base_type::value_type                                  value_type;
        typedef typename base_type::CtorDoNotInitialize                         CtorDoNotInitialize;
        typedef typename base_type::CtorSprintf                                 CtorSprintf;
        typedef aligned_buffer<nodeCount * sizeof(T), EASTL_ALIGN_OF(T)>        aligned_buffer_type;

        enum { kMaxSize = nodeCount - 1 }; // -1 because we need to save one element for the silent terminating null.

        using base_type::mAllocator;
        using base_type::npos;
        using base_type::mpBegin;
        using base_type::mpEnd;
        using base_type::mpCapacity;
        using base_type::append;
        using base_type::resize;
        using base_type::clear;
        using base_type::size;
        using base_type::sprintf_va_list;

    protected:
        union // We define a union in order to avoid strict pointer aliasing issues with compilers like GCC.
        {
            value_type          mArray[1];
            aligned_buffer_type mBuffer;     // Question: Why are we doing this aligned_buffer thing? Why not just do an array of value_type, given that we are using just strings of char types.
        };

    public:
        fixed_string();
        explicit fixed_string(const overflow_allocator_type& overflowAllocator);             // Only applicable if bEnableOverflow is true.
        fixed_string(const base_type& x, size_type position, size_type n = base_type::npos); // Currently we don't support overflowAllocator specification for other constructors, for simplicity.
        fixed_string(const value_type* p, size_type n);
        fixed_string(const value_type* p);
        fixed_string(size_type n, const value_type& value);
        fixed_string(const this_type& x);
        fixed_string(const base_type& x);
        fixed_string(const value_type* pBegin, const value_type* pEnd);
        fixed_string(CtorDoNotInitialize, size_type n);
        fixed_string(CtorSprintf, const value_type* pFormat, ...);

        this_type& operator=(const this_type& x);
        this_type& operator=(const base_type& x);
        this_type& operator=(const value_type* p);
        this_type& operator=(const value_type c);

        void swap(this_type& x);

        void      set_capacity(size_type n);
        void      reset_lose_memory();          // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.
        size_type max_size() const;

        // The inherited versions of substr/left/right call the basic_string constructor,
        // which will call the overflow allocator and fail if bEnableOverflow == false
        this_type substr(size_type position, size_type n) const;
        this_type left(size_type n) const;
        this_type right(size_type n) const;

        // OverflowAllocator
        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif

    }; // fixed_string





    ///////////////////////////////////////////////////////////////////////
    // fixed_string
    ///////////////////////////////////////////////////////////////////////

    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string()
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const overflow_allocator_type& overflowAllocator)
        : base_type(fixed_allocator_type(mBuffer.buffer, overflowAllocator))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const this_type& x)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(x);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const base_type& x)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.get_allocator().get_name());
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(x);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const base_type& x, size_type position, size_type n)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.get_allocator().get_name());
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(x, position, n);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const value_type* p, size_type n)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(p, n);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const value_type* p)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        append(p); // There better be enough space to hold the assigned string.
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(size_type n, const value_type& value)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(n, value); // There better be enough space to hold the assigned string.
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(const value_type* pBegin, const value_type* pEnd)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        append(pBegin, pEnd);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(CtorDoNotInitialize, size_type n)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mArray;
        mpCapacity = mpBegin + nodeCount;

        if((mpBegin + n) < mpCapacity)
        {
            mpEnd = mpBegin + n;
           *mpEnd = 0;
        }
        else
        {
            mpEnd = mArray;
           *mpEnd = 0;
            resize(n);
        }
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::fixed_string(CtorSprintf, const value_type* pFormat, ...)
        : base_type(fixed_allocator_type(mBuffer.buffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_STRING_DEFAULT_NAME);
        #endif

        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
       *mpBegin = 0;

        va_list arguments;
        va_start(arguments, pFormat);
        sprintf_va_list(pFormat, arguments);
        va_end(arguments);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::this_type&
    fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            append(x);
        }
        return *this;
    }

 
    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type& fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const base_type& x)
    {
        if(static_cast<base_type*>(this) != &x)
        {
            clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.get_allocator();
            #endif

            append(x);
        }
        return *this;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type& fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const value_type* p)
    {
        if(mpBegin != p)
        {
            clear();
            append(p);
        }
        return *this;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type& fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::operator=(const value_type c)
    {
        clear();
        append((size_type)1, c);
        return *this;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::set_capacity(size_type n)
    {
        // We act consistently with vector::set_capacity and reduce our 
        // size if the new capacity is smaller than our size.
        if(n < size())
            resize(n);
        // To consider: If bEnableOverflow is true, then perhaps we should
        // switch to the overflow allocator and set the capacity.
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
        inline void fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::reset_lose_memory()
    {
        mpBegin = mpEnd = mArray;
        mpCapacity = mpBegin + nodeCount;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    size_type fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::substr(size_type position, size_type n) const
    {
        #if EASTL_STRING_OPT_RANGE_ERRORS
            if(position > (size_type)(mpEnd - mpBegin))
                ThrowRangeException();
        #endif

        return fixed_string(mpBegin + position, mpBegin + position + eastl::min_alt(n, (size_type)(mpEnd - mpBegin) - position));
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::left(size_type n) const
    {
        const size_type nLength = size();
        if(n < nLength)
            return fixed_string(mpBegin, mpBegin + n);
        return *this;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    this_type fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::right(size_type n) const
    {
        const size_type nLength = size();
        if(n < nLength)
            return fixed_string(mpEnd - n, mpEnd);
        return *this;
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline typename fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::
    overflow_allocator_type& fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void 
    fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    // operator ==, !=, <, >, <=, >= come from the string implementations.

    template <typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
    inline void swap(fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>& a, 
                     fixed_string<T, nodeCount, bEnableOverflow, OverflowAllocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // EASTL_ABSTRACT_STRING_ENABLED

#endif // Header include guard













