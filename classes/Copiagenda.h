// Copiagenda.h : header file
//

#ifndef _COPIAGENDA_
#define _COPIAGENDA_

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Clase CCopiagenda

class CCopiagenda : public CObject
{
	CInternetSession*	m_pSession;


	CString ReadData(CHttpFile* pFile);
	CString GetCookies(CString csHeaders);
	CString GetLocation(CString csHeaders);

	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

	UINT PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);
	UINT GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData);

	CStringArray* ParseLine(CString csLine);
	CMapStringToOb* ParseResponse(CString csCode);

// Construction
public:
	CCopiagenda();	// Constructor
	~CCopiagenda();

	CMapStringToOb* RetrieveContacts(CString csLogin, CString csPwd);
	CStringArray* SearchByName(CString csName, CMapStringToOb* pmsoAddressBook);
};

#endif // _COPIAGENDA_
