// SMSReceiver.h : header file
//

#ifndef _SMS_RECEIVER_
#define _SMS_RECEIVER_

#define POP3_STATUS_INIT			0
#define POP3_STATUS_LOGINSENT		1
#define POP3_STATUS_LOGIN			2
#define POP3_STATUS_PASSSENT		3
#define POP3_STATUS_PASS			4
#define POP3_STATUS_STATSENT		5
#define POP3_STATUS_STAT			6
#define POP3_STATUS_RETRSENT		7
#define POP3_STATUS_RETR			8
#define POP3_STATUS_QUITSENT		9

typedef void (* tagCBMessages)(DWORD dwCookie, CStringList& slMessages);


#include <afxsock.h>

#define FSOCK_CONNECT	1
#define FSOCK_RECEIVE	2
#define FSOCK_ACCEPT	3
#define FSOCK_CLOSE		4

typedef void (* tagCallbackFn)(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);

class CMySocket : public CAsyncSocket
{
// Attributes
public:

// Operations
public:
	CMySocket();
	CMySocket(tagCallbackFn pCallbackFunction,DWORD dwCookie);
	virtual ~CMySocket();

	void SetCookie(DWORD dwCookie){m_dwCookie=dwCookie;};
	void SetCallbackFn(tagCallbackFn pCallbackFunction){m_pCallbackFunction=pCallbackFunction;};

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FWapSocket)
	public:
	virtual void OnReceive(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(FWapSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	DWORD			m_dwCookie;
	tagCallbackFn	m_pCallbackFunction;
};


/////////////////////////////////////////////////////////////////////////////
// Class CSMSReceiver

class CSMSReceiver : public CObject
{
	void OnReceiveData(char* pBufData,DWORD dwLenData,int nErrorCode);
	void OnConnectSocket(int nErrorCode);
	void OnCloseSocket(int nErrorCode);

	static void OnEvent(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);

	void EvolStatusInit(CString csData);
	void EvolStatusLoginSent(CString csData);
	void EvolStatusPassSent(CString csData);
	void EvolStatusStatSent(CString csData);
	void EvolStatusRetrSent(CString csData);
	void EvolStatusQuitSent(CString csData);

	CString UTF82ASCII(CString csTextoUTF8);
	CString ConvertText(CString csText, bool bUTF8);

	CString ReadSMS(CString csMensaje);

	CString m_csServer;
	CString m_csLogin;
	CString m_csPassword;

	CMySocket* m_pSocket;
	bool m_bConnected;

	UINT m_nStatus;

	UINT m_nNumMessages;
	UINT m_nMessageNum;

	CStringList	m_slMessages;

	DWORD			m_dwCookie;
	tagCBMessages	m_pCallbackFunction;

// Construction
public:
	CSMSReceiver();
	CSMSReceiver(tagCBMessages pCallbackFunction, DWORD dwCookie, CString csServer, CString csLogin, CString csPassword);
	~CSMSReceiver();

	void Start();
};

#endif // _SMS_RECEIVER_
