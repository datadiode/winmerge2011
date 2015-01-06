/** 
 * @file  FileVersion.h
 *
 * @brief Declaration file for FileVersion
 */
#pragma once

/**
 * @brief A class that contains file version information.
 * This class contains file version information that Windows allows
 * file to have (file version, product version).
 */
class FileVersion
{
public:
	DWORD m_versionMS; //*< File version most significant dword. */
	DWORD m_versionLS; //*< File version least significant dword. */
	FileVersion(): m_versionMS(0), m_versionLS(0) { }
	void Clear() { m_versionMS = m_versionLS = 0; }
	String GetVersionString() const;
};
