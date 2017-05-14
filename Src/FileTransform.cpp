#include "StdAfx.h"
#include "Merge.h"
#include "MainFrm.h"
#include "ConsoleWindow.h"
#include "FileTransform.h"
#include "UniMarkdownFile.h"
#include "codepage_detect.h"

/**
 * @brief Plugin file reader class.
 */
class UniPluginFile : public UniLocalFile
{
public:
	UniPluginFile(PackingInfo *packingInfo)
	{
		LPCTSTR moniker = packingInfo->pluginMoniker.c_str();
		OException::Check(
			CoGetObject(moniker, NULL, IID_IDispatch, reinterpret_cast<void **>(&m_spFactoryDispatch)));
		CMyDispId DispId;
		DispId.Call(m_spFactoryDispatch,
			CMyDispParams<1>().Unnamed(static_cast<IMergeApp *>(theApp.m_pMainWnd)), DISPATCH_PROPERTYPUTREF);
		if (LPCTSTR query = StrChr(moniker, _T('?')))
		{
			if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"Arguments")))
			{
				OException::Check(DispId.Call(m_spFactoryDispatch,
					CMyDispParams<1>().Unnamed(query + 1), DISPATCH_PROPERTYPUT));
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
		if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"Encoding")))
		{
			CMyVariant var;
			OException::Check(DispId.Call(m_spFactoryDispatch,
				CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
			OException::Check(var.ChangeType(VT_BSTR));
			V_BSTR(&var) = reinterpret_cast<HString *>(V_BSTR(&var))->Oct(CP_UTF8)->B;
			if (char *const name = reinterpret_cast<char *>(V_BSTR(&var)))
			{
				if (EncodingInfo const *encodinginfo = LookupEncoding(name))
				{
					m_unicoding = NEITHER;
					m_codepage = encodinginfo->GetCodePage();
				}
			}
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
		if (SUCCEEDED(DispId.Init(m_spFactoryDispatch, L"TextType")))
		{
			CMyVariant var;
			OException::Check(DispId.Call(m_spFactoryDispatch,
				CMyDispParams<0>().Unnamed, DISPATCH_PROPERTYGET, &var));
			OException::Check(var.ChangeType(VT_BSTR));
			packingInfo->textType = V_BSTR(&var);
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
					CMyDispParams<1>().Unnamed(packingInfo->pluginOptions),
					DISPATCH_PROPERTYPUTREF));
			}
		}
		OException::Check(
			m_OpenTextFile.Init(m_spFactoryDispatch, L"OpenTextFile"));
		if (SUCCEEDED(m_CreateTextFile.Init(m_spFactoryDispatch, L"CreateTextFile")))
			packingInfo->canWrite = true;
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
		// Convert BSTR to String
		if (m_unicoding == NEITHER)
		{
			V_BSTR(&var) = reinterpret_cast<HString *>(V_BSTR(&var))->Oct()->B;
			UINT const len = SysStringByteLen(V_BSTR(&var));
			ucr::maketstring(line, reinterpret_cast<char *>(V_BSTR(&var)), len, m_codepage, lossy);
		}
		else
		{
			line.assign(V_BSTR(&var), SysStringLen(V_BSTR(&var)));
		}
		// Don't let EOL chars exist in the midst of lines and cause crashes
		String::iterator p = line.begin();
		String::iterator const q = line.end();
		if ((p = std::remove_if(p, q, LineInfo::IsEol)) != q)
		{
			line.erase(p, q);
			*lossy = true;
		}
		if (*lossy)
			++m_txtstats.nlosses;
		eol = _T("\r\n");
		return true;
	}
	virtual bool WriteString(LPCTSTR line, stl_size_t length)
	{
		OException::Check(m_Write.Call(m_spStreamDispatch,
			CMyDispParams<1>().Unnamed(line, length), DISPATCH_METHOD));
		return true;
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
				CMyDispParams<1>().Unnamed(filename), DISPATCH_METHOD, &var));
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
	virtual bool OpenCreate(LPCTSTR filename)
	{
		CMyVariant var;
		OException::Check(
			m_CreateTextFile.Call(m_spFactoryDispatch,
			CMyDispParams<1>().Unnamed(filename), DISPATCH_METHOD, &var));
		OException::Check(var.ChangeType(VT_DISPATCH));
		m_spStreamDispatch = V_DISPATCH(&var);
		if (m_spStreamDispatch == NULL)
			return false;
		OException::Check(
			m_Write.Init(m_spStreamDispatch, L"Write"));
		return true;
	}

private:
	CMyComPtr<IDispatch> m_spFactoryDispatch;
	CMyDispId m_OpenTextFile;
	CMyDispId m_CreateTextFile;
	CMyComPtr<IDispatch> m_spStreamDispatch;
	CMyDispId m_ReadLine;
	CMyDispId m_Write;
	CMyDispId m_AtEndOfStream;
};

UniFile *PackingInfo::Default(PackingInfo *)
{
	return new UniMemFile;
}

UniFile *PackingInfo::XML(PackingInfo *packingInfo)
{
	packingInfo->textType = _T("xml");
	packingInfo->disallowMixedEOL = true;
	return new UniMarkdownFile;
}

UniFile *PackingInfo::Plugin(PackingInfo *packingInfo)
{
	packingInfo->disallowMixedEOL = true;
	return new UniPluginFile(packingInfo);
}
