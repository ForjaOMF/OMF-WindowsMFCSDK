// AutoWP.h : header file
//

#ifndef _AutoWP_
#define _AutoWP_

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Clase CAutoWP

class CAutoWP : public CObject
{
	CInternetSession*	m_pSession;


	CString ReadData(CHttpFile* pFile);
	CString GetCookies(CString csHeaders);
	CString GetLocation(CString csHeaders);

	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

	UINT PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

	CString URLEncode(CString csOriginal);

// Construcción
public:
	CAutoWP();	// Constructor
	~CAutoWP();

	UINT SendAutoWP(CString csLogin, CString csPwd, CString csURL, CString csText);
};

#endif // _AutoWP_
