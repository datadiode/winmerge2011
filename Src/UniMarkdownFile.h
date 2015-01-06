/**
 *  @file UniMarkdownFile.h
 *
 *  @brief Declaration of UniMarkdownFile class.
 */
#include "Common/UniFile.h"
#include "FileTransform.h"

class CMarkdown;

/**
 * @brief XML file reader class.
 */
class UniMarkdownFile : public UniMemFile
{
public:
	UniMarkdownFile();
	static UniFile *CreateInstance(PackingInfo *packingInfo)
	{
		packingInfo->textType = _T("xml");
		return new UniMarkdownFile;
	}
	virtual bool OpenReadOnly(LPCTSTR filename);
	virtual void Close();
	virtual bool ReadString(String &line, String &eol, bool *lossy);
private:
	void Move();
	String maketstring(LPCSTR lpd, int len);
	int m_depth;
	bool m_bMove;
	LPBYTE m_transparent;
	CMarkdown *m_pMarkdown;
};
