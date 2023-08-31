/*****************************************************************************
* version.h file
*
* Written by Gary Webb
* Copyright September 26, 2000
*
* Use CVersionInfo to obtain information from the version resource of your
* application, or specify a valid path to another executable to obtain its
* version information.
*
* The implementation file is version.cpp lives in LibLinuxOS and LibWinOS
*****************************************************************************/
#ifndef CVersionInfo_Class_File
#define CVersionInfo_Class_File

#ifdef _WIN32
	#include <windows.h>
#endif
	#include <string>
	#include <list>
using namespace std;
//---------------------------------------------------------------------------

class CVersionInfo
{
public:
    CVersionInfo(void);
    ~CVersionInfo(void);
    bool SetFilePath(string szPath);
    bool SetLanguage(int nLanguage);
    int GetLanguageCount(void);
    string GetComments(void);
    string GetCompanyName(void);
    string GetFileDescription(void);
    string GetFileVersion(void);
    string GetInternalName(void);
    string GetLegalCopyright(void);
    string GetLegalTrademarks(void);
    string GetOriginalFilename(void);
    string GetProductName(void);
    string GetProductVersion(void);
    string GetPrivateBuild(void);
    string GetSpecialBuild(void);
	string GetFilePath() { return m_pszFilePath; };
private:
    bool m_bValidPath;
    bool m_bVersionInfoExists;
    string m_pszFilePath;
    int m_nLanguage;
    int m_nLangCount;
    list<string> m_lstLangCode;
    bool GetLanguageCodes(void);
    string GetData(string szData);
};

#endif
