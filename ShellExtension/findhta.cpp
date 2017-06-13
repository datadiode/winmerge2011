#include "StdAfx.h"
#include <oleacc.h>
#include <mshtml.h>

EXTERN_C HWND FindHTA(LPCWSTR name)
{
    static TCHAR const cls[] = TEXT("HTML Application Host Window Class");
    UINT const WM_HTML_GETOBJECT = RegisterWindowMessage(TEXT("WM_HTML_GETOBJECT"));
    HWND hwnd = NULL;
    while (name != NULL && (hwnd = FindWindowEx(NULL, hwnd, cls, NULL)) != NULL)
    {
        HWND const hwndIE = FindWindowEx(hwnd, NULL, TEXT("Internet Explorer_Server"), NULL);
        DWORD_PTR result = 0;
        SendMessageTimeout(hwndIE, WM_HTML_GETOBJECT, 0, 0, SMTO_ABORTIFHUNG, 1000, &result);
        if (result)
        {
            IHTMLDocument2 *pHtmlDoc = NULL;
            HRESULT hr = ObjectFromLresult(result, IID_IHTMLDocument2, 0, (void**)&pHtmlDoc);
            if (SUCCEEDED(hr))
            {
                IHTMLWindow2 *pHtmlWnd = NULL;
                hr = pHtmlDoc->get_parentWindow(&pHtmlWnd);
                if (SUCCEEDED(hr))
                {
                    BSTR bstr = NULL;
                    pHtmlWnd->get_name(&bstr);
                    if (lstrcmpiW(bstr, name) == 0)
                        name = NULL;
                    SysFreeString(bstr);
                    pHtmlWnd->Release();
                }
                pHtmlDoc->Release();
            }
        }
    }
    return hwnd;
}
