// MMSSender.h : header file
//

#ifndef _MMS_SENDER_
#define _MMS_SENDER_

#define OBJ_BUFFER_SIZE	65536*4-1

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Clase CMMSSender

class CMMSSender : public CObject
{
	CInternetSession*	m_pSession;

    CString		m_csLogin;
    CString		m_csUser;
    CString		m_csCookie;
    CString		m_csServer;


	CString ReadData(CHttpFile* pFile);
	CString GetCookies(CString csHeaders);
	CString GetLocation(CString csHeaders);

	CString GetParameter(CString csURL,CString csParam);

	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

// Construction
public:
	CMMSSender();	// Constructor
	~CMMSSender();

	CString Login(CString csLogin, CString csPwd);

	void InsertImage(CString csObjName, CString csObjPath); // Obsolete.
	void InsertImage(CString csObjPath);

	void InsertAudio(CString csObjName, CString csObjPath); // Obsolete.
	void InsertAudio(CString csObjPath);

	void InsertVideo(CString csObjName, CString csObjPath); // Obsolete.
	void InsertVideo(CString csObjPath);

	bool SendMessage(CString csSubject, CString csDest, CString csMsg);
	void Logout();
};

#endif // _MMS_SENDER_
