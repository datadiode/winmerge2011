

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed Mar 23 13:51:37 2011
 */
/* Compiler settings for .\ShellExtension.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ShellExtension_h__
#define __ShellExtension_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IWinMergeShell_FWD_DEFINED__
#define __IWinMergeShell_FWD_DEFINED__
typedef interface IWinMergeShell IWinMergeShell;
#endif 	/* __IWinMergeShell_FWD_DEFINED__ */


#ifndef __WinMergeShell_FWD_DEFINED__
#define __WinMergeShell_FWD_DEFINED__

#ifdef __cplusplus
typedef class WinMergeShell WinMergeShell;
#else
typedef struct WinMergeShell WinMergeShell;
#endif /* __cplusplus */

#endif 	/* __WinMergeShell_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IWinMergeShell_INTERFACE_DEFINED__
#define __IWinMergeShell_INTERFACE_DEFINED__

/* interface IWinMergeShell */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IWinMergeShell;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("64200B75-83AE-464A-88C6-262E175BBA92")
    IWinMergeShell : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IWinMergeShellVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWinMergeShell * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWinMergeShell * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWinMergeShell * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWinMergeShell * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWinMergeShell * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWinMergeShell * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWinMergeShell * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IWinMergeShellVtbl;

    interface IWinMergeShell
    {
        CONST_VTBL struct IWinMergeShellVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWinMergeShell_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IWinMergeShell_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IWinMergeShell_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IWinMergeShell_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IWinMergeShell_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IWinMergeShell_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IWinMergeShell_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IWinMergeShell_INTERFACE_DEFINED__ */



#ifndef __SHELLEXTENSIONLib_LIBRARY_DEFINED__
#define __SHELLEXTENSIONLib_LIBRARY_DEFINED__

/* library SHELLEXTENSIONLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_SHELLEXTENSIONLib;

EXTERN_C const CLSID CLSID_WinMergeShell;

#ifdef __cplusplus

class DECLSPEC_UUID("4E716236-AA30-4C65-B225-D68BBA81E9C2")
WinMergeShell;
#endif
#endif /* __SHELLEXTENSIONLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


