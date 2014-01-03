#include "StdAfx.h"
#include "paths.h"
#include "Merge.h"
#include "MainFrm.h"
#include "Environment.h"
#include "ConsoleWindow.h"
#include "FileTransform.h"
#include "UniMarkdownFile.h"

/**
 * @brief Plugin file reader class.
 */
class UniPluginFile : public UniLocalFile
{
public:
	UniPluginFile(PackingInfo *packingInfo)
	{
		OException::Check(
			CoGetObject(packingInfo->pluginMoniker.c_str(), NULL, IID_IDispatch,
				reinterpret_cast<void **>(&m_spFactoryDispatch)));
		CMyDispId DispId;
		DispId.Call(m_spFactoryDispatch,
			CMyDispParams<1>().Unnamed[static_cast<IMergeApp *>(theApp.m_pMainWnd)], DISPATCH_PROPERTYPUTREF);
		if (LPCTSTR query = StrChr(packingInfo->pluginMoniker.c_str(), _T('?')))
		{
			if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"Arguments")))
			{
				OException::Check(DispId.Call(m_spFactoryDispatch,
					CMyDispParams<1>().Unnamed[query + 1], DISPATCH_PROPERTYPUT));
			}
		}
		if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"ShowConsole")))
		{
			CMyVariant var;
			OException::Check(DispId.Call(m_spFactoryDispatch,
				CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
			OException::Check(var.ChangeType(VT_UI2));
			ShowConsoleWindow(V_UI2(&var));
		}
		if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"ReadOnly")))
		{
			CMyVariant var;
			OException::Check(DispId.Call(m_spFactoryDispatch,
				CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
			OException::Check(var.ChangeType(VT_BOOL));
			packingInfo->readOnly = V_BOOL(&var) != VARIANT_FALSE;
		}
		else
		{
			// Disallow editing by default
			packingInfo->readOnly = true;
		}
		if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"Options")))
		{
			if (packingInfo->pluginOptions == NULL)
			{
				CMyVariant options;
				OException::Check(DispId.Call(m_spFactoryDispatch,
					CMyDispParams<0>().Unnamed,
					DISPATCH_PROPERTYGET, &options));
				OException::Check(options.ChangeType(VT_DISPATCH));
				packingInfo->pluginOptionsOwner = m_spFactoryDispatch;
				packingInfo->pluginOptions = V_DISPATCH(&options);
			}
			else
			{
				OException::Check(DispId.Call(m_spFactoryDispatch,
					CMyDispParams<1>().Unnamed[packingInfo->pluginOptions],
					DISPATCH_PROPERTYPUTREF));
			}
		}
		OException::Check(
			m_OpenTextFile.Init(m_spFactoryDispatch, L"OpenTextFile"));
	}
	static UniFile *CreateInstance(PackingInfo *packingInfo)
	{
		return new UniPluginFile(packingInfo);
	}
	virtual void ReadBom()
	{
	}
	virtual bool ReadString(String &line, String &eol, bool *lossy)
	{
		CMyVariant var;
		OException::Check(m_AtEndOfStream.Call(m_spStreamDispatch,
			CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
		OException::Check(var.ChangeType(VT_BOOL));
		if (V_BOOL(&var) != VARIANT_FALSE)
		{
			line.clear();
			eol.clear();
			return false;
		}
		OException::Check(m_ReadLine.Call(m_spStreamDispatch,
			CMyDispParams<0>().Unnamed, DISPATCH_METHOD, &var));
		OException::Check(var.ChangeType(VT_BSTR));
		line = V_BSTR(&var);
		eol = _T("\r\n");
		return true;
	}
	virtual bool WriteString(LPCTSTR, stl_size_t)
	{
		return false;
	}
	virtual void Close()
	{
		m_spStreamDispatch.Release();
	}
	virtual bool IsOpen() const
	{
		return m_spStreamDispatch != NULL;
	}
	virtual bool OpenReadOnly(LPCTSTR filename)
	{
		CMyVariant var;
		OException::Check(
			m_OpenTextFile.Call(m_spFactoryDispatch,
				CMyDispParams<1>().Unnamed[filename], DISPATCH_METHOD, &var));
		OException::Check(var.ChangeType(VT_DISPATCH));
		m_spStreamDispatch = V_DISPATCH(&var);
		if (m_spStreamDispatch == NULL)
			return false;
		OException::Check(
			m_ReadLine.Init(m_spStreamDispatch, L"ReadLine"));
		OException::Check(
			m_AtEndOfStream.Init(m_spStreamDispatch, L"AtEndOfStream"));
		return true;
	}

private:
	CMyComPtr<IDispatch> m_spFactoryDispatch;
	CMyDispId m_OpenTextFile;
	CMyComPtr<IDispatch> m_spStreamDispatch;
	CMyDispId m_ReadLine;
	CMyDispId m_AtEndOfStream;
};

void PackingInfo::SetXML()
{
	pfnCreateUniFile = UniMarkdownFile::CreateInstance;
	textType = _T("xml");
	disallowMixedEOL = true;
}

void PackingInfo::SetPlugin(LPCTSTR text)
{
	pfnCreateUniFile = UniPluginFile::CreateInstance;
	env_ResolveMoniker(pluginMoniker = text);
	disallowMixedEOL = true;
}
