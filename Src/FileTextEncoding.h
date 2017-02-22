/**
 * @file  FileTextEncoding.h
 *
 * @brief Declaration of FileTextEncoding structure
 */
#pragma once

#include "unicoder.h"

/**
 * @brief Text encoding (eg, UTF-8, or CP-1252)
 */
class FileTextEncoding
{
public:
	int m_codepage; /**< 8bit codepage, if applicable, -1 is unknown or N/A */
	UNICODESET m_unicoding; /**< Unicode encoding. */
	bool m_bom; /**< Unicode byte marker */
	bool m_guessed; /**< Whether encoding was guessed from content */
	bool m_binary;

	FileTextEncoding();
	void Clear();
	void SetCodepage(int codepage);
	void SetUnicoding(UNICODESET unicoding);
	String GetName() const;

	static int Collate(FileTextEncoding const &, FileTextEncoding const &);

	static void UpdateDefaultCodepage(int cpDefaultMode, int customCodepage);
	static int GetDefaultCodepage();
	static int IsCodepageDBCS(int codepage);

private:
	static int m_defaultCodepage; // store the default codepage as specified by user in options
};
