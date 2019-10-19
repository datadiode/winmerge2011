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
// This file implements a map and multimap which use a fixed size memory 
// pool for their nodes. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_MAP_H
#define EASTL_FIXED_MAP_H


#include <EASTL/map.h>
#include <EASTL/fixed_set.h> // Included because fixed_rbtree_base resides here.

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
    /// EASTL_FIXED_MAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_MAP_DEFAULT_NAME
        #define EASTL_FIXED_MAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_map" // Unless the user overrides something, this is "EASTL fixed_map".
    #endif

    #ifndef EASTL_FIXED_MULTIMAP_DEFAULT_NAME
        #define EASTL_FIXED_MULTIMAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_multimap" // Unless the user overrides something, this is "EASTL fixed_multimap".
    #endif



    /// fixed_map
    ///
    /// Implements a map with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    ///     Key                    The key object (key in the key/value pair).
    ///     T                      The mapped object (value in the key/value pair).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     OverflowAllocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_map : public map<Key, T, Compare, fixed_node_allocator<sizeof(typename map<Key, T>::node_type), 
                                 nodeCount, EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> >
    {
    public:
        typedef fixed_node_allocator<sizeof(typename map<Key, T>::node_type), nodeCount, 
                     EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator>                                             fixed_allocator_type;
        typedef typename fixed_allocator_type::overflow_allocator_type                                                     overflow_allocator_type;
        typedef fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>                                  this_type;
        typedef map<Key, T, Compare, fixed_allocator_type>                                                                 base_type;
        typedef typename base_type::value_type                                                                             value_type;
        typedef typename base_type::node_type                                                                              node_type;
        typedef typename base_type::size_type                                                                              size_type;

        enum { kMaxSize = nodeCount };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        fixed_map();
        explicit fixed_map(const overflow_allocator_type& overflowAllocator);
        explicit fixed_map(const Compare& compare);
        fixed_map(const this_type& x);

        template <typename InputIterator>
        fixed_map(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void reset_lose_memory(); // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        size_type max_size() const;

        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif
    }; // fixed_map




    /// fixed_multimap
    ///
    /// Implements a multimap with a fixed block of memory identified by the 
    /// nodeCount template parameter. 
    ///
    ///     Key                    The key object (key in the key/value pair).
    ///     T                      The mapped object (value in the key/value pair).
    ///     nodeCount              The max number of objects to contain.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Compare                Compare function/object for set ordering.
    ///     OverflowAllocator              Overflow allocator, which is only used if bEnableOverflow == true. Defaults to the global heap.
    ///
    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow = true, typename Compare = eastl::less<Key>, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_multimap : public multimap<Key, T, Compare, fixed_node_allocator<sizeof(typename multimap<Key, T>::node_type), 
                                           nodeCount, EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator> >
    {
    public:
        typedef fixed_node_allocator<sizeof(typename multimap<Key, T>::node_type), nodeCount, 
                    EASTL_ALIGN_OF(T), 0, bEnableOverflow, OverflowAllocator>                                                        fixed_allocator_type;
        typedef typename fixed_allocator_type::overflow_allocator_type                                                               overflow_allocator_type;
        typedef multimap<Key, T, Compare, fixed_allocator_type>                                                                      base_type;
        typedef fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>                                       this_type;
        typedef typename base_type::value_type                                                                                       value_type;
        typedef typename base_type::node_type                                                                                        node_type;
        typedef typename base_type::size_type                                                                                        size_type;

        enum { kMaxSize = nodeCount };

        using base_type::insert;

    protected:
        char mBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

        using base_type::mAllocator;

    public:
        fixed_multimap();
        fixed_multimap(const overflow_allocator_type& overflowAllocator);
        explicit fixed_multimap(const Compare& compare);
        fixed_multimap(const this_type& x);

        template <typename InputIterator>
        fixed_multimap(InputIterator first, InputIterator last);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void reset_lose_memory(); // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        size_type max_size() const;

        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif
    }; // fixed_multimap





    ///////////////////////////////////////////////////////////////////////
    // fixed_map
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_map()
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_map(const overflow_allocator_type& overflowAllocator)
        : base_type(fixed_allocator_type(NULL, overflowAllocator))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_map(const Compare& compare)
        : base_type(compare, fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_map(const this_type& x)
        : base_type(x.mCompare, fixed_allocator_type(NULL))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mAllocator.reset(mBuffer);
        base_type::operator=(x);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::
    fixed_map(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        insert(first, last);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::this_type&
    fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::operator=(const this_type& x)
    {
        base_type::operator=(x);
        return *this;
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
        inline void fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::reset_lose_memory()
    {
        base_type::reset_lose_memory();
        base_type::get_allocator().reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::size_type
    fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::overflow_allocator_type&
    fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void
    fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }

    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void swap(fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>& a, 
                     fixed_map<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }




    ///////////////////////////////////////////////////////////////////////
    // fixed_multimap
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_multimap()
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_multimap(const overflow_allocator_type& overflowAllocator)
        : base_type(fixed_allocator_type(NULL, overflowAllocator))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_multimap(const Compare& compare)
        : base_type(compare, fixed_allocator_type(mBuffer))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::fixed_multimap(const this_type& x)
        : base_type(x.mCompare, fixed_allocator_type(NULL))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        mAllocator.reset(mBuffer);
        base_type::operator=(x);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::
    fixed_multimap(InputIterator first, InputIterator last)
        : base_type(fixed_allocator_type(NULL))
    {
        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mBuffer);
        insert(first, last);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::this_type& 
    fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::operator=(const this_type& x)
    {
        base_type::operator=(x);
        return *this;
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
        inline void fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::reset_lose_memory()
    {
        base_type::reset_lose_memory();
        base_type::get_allocator().reset(mBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::size_type 
    fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline typename fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::overflow_allocator_type&
    fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void
    fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, bool bEnableOverflow, typename Compare, typename OverflowAllocator>
    inline void swap(fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>& a, 
                     fixed_multimap<Key, T, nodeCount, bEnableOverflow, Compare, OverflowAllocator>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }


} // namespace eastl


#endif // Header include guard









