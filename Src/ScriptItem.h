#pragma once

struct script_item
{
	script_item(LineFilterItem *item = NULL)
		: item(item)
		, filenameSpec(NULL)
		, object(NULL)
	{
	}
	HString *filenameSpec; /** Optional target filename patterns */
	LineFilterItem *item;
	IDispatch *object;
	CMyDispId Reset;
	CMyDispId ProcessLine;
	void dispose()
	{
		filenameSpec->Free();
		if (object)
			object->Release();
	}
};
