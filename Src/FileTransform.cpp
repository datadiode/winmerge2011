#include "StdAfx.h"
#include "dllpstub.h"
#include "paths.h"
#include "Merge.h"
#include "MainFrm.h"
#include "Environment.h"
#include "Common/RegKey.h"
#include "Common/SettingStore.h"
#include "Common/WindowPlacement.h"
#include "FileTransform.h"
#include "UniMarkdownFile.h"

static HWND NTAPI AllocConsoleHidden(LPCTSTR lpTitle)
{
	// Silently launch a ping.exe 127.0.0.1 and attach to its console.
	// This gives us control over the console window's initial visibility.
	STARTUPINFO si;
	ZeroMemory(&si, sizeof si);
	PROCESS_INFORMATION pi;
	si.cb = sizeof si;
	si.lpTitle = const_cast<LPTSTR>(lpTitle);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	TCHAR path[MAX_PATH];
	GetSystemDirectory(path, MAX_PATH);
	PathAppend(path, _T("ping.exe -n 2 127.0.0.1"));
	if (CreateProcess(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		Sleep(200);
		AttachConsole(pi.dwProcessId);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	return ::GetConsoleWindow();
}

static BOOL WINAPI ConsoleCtrlHandler(DWORD CtrlType)
{
	if (HWND hWnd = GetConsoleWindow())
	{
		CWindowPlacement wp;
		if (GetWindowPlacement(hWnd, &wp))
		{
			CRegKeyEx rk = SettingStore.GetSectionKey(_T("Settings"));
			wp.RegWrite(rk.GetKey(), _T("ConsoleWindowPlacement"));
		}
		FreeConsole();
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
	return TRUE;
}

static void NTAPI ShowConsoleWindow(int showCmd)
{
	if (HWND hWnd = GetConsoleWindow())
	{
		ShowWindow(hWnd, showCmd);
	}
	else if (HWND hWnd = AllocConsoleHidden(_T("WinMerge")))
	{
		SetConsoleCtrlHandler(::ConsoleCtrlHandler, TRUE);
		if (HMENU hMenu = GetSystemMenu(hWnd, FALSE))
		{
			DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
		}
		CWindowPlacement wp;
		CRegKeyEx rk = SettingStore.GetSectionKey(_T("Settings"));
		if (wp.RegQuery(rk.GetKey(), _T("ConsoleWindowPlacement")))
		{
			wp.showCmd = showCmd;
			SetWindowPlacement(hWnd, &wp);
		}
		else
		{
			ShowWindow(hWnd, showCmd);
		}
	}
}

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
	virtual bool WriteString(LPCTSTR line, size_t length)
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
