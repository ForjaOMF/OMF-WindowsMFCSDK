// SMS20.h : header file
//

#ifndef _SMS20_
#define _SMS20_

#include <msxml2.h>

class CInternetSession;
class CHttpFile;

/////////////////////////////////////////////////////////////////////////////
// Class CSMS20Contact

class CSMS20Contact : public CObject
{
	CString m_csUserID;
	CString m_csAlias;
	bool m_bPresent;
public:
	CSMS20Contact();
	CSMS20Contact(CString csUserID, CString csAlias, bool bPresent=false);	// Constructor
	~CSMS20Contact();

	void SetUserID(CString csUserID){m_csUserID=csUserID;};
	void SetAlias(CString csAlias){m_csAlias=csAlias;};
	void MakePresent(bool bPresent){m_bPresent=bPresent;};

	CString GetUserID(){return m_csUserID;};
	CString GetAlias(){return m_csAlias;};
	bool IsPresent(){return m_bPresent;};
};

/////////////////////////////////////////////////////////////////////////////
// Class CSMS20Helper

class CSMS20Helper : public CObject
{
	CString SearchValue(CString csData, CString csTag, UINT& nEndPos);
public:
	CSMS20Helper();

	CString ASCII2UTF8(CString csText);
	CString UTF82ASCII(CString csUTF8Text);

	CMapStringToString* GetContactPresence(CString csData);
	CString SearchAuthorizePresence(CString csData);
	CString SearchTransactionId(CString csData);
	CString SearchMessage(CString csData);
};

/////////////////////////////////////////////////////////////////////////////
// Class CSMS20

class CSMS20 : public CObject
{
	CInternetSession*	m_pSession;

	UINT m_nTransId;

    CString m_csSessionID;
    CString m_csMyAlias;

	// Headers and URL are always the same except in Login
	CString m_csHeaders;
	CString m_csURL;

	CString SearchValue(CString csData, CString csTag, UINT& nEndPos);

	CString GetAlias(CString csData, CString csLogin);
	CMapStringToOb* GetContactList(CString csDatos);
	CMapStringToOb* GetPresenceList(CString csDatos);

	CString ReadData(CHttpFile* pFile);
	UINT PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData);

public:
	CSMS20();	// Constructor
	~CSMS20();

	CString GetMyAlias(){return m_csMyAlias;};

	CString Login(CString csLog, CString csPassw);
	CMapStringToOb* Connect(CString csLog, CString csNickname);

    CString Polling();

	CString AddContact(CString csLog, CString csContact);
	void AuthorizeContact(CString csUser, CString csTransaction);
	void DeleteContact(CString csLog, CString csContact);

	void SendMessage(CString csLog, CString csDestination, CString csMessage);

	void Disconnect();
};

#endif // _SMS20_
