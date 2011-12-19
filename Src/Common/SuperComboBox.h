#pragma once

/////////////////////////////////////////////////////////////////////////////
// HSuperComboBox subclass

class HSuperComboBox : public HComboBox
{
public:
	void SetAutoComplete(INT nSource);
	HEdit *GetEditControl()
	{
		return reinterpret_cast<HEdit *>(SendMessage(CBEM_GETEDITCONTROL));
	}
	HComboBox *GetComboControl()
	{
		return reinterpret_cast<HComboBox *>(SendMessage(CBEM_GETCOMBOCONTROL));
	}
	int AddString(LPCTSTR lpszItem);
	int InsertString(int nIndex, LPCTSTR lpszItem);
	void SaveState(LPCTSTR szRegSubKey, UINT nMaxItems = 20);
	void LoadState(LPCTSTR szRegSubKey, UINT nMaxItems = 20);
	void AutoCompleteFromLB(int nIndexFrom = -1);
	void AdjustDroppedWidth();
	void EnsureSelection();
protected:
	int FindString(int nIndexStart, LPCTSTR lpszString);
};

/////////////////////////////////////////////////////////////////////////////
