#include "Common/MyCom.h"

template<class ISuper>
class CMyDispatch : public ISuper
{
protected:
	CMyComPtr<ITypeInfo> m_spTypeInfo;
	CMyDispatch(REFIID iid = __uuidof(ISuper))
	{
		TCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, _countof(path));
		CMyComPtr<ITypeLib> spTypeLib;
		OException::Check(::LoadTypeLib(path, &spTypeLib));
		OException::Check(spTypeLib->GetTypeInfoOfGuid(iid, &m_spTypeInfo));
	}
public:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void **ppv)
	{
		static const QITAB rgqit[] = 
		{   
			QITABENT(CMyDispatch, IDispatch),
			{ 0 }
		};
		return QISearch(this, rgqit, iid, ppv);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		return 1;
	}
	STDMETHOD_(ULONG, Release)()
	{
		return 1;
	}
	// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID, ITypeInfo **ppTInfo)
	{
		(*ppTInfo = m_spTypeInfo)->AddRef();
		return S_OK;
	}
	STDMETHOD(GetIDsOfNames)(REFIID, LPOLESTR *rgNames, UINT cNames, LCID, DISPID *rgDispId)
	{
		return ::DispGetIDsOfNames(m_spTypeInfo, rgNames, cNames, rgDispId);
	}
	STDMETHOD(Invoke)(DISPID dispid, REFIID, LCID, WORD wFlags,
		DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
	{
		HRESULT hr = DISP_E_EXCEPTION;
		try
		{
			hr = ::DispInvoke(static_cast<ISuper *>(this), m_spTypeInfo,
				dispid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
		}
		catch (OException *e)
		{
			pExcepInfo->wCode = 1001;
			::SysReAllocString(&pExcepInfo->bstrDescription, e->msg);
			m_spTypeInfo->GetDocumentation(-1, &pExcepInfo->bstrSource, NULL, NULL, NULL);
			delete e;
		}
		return hr;
	}
};

template<class ISuper>
class CScriptable
	: public CMyDispatch<ISuper>
	, public IElementBehavior
	, public IElementBehaviorFactory
{
protected:
	CScriptable(REFIID iid = __uuidof(ISuper)) : CMyDispatch<ISuper>(iid)
	{
	}
public:
	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void **ppv)
	{
		static const QITAB rgqit[] = 
		{   
			QITABENT(CScriptable, IDispatch),
			QITABENT(CScriptable, IElementBehavior),
			QITABENT(CScriptable, IElementBehaviorFactory),
			{ 0 }
		};
		return QISearch(this, rgqit, iid, ppv);
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		return 1;
	}
	STDMETHOD_(ULONG, Release)()
	{
		return 1;
	}
	// IElementBehavior
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite)
	{
		return S_OK;
	}
	STDMETHOD(Notify)(LONG lEvent, VARIANT *pVar)
	{
		return S_OK;
	}
	STDMETHOD(Detach)()
	{
		return S_OK;
	}
};
