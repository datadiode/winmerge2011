/*
Copyright (C) 2009, 2010, 2012 Electronic Arts, Inc.  All rights reserved.

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
// eastl_rw.h
//
// This is an EASTL configuration header that can be used in place of 
// EABase and which uses RenderWare 4.5 definitions. 
//
// To use this file, you can either copy and paste its contents right below 
// the EASTL_USER_CONFIG_HEADER section of EASTL's config.h or you can leave 
// config.h unmodified and instead #define EASTL_USER_CONFIG_HEADER be 
// config_rw.h and config.h will #include this file automatically.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_RW_H
#define EASTL_RW_H


// Include RenderWare core headers
#include <rw/core/base/ostypes.h>
#include <rw/core/corelite.h>


// Disable the use of eabase.h
#define EASTL_EABASE_DISABLED


// From here on, we replicate functionality from EABase.
#ifndef EA_WCHAR_T_NON_NATIVE
    #if defined(__ICL) || defined(__ICC)
        #if (EA_COMPILER_VERSION < 700)
            #define EA_WCHAR_T_NON_NATIVE 1
        #else
            #if (!defined(_WCHAR_T_DEFINED) && !defined(_WCHAR_T))
                #define EA_WCHAR_T_NON_NATIVE 1
            #endif
        #endif
    #elif defined(_MSC_VER) || defined(EA_COMPILER_BORLAND)
        #ifndef _NATIVE_WCHAR_T_DEFINED
            #define EA_WCHAR_T_NON_NATIVE 1
        #endif
    #endif
#endif


#ifndef EA_PLATFORM_WORD_SIZE
    #if defined (rwHOST_XBOX2) || defined (rwHOST_PS3) || defined (rwHOST_PS2)
        #define EA_PLATFORM_WORD_SIZE 8
    #else
        #define EA_PLATFORM_WORD_SIZE rwHOST_POINTERSIZE
    #endif
#endif


#if defined(__GNUC__) && (defined(__ICL) || defined(__ICC))
    #define EA_COMPILER_NO_EXCEPTIONS
#elif (defined(__GNUC__) || (defined(__ICL) || defined(__ICC)) || defined(CS_UNDEFINED_STRING)) && !defined(__EXCEPTIONS)
    #define EA_COMPILER_NO_EXCEPTIONS
#elif (defined(__BORLANDC__) || defined(_MSC_VER)) && !defined(_CPPUNWIND)
    #define EA_COMPILER_NO_UNWIND
#endif


#ifndef EASTL_DEBUG
    #if defined(RWDEBUG) || defined(_DEBUG)
        #define EASTL_DEBUG 1
    #else
        #define EASTL_DEBUG 0
    #endif
#endif



#endif // Header include guard







