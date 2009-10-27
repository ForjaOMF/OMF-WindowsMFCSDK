// Localizame.h : header file
//

#ifndef _LOCALIZAME_
#define _LOCALIZAME_

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Clase CLocalizame

class CLocalizame : public CObject
{
	CInternetSession*	m_pSession;

	CString	m_csCookie;


	CString ReadData(CHttpFile* pFile);
	CString GetCookies(CString csHeaders);

	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

// Construction
public:
	CLocalizame();	// Constructor
	~CLocalizame();

	bool Login(CString csLogin, CString csPwd);
	CString Locate(CString csNumero);
	void Authorize(CString csNumero);
	void Unauthorize(CString csNumero);
	void Logout();
};

#endif // _LOCALIZAME_
