/*
Copyright (C) 2005,2009,2010,2012 Electronic Arts, Inc.  All rights reserved.

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
// EASTL/internal/type_transformations.h
// Written and maintained by Paul Pedriana - 2005
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_TYPE_TRANFORMATIONS_H
#define EASTL_INTERNAL_TYPE_TRANFORMATIONS_H


namespace eastl
{


    // The following transformations are defined here. If the given item 
    // is missing then it simply hasn't been implemented, at least not yet.
    //    add_unsigned
    //    add_signed
    //    remove_const
    //    remove_volatile
    //    remove_cv
    //    add_const
    //    add_volatile
    //    add_cv
    //    remove_reference
    //    add_reference
    //    remove_extent
    //    remove_all_extents
    //    remove_pointer
    //    add_pointer
    //    aligned_storage
    //    union_cast


    ///////////////////////////////////////////////////////////////////////
    // add_signed
    //
    // Adds signed-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_signed<int>::type is int
    // add_signed<unsigned int>::type is int
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_signed_CONFORMANCE 1    // add_signed is conforming.

    template<class T>
    struct add_signed
    { typedef T type; };

    template<>
    struct add_signed<unsigned char>
    { typedef signed char type; };

    #if (defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)) // If char is unsigned (which is usually not the case)...
        template<>
        struct add_signed<char>
        { typedef signed char type; };
    #endif

    template<>
    struct add_signed<unsigned short>
    { typedef short type; };

    template<>
    struct add_signed<unsigned int>
    { typedef int type; };

    template<>
    struct add_signed<unsigned long>
    { typedef long type; };

    template<>
    struct add_signed<unsigned long long>
    { typedef long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 4294967295U)) // If wchar_t is a 32 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 65535)) // If wchar_t is a 16 bit unsigned value...
            template<>
            struct add_signed<wchar_t>
            { typedef int16_t type; };
        #endif
    #endif



    ///////////////////////////////////////////////////////////////////////
    // add_unsigned
    //
    // Adds unsigned-ness to the given type. 
    // Modifies only integral values; has no effect on others.
    // add_unsigned<int>::type is unsigned int
    // add_unsigned<unsigned int>::type is unsigned int
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_unsigned_CONFORMANCE 1    // add_unsigned is conforming.

    template<class T>
    struct add_unsigned
    { typedef T type; };

    template<>
    struct add_unsigned<signed char>
    { typedef unsigned char type; };

    #if (defined(CHAR_MAX) && defined(SCHAR_MAX) && (CHAR_MAX == SCHAR_MAX)) // If char is signed (which is usually so)...
        template<>
        struct add_unsigned<char>
        { typedef unsigned char type; };
    #endif

    template<>
    struct add_unsigned<short>
    { typedef unsigned short type; };

    template<>
    struct add_unsigned<int>
    { typedef unsigned int type; };

    template<>
    struct add_unsigned<long>
    { typedef unsigned long type; };

    template<>
    struct add_unsigned<long long>
    { typedef unsigned long long type; };

    #ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
        #if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 2147483647)) // If wchar_t is a 32 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint32_t type; };
        #elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 32767)) // If wchar_t is a 16 bit signed value...
            template<>
            struct add_unsigned<wchar_t>
            { typedef uint16_t type; };
        #endif
    #endif

    ///////////////////////////////////////////////////////////////////////
    // remove_const
    //
    // Remove const from a type.
    //
    // The remove_const transformation trait removes top-level const 
    // qualification (if any) from the type to which it is applied. For a 
    // given type T, remove_const<T const>::type is equivalent to the type T. 
    // For example, remove_const<char*>::type is equivalent to char* while 
    // remove_const<const char*>::type is equivalent to const char*. 
    // In the latter case, the const qualifier modifies char, not *, and is 
    // therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_remove_const_CONFORMANCE 1    // remove_const is conforming.

    template<typename T>
    struct remove_const{ typedef T type; };

    template<typename T>
    struct remove_const<T const>{ typedef T type; };
  


    ///////////////////////////////////////////////////////////////////////
    // remove_volatile
    //
    // Remove volatile from a type.
    //
    // The remove_volatile transformation trait removes top-level volatile 
    // qualification (if any) from the type to which it is applied. 
    // For a given type T, the type remove_volatile <T volatile>::T is equivalent 
    // to the type T. For example, remove_volatile <char* volatile>::type is 
    // equivalent to char* while remove_volatile <volatile char*>::type is 
    // equivalent to volatile char*. In the latter case, the volatile qualifier 
    // modifies char, not *, and is therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_remove_volatile_CONFORMANCE 1    // remove_volatile is conforming.

    template<typename T>
    struct remove_volatile{ typedef T type; };

    template<typename T>
    struct remove_volatile<T volatile>{ typedef T type; };
  


    ///////////////////////////////////////////////////////////////////////
    // remove_cv
    //
    // Remove const and volatile from a type.
    //
    // The remove_cv transformation trait removes top-level const and/or volatile 
    // qualification (if any) from the type to which it is applied. For a given type T, 
    // remove_cv<T const volatile>::type is equivalent to T. For example, 
    // remove_cv<char* volatile>::type is equivalent to char*, while remove_cv<const char*>::type 
    // is equivalent to const char*. In the latter case, the const qualifier modifies 
    // char, not *, and is therefore not at the top level.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_remove_cv_CONFORMANCE 1    // remove_cv is conforming.

    /* Older version
    template <typename T> struct remove_cv_imp{};
    template <typename T> struct remove_cv_imp<T*>                { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const T*>          { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<volatile T*>       { typedef T unqualified_type; };
    template <typename T> struct remove_cv_imp<const volatile T*> { typedef T unqualified_type; };

    template <typename T> struct remove_cv{ typedef typename remove_cv_imp<T*>::unqualified_type type; };
    template <typename T> struct remove_cv<T&>{ typedef T& type; }; // References are automatically not const nor volatile. See section 8.3.2p1 of the C++ standard.

    template <typename T, size_t N> struct remove_cv<T const[N]>         { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T volatile[N]>      { typedef T type[N]; };
    template <typename T, size_t N> struct remove_cv<T const volatile[N]>{ typedef T type[N]; };
    */

    template<typename T>
    struct remove_cv
    {
        typedef typename remove_const<typename remove_volatile<T>::type>::type type;
    };
  


    ///////////////////////////////////////////////////////////////////////
    // add_const
    //
    // Add const to a type.
    //
    // Tor a given type T, add_const<T>::type is equivalent to T 
    // const if is_const<T>::value == false, and
    //    - is_void<T>::value == true, or
    //    - is_object<T>::value == true.
    //
    // Otherwise, add_const<T>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_const_CONFORMANCE 1    // add_const is conforming.

    template<typename T>
    struct add_const{ typedef T const type; };
   


    ///////////////////////////////////////////////////////////////////////
    // add_volatile
    //
    // Add volatile to a type.
    // 
    // For a given type T, add_volatile<T>::type is equivalent to T volatile 
    // if is_volatile<T>::value == false, and
    //   - is_void<T>::value == true, or
    //   - is_object<T>::value == true.
    //
    // Otherwise, add_volatile<T>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_volatile_CONFORMANCE 1    // add_volatile is conforming.

    template<typename T>
    struct add_volatile{ typedef T volatile type; };
  


    ///////////////////////////////////////////////////////////////////////
    // add_cv
    //
    // The add_cv transformation trait adds const and volatile qualification 
    // to the type to which it is applied. For a given type T, 
    // add_volatile<T>::type is equivalent to add_const<add_volatile<T>::type>::type.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_cv_CONFORMANCE 1    // add_cv is conforming.

    template<typename T>
    struct add_cv
    {
        typedef typename add_const<typename add_volatile<T>::type>::type type;
    };



    ///////////////////////////////////////////////////////////////////////
    // add_reference
    //
    // Add reference to a type.
    //
    // The add_reference transformation trait adds a level of indirection 
    // by reference to the type to which it is applied. For a given type T, 
    // add_reference<T>::type is equivalent to T& if is_reference<T>::value == false, 
    // and T otherwise.
    //
    ///////////////////////////////////////////////////////////////////////

    #define EASTL_TYPE_TRAIT_add_reference_CONFORMANCE 1    // add_reference is conforming.

    template <typename T>
    struct add_reference_impl{ typedef T& type; };

    template <typename T>
    struct add_reference_impl<T&>{ typedef T& type; };

    template <>
    struct add_reference_impl<void>{ typedef void type; };

    template <typename T>
    struct add_reference { typedef typename add_reference_impl<T>::type type; };



    ///////////////////////////////////////////////////////////////////////
    // remove_reference
    //
    // Remove reference from a type.
    //
    // The remove_reference transformation trait removes top-level of 
    // indirection by reference (if any) from the type to which it is applied. 
    // For a given type T, remove_reference<T&>::type is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_pointer
    //
    // Remove pointer from a type.
    //
    // The remove_pointer transformation trait removes top-level indirection 
    // by pointer (if any) from the type to which it is applied. Pointers to 
    // members are not affected. For a given type T, remove_pointer<T*>::type 
    // is equivalent to T.
    //
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_extent
    //
    // The remove_extent transformation trait removes a dimension from an array.
    // For a given non-array type T, remove_extent<T>::type is equivalent to T.
    // For a given array type T[N], remove_extent<T[N]>::type is equivalent to T.
    // For a given array type const T[N], remove_extent<const T[N]>::type is equivalent to const T.
    // For example, given a multi-dimensional array type T[M][N], remove_extent<T[M][N]>::type is equivalent to T[N].
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // remove_all_extents
    //
    // The remove_all_extents transformation trait removes all dimensions from an array.
    // For a given non-array type T, remove_all_extents<T>::type is equivalent to T.
    // For a given array type T[N], remove_all_extents<T[N]>::type is equivalent to T.
    // For a given array type const T[N], remove_all_extents<const T[N]>::type is equivalent to const T.
    // For example, given a multi-dimensional array type T[M][N], remove_all_extents<T[M][N]>::type is equivalent to T.
    ///////////////////////////////////////////////////////////////////////

    // Not implemented yet.



    ///////////////////////////////////////////////////////////////////////
    // aligned_storage
    //
    // The aligned_storage transformation trait provides a type that is 
    // suitably aligned to store an object whose size is does not exceed length 
    // and whose alignment is a divisor of alignment. When using aligned_storage, 
    // length must be non-zero, and alignment must be equal to alignment_of<T>::value 
    // for some type T.
    //
    ///////////////////////////////////////////////////////////////////////

    // template <size_t length, size_t alignment> struct aligned_storage;
    // Not implemented yet.


    ///////////////////////////////////////////////////////////////////////
    // union_cast
    //
    // Safely converts between unrelated types that have a binary equivalency.
    // This appoach is required by strictly conforming C++ compilers because
    // directly using a C or C++ cast between unrelated types is fraught with 
    // the possibility of undefined runtime behavior due to type aliasing.
    // The Source and Dest types must be POD types due to the use of a union 
    // in C++ versions prior to C++11. C++11 relaxes the definition of a POD
    // such that it allows a classes with trivial default constructors whereas 
    // previous versions did not, so beware of this when writing portable code.
    //
    // Example usage:
    //    float f32 = 1.234f;
    //    uint32_t n32 = union_cast<uint32_t>(f32);
    //
    // Example possible mis-usage:
    // The following is valid only if you are aliasing the pointer value and 
    // not what it points to. Most of the time the user intends the latter, 
    // which isn't strictly possible.
    //    Widget* pWidget = CreateWidget();
    //    Foo*    pFoo    = union_cast<Foo*>(pWidget);
    ///////////////////////////////////////////////////////////////////////

    template <typename DestType, typename SourceType>
    DestType union_cast(SourceType sourceValue)
    {
        EASTL_CT_ASSERT((sizeof(DestType) == sizeof(SourceType)) && 
                        (EA_ALIGN_OF(DestType) == EA_ALIGN_OF(SourceType)));               // To support differening alignments, we would need to use a memcpy-based solution or find a way to make the two union members align with each other.
        //EASTL_CT_ASSERT(is_pod<DestType>::value && is_pod<SourceType>::value);           // Disabled because we don't want to restrict what the user can do, as some compiler's definitions of is_pod aren't up to C++11 Standards.
        //EASTL_CT_ASSERT(!is_pointer<DestType>::value && !is_pointer<SourceType>::value); // Disabled because it's valid to alias pointers as long as you are aliasong the pointer value and not what it points to.

        union {
            SourceType sourceValue;
            DestType   destValue;
        } u;
        u.sourceValue = sourceValue;

        return u.destValue;
    }


} // namespace eastl


#endif // Header include guard





















