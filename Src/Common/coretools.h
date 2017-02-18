/**
 * @file  coretools.h
 *
 * @brief Declaration file for Coretools.cpp
 */
#pragma once

/******** function protos ********/

LPSTR NTAPI EatPrefix(LPCSTR text, LPCSTR prefix);
LPCWSTR NTAPI EatPrefix(LPCWSTR text, LPCWSTR prefix);
LPCWSTR NTAPI EatPrefixTrim(LPCWSTR text, LPCWSTR prefix);

String GetModulePath(HMODULE hModule = NULL);
DWORD_PTR GetShellImageList();

int linelen(const char *string);

unsigned Unslash(unsigned codepage, char *string);

inline void Unslash(unsigned codepage, std::string &s)
{
	s.resize(Unslash(codepage, &s.front()));
}

unsigned RemoveLeadingZeros(char *string);

HANDLE NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, HANDLE *phReadPipe, WORD wShowWindow = SW_SHOWMINNOACTIVE);
DWORD NTAPI RunIt(LPCTSTR szExeFile, LPCTSTR szArgs, LPCTSTR szDir, WORD wShowWindow = SW_SHOWMINNOACTIVE);

void DecorateCmdLine(String &sDecoratedCmdLine, String &sExecutable);

template<typename IMAGE_NT_HEADERS>
LPVOID GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	BYTE *const pORG = reinterpret_cast<BYTE *>(reinterpret_cast<INT_PTR>(hModule) & ~1);
	if (pORG == reinterpret_cast<BYTE *>(hModule))
		return NULL; // failed to load or not loaded with LOAD_LIBRARY_AS_DATAFILE
	IMAGE_DOS_HEADER const *const pMZ = reinterpret_cast<IMAGE_DOS_HEADER const *>(pORG);
	IMAGE_NT_HEADERS const *const pPE = reinterpret_cast<IMAGE_NT_HEADERS const *>(pORG + pMZ->e_lfanew);
	if (pPE->FileHeader.SizeOfOptionalHeader != sizeof pPE->OptionalHeader)
		return NULL; // mismatched bitness
	IMAGE_SECTION_HEADER const *pSH = IMAGE_FIRST_SECTION(pPE);
	WORD nSH = pPE->FileHeader.NumberOfSections;
	DWORD offset = pPE->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	// Search for the section which contains the offset at hand
	while ((pSH->VirtualAddress > offset || pSH->VirtualAddress + pSH->Misc.VirtualSize <= offset) && --nSH)
		++pSH;
	if (nSH == 0)
		return NULL; // section not found
	BYTE const *const pRD = pORG + pSH->PointerToRawData - pSH->VirtualAddress;
	IMAGE_EXPORT_DIRECTORY const *const pEAT = reinterpret_cast<IMAGE_EXPORT_DIRECTORY const *>(pRD + offset);
	ULONG const *const rgAddressOfNames = reinterpret_cast<ULONG const *>(pRD + pEAT->AddressOfNames);
	DWORD lower = 0;
	DWORD upper = pEAT->NumberOfNames;
	int const cbProcName = lstrlenA(lpProcName) + 1;
	while (lower < upper)
	{
		DWORD const match = (upper + lower) >> 1;
		int const cmp = memcmp(pRD + rgAddressOfNames[match], lpProcName, cbProcName);
		if (cmp >= 0)
			upper = match;
		if (cmp <= 0)
			lower = match + 1;
	}
	if (lower <= upper)
		return NULL; // name not found
	WORD const *const rgNameIndexes = reinterpret_cast<WORD const *>(pRD + pEAT->AddressOfNameOrdinals);
	DWORD const *rgEntyPoints = reinterpret_cast<DWORD const *>(pRD + pEAT->AddressOfFunctions);
	pSH = IMAGE_FIRST_SECTION(pPE);
	nSH = pPE->FileHeader.NumberOfSections;
	offset = rgEntyPoints[rgNameIndexes[upper]];
	// Search for the section which contains the offset at hand
	while ((pSH->VirtualAddress > offset || pSH->VirtualAddress + pSH->Misc.VirtualSize <= offset) && --nSH)
		++pSH;
	if (nSH == 0)
		return NULL; // section not found
	return pORG + pSH->PointerToRawData - pSH->VirtualAddress + offset;
}
