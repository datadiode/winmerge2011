/** 
 * @file OptionsMgr.h
 *
 * @brief Option accessors to support existing OptionsMgr coding style
 *
 */
#pragma once

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
