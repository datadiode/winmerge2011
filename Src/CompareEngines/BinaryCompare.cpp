/**
 * @file  BinaryCompare.cpp
 *
 * @brief Implementation file for BinaryCompare
 */
#include "StdAfx.h"
#include <io.h>
#include "FileLocation.h"
#include "CompareOptions.h"
#include "DiffContext.h"
#include "BinaryCompare.h"

using namespace CompareEngines;

static const size_t CMPBUFF = 0x040000;

/**
 * @brief Default constructor.
 */
BinaryCompare::BinaryCompare(const CDiffContext *pCtxt)
	: DIFFOPTIONS(pCtxt->m_options), m_pCtxt(pCtxt)
{
	m_osfhandle[0] = NULL;
	m_osfhandle[1] = NULL;
}

/**
 * @brief Set filedata.
 * @param [in] items Count of filedata items to set.
 * @param [in] data File data.
 * @note Path names are set by SetPaths() function.
 */
void BinaryCompare::SetFileData(int items, file_data *data)
{
	// We support only two files currently!
	ASSERT(items == 2);
	m_osfhandle[0] = reinterpret_cast<HANDLE>(_get_osfhandle(data[0].desc));
	m_osfhandle[1] = reinterpret_cast<HANDLE>(_get_osfhandle(data[1].desc));
	m_st_size[0] = data[0].stat.st_size;
	m_st_size[1] = data[1].stat.st_size;
}

/**
 * @brief Compare two specified files, byte-by-byte
 * @param [in] location FileLocation
 * @return DIFFCODE
 */
unsigned BinaryCompare::CompareFiles()
{
	if (m_st_size[0] != m_st_size[1])
		return DIFFCODE::DIFF;
	if (m_osfhandle[0] == m_osfhandle[1])
		return DIFFCODE::SAME;
	char buff[2][CMPBUFF];
	DWORD size[2] = { CMPBUFF, CMPBUFF };
	do
	{
		if (m_pCtxt->ShouldAbort())
			return DIFFCODE::CMPABORT;
		if (!ReadFile(m_osfhandle[0], buff[0], CMPBUFF, &size[0], NULL) ||
			!ReadFile(m_osfhandle[1], buff[1], CMPBUFF, &size[1], NULL))
			return DIFFCODE::CMPERR;
		if ((size[0] != size[1]) || (memcmp(buff[0], buff[1], size[0]) != 0))
			return DIFFCODE::DIFF;
	} while (size[0] == CMPBUFF);
	return DIFFCODE::SAME;
}
