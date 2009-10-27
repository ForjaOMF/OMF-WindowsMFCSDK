// SMSSender.h : header file
//

#ifndef _SMS_SENDER_
#define _SMS_SENDER_

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Clase CSMSSender

class CSMSSender : public CObject
{
	CInternetSession*	m_pSession;


	CString ReadData(CHttpFile* pFile);

	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

	UINT PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

// Construcción
public:
	CSMSSender();	// Constructor
	~CSMSSender();

	CString SendMessage(CString csLogin, CString csPwd, CString csDest, CString csMsg);
};

#endif // _SMS_SENDER_
