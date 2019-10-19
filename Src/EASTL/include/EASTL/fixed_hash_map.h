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
// This file implements a hash_map and hash_multimap which use a fixed size 
// memory pool for its buckets and nodes. 
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_FIXED_HASH_MAP_H
#define EASTL_FIXED_HASH_MAP_H


#include <EASTL/hash_map.h>
#include <EASTL/internal/fixed_pool.h>

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
    #pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{
    /// EASTL_FIXED_HASH_MAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    /// In the case of fixed-size containers, the allocator name always refers
    /// to overflow allocations. 
    ///
    #ifndef EASTL_FIXED_HASH_MAP_DEFAULT_NAME
        #define EASTL_FIXED_HASH_MAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_hash_map" // Unless the user overrides something, this is "EASTL fixed_hash_map".
    #endif

    #ifndef EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME
        #define EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " fixed_hash_multimap" // Unless the user overrides something, this is "EASTL fixed_hash_multimap".
    #endif



    /// fixed_hash_map
    ///
    /// Implements a hash_map with a fixed block of memory identified by the nodeCount and bucketCount
    /// template parameters. 
    ///
    /// Template parameters:
    ///     Key                    The key type for the map. This is a map of Key to T (value).
    ///     T                      The value type for the map.
    ///     nodeCount              The max number of objects to contain. This value must be >= 1.
    ///     bucketCount            The number of buckets to use. This value must be >= 2.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Hash                   hash_set hash function. See hash_set.
    ///     Predicate              hash_set equality testing function. See hash_set.
    ///
    template <typename Key, typename T, size_t nodeCount, size_t bucketCount = nodeCount + 1, bool bEnableOverflow = true,
              typename Hash = eastl::hash<Key>, typename Predicate = eastl::equal_to<Key>, bool bCacheHashCode = false, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_hash_map : public hash_map<Key, 
                                           T,
                                           Hash,
                                           Predicate,
                                           fixed_hashtable_allocator<
                                                bucketCount + 1,
                                                sizeof(typename hash_map<Key, T, Hash, Predicate, OverflowAllocator, bCacheHashCode>::node_type), 
                                                nodeCount,
                                                EASTL_ALIGN_OF(T), 
                                                0, 
                                                bEnableOverflow,
                                                OverflowAllocator>, 
                                           bCacheHashCode>
    {
    public:
        typedef fixed_hashtable_allocator<bucketCount + 1, sizeof(typename hash_map<Key, T, Hash, Predicate, 
                        OverflowAllocator, bCacheHashCode>::node_type), nodeCount, EASTL_ALIGN_OF(T), 0,
                        bEnableOverflow, OverflowAllocator>                                                                         fixed_allocator_type;
        typedef typename fixed_allocator_type::overflow_allocator_type                                                              overflow_allocator_type;
        typedef hash_map<Key, T, Hash, Predicate, fixed_allocator_type, bCacheHashCode>                                             base_type;
        typedef fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator> this_type;
        typedef typename base_type::node_type                                                                                       node_type;
        typedef typename base_type::size_type                                                                                       size_type;

        enum { kMaxSize = nodeCount };

        using base_type::mAllocator;
        using base_type::insert;

    protected:
        node_type** mBucketBuffer[bucketCount + 1]; // '+1' because the hash table needs a null terminating bucket.
        char        mNodeBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

    public:
        explicit fixed_hash_map(const overflow_allocator_type& overflowAllocator);

        explicit fixed_hash_map(const Hash& hashFunction = Hash(), 
                                const Predicate& predicate = Predicate());

        fixed_hash_map(const Hash& hashFunction, 
                       const Predicate& predicate,
                       const overflow_allocator_type& overflowAllocator);

        template <typename InputIterator>
        fixed_hash_map(InputIterator first, InputIterator last, 
                        const Hash& hashFunction = Hash(), 
                        const Predicate& predicate = Predicate());

        fixed_hash_map(const this_type& x);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void reset_lose_memory(); // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        size_type max_size() const;

        overflow_allocator_type& get_overflow_allocator();
        void                     set_overflow_allocator(const overflow_allocator_type& allocator);

        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif
    }; // fixed_hash_map





    /// fixed_hash_multimap
    ///
    /// Implements a hash_multimap with a fixed block of memory identified by the nodeCount and bucketCount
    /// template parameters. 
    ///
    /// Template parameters:
    ///     Key                    The key type for the map. This is a map of Key to T (value).
    ///     T                      The value type for the map.
    ///     nodeCount              The max number of objects to contain. This value must be >= 1.
    ///     bucketCount            The number of buckets to use. This value must be >= 2.
    ///     bEnableOverflow        Whether or not we should use the global heap if our object pool is exhausted.
    ///     Hash                   hash_set hash function. See hash_set.
    ///     Predicate              hash_set equality testing function. See hash_set.
    ///
    template <typename Key, typename T, size_t nodeCount, size_t bucketCount = nodeCount + 1, bool bEnableOverflow = true,
              typename Hash = eastl::hash<Key>, typename Predicate = eastl::equal_to<Key>, bool bCacheHashCode = false, typename OverflowAllocator = EASTLAllocatorType>
    class fixed_hash_multimap : public hash_multimap<Key,
                                                     T,
                                                     Hash,
                                                     Predicate,
                                                     fixed_hashtable_allocator<
                                                        bucketCount + 1, 
                                                        sizeof(typename hash_multimap<Key, T, Hash, Predicate, OverflowAllocator, bCacheHashCode>::node_type), 
                                                        nodeCount,
                                                        EASTL_ALIGN_OF(T), 
                                                        0, 
                                                        bEnableOverflow,
                                                        OverflowAllocator>, 
                                                     bCacheHashCode>
    {
    public:
        typedef fixed_hashtable_allocator<bucketCount + 1, sizeof(typename hash_multimap<Key, T, Hash, Predicate, 
                        OverflowAllocator, bCacheHashCode>::node_type), nodeCount, EASTL_ALIGN_OF(T), 0, 
                        bEnableOverflow, OverflowAllocator>                                                                              fixed_allocator_type;
        typedef typename fixed_allocator_type::overflow_allocator_type                                                                   overflow_allocator_type;
        typedef hash_multimap<Key, T, Hash, Predicate, fixed_allocator_type, bCacheHashCode>                                             base_type;
        typedef fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator> this_type;
        typedef typename base_type::node_type                                                                                            node_type;
        typedef typename base_type::size_type                                                                                            size_type;

        enum { kMaxSize = nodeCount };

        using base_type::mAllocator;
        using base_type::insert;

    protected:
        node_type** mBucketBuffer[bucketCount + 1]; // '+1' because the hash table needs a null terminating bucket.
        char        mNodeBuffer[fixed_allocator_type::kBufferSize]; // kBufferSize will take into account alignment requirements.

    public:
        explicit fixed_hash_multimap(const overflow_allocator_type& overflowAllocator);

        explicit fixed_hash_multimap(const Hash& hashFunction = Hash(), 
                                        const Predicate& predicate = Predicate());

        fixed_hash_multimap(const Hash& hashFunction,
                            const Predicate& predicate,
                            const overflow_allocator_type& overflowAllocator);

        template <typename InputIterator>
        fixed_hash_multimap(InputIterator first, InputIterator last, 
                        const Hash& hashFunction = Hash(), 
                        const Predicate& predicate = Predicate());

        fixed_hash_multimap(const this_type& x);

        this_type& operator=(const this_type& x);

        void swap(this_type& x);

        void reset_lose_memory(); // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

        size_type max_size() const;

        overflow_allocator_type& get_overflow_allocator();

        void set_overflow_allocator(const overflow_allocator_type& allocator);

        #if EASTL_RESET_ENABLED
            void reset(); // This function name is deprecated; use reset_lose_memory instead.
        #endif
    }; // fixed_hash_multimap






    ///////////////////////////////////////////////////////////////////////
    // fixed_hash_multimap
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_map(const overflow_allocator_type& overflowAllocator)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), Hash(), 
                    Predicate(), fixed_allocator_type(NULL, mBucketBuffer, overflowAllocator))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_map(const Hash& hashFunction, 
                   const Predicate& predicate)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_map(const Hash& hashFunction, 
                   const Predicate& predicate,
                   const overflow_allocator_type& overflowAllocator)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer, overflowAllocator))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_map(InputIterator first, InputIterator last, 
                    const Hash& hashFunction, 
                    const Predicate& predicate)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
        insert(first, last);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_map(const this_type& x)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), x.hash_function(), 
                    x.equal_function(), fixed_allocator_type(NULL, mBucketBuffer))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        mAllocator.reset(mNodeBuffer);
        insert(x.begin(), x.end());
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::this_type& 
    fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            base_type::clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            insert(x.begin(), x.end());
        }
        return *this;
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
        inline void fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
        reset()
        {
            reset_lose_memory();
        }
    #endif


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    reset_lose_memory()
    {
        base_type::reset_lose_memory();
        base_type::get_allocator().reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::size_type 
    fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::overflow_allocator_type& 
    fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode>
    inline void swap(fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& a, 
                     fixed_hash_map<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }




    ///////////////////////////////////////////////////////////////////////
    // fixed_hash_multimap
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_multimap(const overflow_allocator_type& overflowAllocator)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), Hash(), 
                    Predicate(), fixed_allocator_type(NULL, mBucketBuffer, overflowAllocator))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_multimap(const Hash& hashFunction, 
                        const Predicate& predicate)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_multimap(const Hash& hashFunction,
                        const Predicate& predicate,
                        const overflow_allocator_type& overflowAllocator)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer, overflowAllocator))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    template <typename InputIterator>
    fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_multimap(InputIterator first, InputIterator last, 
                        const Hash& hashFunction, 
                        const Predicate& predicate)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), hashFunction, 
                    predicate, fixed_allocator_type(NULL, mBucketBuffer))
    {
        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(EASTL_FIXED_HASH_MULTIMAP_DEFAULT_NAME);
        #endif

        mAllocator.reset(mNodeBuffer);
        insert(first, last);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    fixed_hash_multimap(const this_type& x)
        : base_type(prime_rehash_policy::GetPrevBucketCountOnly(bucketCount), x.hash_function(), 
                    x.equal_function(),fixed_allocator_type(NULL, mBucketBuffer))
    {
        mAllocator.copy_overflow_allocator(x.mAllocator);

        #if EASTL_NAME_ENABLED
            mAllocator.set_name(x.mAllocator.get_name());
        #endif

        EASTL_CT_ASSERT((nodeCount >= 1) && (bucketCount >= 2));
        base_type::set_max_load_factor(10000.f); // Set it so that we will never resize.

        mAllocator.reset(mNodeBuffer);
        insert(x.begin(), x.end());
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::this_type& 
    fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::operator=(const this_type& x)
    {
        if(this != &x)
        {
            base_type::clear();

            #if EASTL_ALLOCATOR_COPY_ENABLED
                mAllocator = x.mAllocator;
            #endif

            insert(x.begin(), x.end());
        }
        return *this;
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    swap(this_type& x)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(*this, x);
    }


    #if EASTL_RESET_ENABLED
        // This function name is deprecated; use reset_lose_memory instead.
        template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
        inline void fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
        reset()
        {
            reset_lose_memory();
        }
    #endif

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::
    reset_lose_memory()
    {
        base_type::reset_lose_memory();
        base_type::get_allocator().reset(mNodeBuffer);
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::size_type
    fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::max_size() const
    {
        return kMaxSize;
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline typename fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::overflow_allocator_type& 
    fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::get_overflow_allocator()
    {
        return mAllocator.get_overflow_allocator();
    }


    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode, typename OverflowAllocator>
    inline void fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode, OverflowAllocator>::set_overflow_allocator(const overflow_allocator_type& allocator)
    {
        mAllocator.set_overflow_allocator(allocator);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, size_t nodeCount, size_t bucketCount, bool bEnableOverflow, typename Hash, typename Predicate, bool bCacheHashCode>
    inline void swap(fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& a, 
                     fixed_hash_multimap<Key, T, nodeCount, bucketCount, bEnableOverflow, Hash, Predicate, bCacheHashCode>& b)
    {
        // Fixed containers use a special swap that can deal with excessively large buffers.
        eastl::fixed_swap(a, b);
    }



} // namespace eastl




#endif // Header include guard












