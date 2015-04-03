/** 
 * @file OptionsMgr.h
 *
 * @brief Option accessors to support existing OptionsMgr coding style
 *
 */
#pragma once

#include "Common/coretypes.h"

class IOptionDef : public SINGLE_LIST_ENTRY
{
public:
	VARTYPE const type;
	LPCTSTR const name;
	virtual void Reset() = 0;
	virtual bool IsDefault() const = 0;
	void Parse(LPCTSTR);
	void LoadOption(HKEY);
	LSTATUS SaveOption(HKEY);
	LSTATUS SaveOption();
	// InitOptions() - first Reset(), then LoadOption() and / or SaveOption()
	static void InitOptions(HKEY loadkey, HKEY savekey);
	static int ExportOptions(LPCTSTR filename);
	static int ImportOptions(LPCTSTR filename);
protected:
	static IOptionDef *First;
	union UOptionPtr
	{
		void *p;
		bool *pBool;
		BYTE *pByte;
		DWORD *pDWord;
		String *pString;
		LogFont *pLogFont;
	};
	IOptionDef(VARTYPE type, LPCTSTR name)
		: type(type), name(name)
	{
		Next = First;
		First = this;
	}
	template<class T> static VARTYPE typeOf();
	template<> VARTYPE typeOf<BYTE>() { return VT_UI1; }
	template<> VARTYPE typeOf<int>() { return VT_I4; }
	template<> VARTYPE typeOf<unsigned int>() { return VT_UI4; }
	template<> VARTYPE typeOf<long>() { return VT_I4; }
	template<> VARTYPE typeOf<unsigned long>() { return VT_UI4; }
	template<> VARTYPE typeOf<bool>() { return VT_BOOL; }
	template<> VARTYPE typeOf<String>() { return VT_BSTR; }
	template<> VARTYPE typeOf<LogFont>() { return VT_FONT; }
};

template<class T>
class COptionDef : public IOptionDef
{
public:
	T curValue;
	T const defValue;
	COptionDef(LPCTSTR name, T value)
		: IOptionDef(typeOf<T>(), name), defValue(value)
	{
		std::swap(curValue, value);
	}
	void Reset()
	{
		curValue = defValue;
	}
	bool IsDefault() const
	{
		return curValue == defValue;
	}
};

#include "OptionsDef.h"

class COptionsMgr
{
public:
	template<class T>
	static const T &Get(const COptionDef<T> &opt)
	{
		return opt.curValue;
	}
	template<class T>
	static void Set(COptionDef<T> &opt, T value)
	{
		std::swap(opt.curValue, value);
	}
	template<class T>
	static void Reset(COptionDef<T> &opt)
	{
		opt.Reset();
	}
	template<class T>
	static const T &GetDefault(const COptionDef<T> &opt)
	{
		return opt.defValue;
	}
	template<class T>
	static LSTATUS SaveOption(COptionDef<T> &opt, T value)
	{
		std::swap(opt.curValue, value);
		return opt.SaveOption();
	}
};
