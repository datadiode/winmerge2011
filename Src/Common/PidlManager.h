/*
 * Copyright (c) 2017 Jochen Neubeck
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/**
 * @file PidlManager.h
 *
 * @brief PIDL management functions
 * @credits https://github.com/dwmkerr/sharpshell was quite instructive.
 */
class PidlManager
{
public:
	// Free a PIDL
	static void Free(LPITEMIDLIST pidl)
	{
		::CoTaskMemFree(pidl);
	}
	// Get the size of a PIDL, exclusive of the zero terminator
	static UINT GetSize(LPCITEMIDLIST pidl)
	{
		UINT cb = 0;
		if (LPCBYTE const p = reinterpret_cast<LPCBYTE>(pidl))
		{
			LPCBYTE q = p;
			while (USHORT const cb = reinterpret_cast<LPCITEMIDLIST>(q)->mkid.cb)
				q += cb;
			cb = static_cast<UINT>(q - p);
		}
		return cb;
	}
	// Combine two PIDLs
	static LPITEMIDLIST Combine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
	{
		UINT const cb1 = GetSize(pidl1);
		UINT const cb2 = GetSize(pidl2);
		LPBYTE const p = static_cast<LPBYTE>(::CoTaskMemAlloc(cb1 + cb2 + 2));
		if (p)
		{
			::CopyMemory(p, pidl1, cb1);
			::CopyMemory(p + cb1, pidl2, cb2);
			reinterpret_cast<LPITEMIDLIST>(p + cb1 + cb2)->mkid.cb = 0;
		}
		return reinterpret_cast<LPITEMIDLIST>(p);
	}
	// Move a shell allocated PIDL to task memory
	static LPITEMIDLIST ReAlloc(LPITEMIDLIST p)
	{
		LPITEMIDLIST q = Combine(p, NULL);
		IMalloc *pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			pMalloc->Free(p);
			pMalloc->Release();
		}
		return q;
	}
};
