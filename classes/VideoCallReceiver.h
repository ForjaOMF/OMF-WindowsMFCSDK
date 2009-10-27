// VideoCallReceiver.h: archivo de encabezado
//

#include "VideoCall.h"

typedef void (* tagCBStartCall)(DWORD dwCookie, CVideoCall* pVCall);
typedef void (* tagCBEndCall)(DWORD dwCookie, CVideoCall* pVCall);

class CSIPSocket;
class CFrame263;

class CVideoCallReceiver : public CObject
{
// Construcción
public:
	CVideoCallReceiver(CString csLogin, CString csPassword, CString csLocalIP, CString csPath, DWORD dwCookie=0);
	virtual ~CVideoCallReceiver();

	void SetStartCallCB(tagCBStartCall pStartCall){m_pStartCall=pStartCall;};
	void SetEndCallCB(tagCBEndCall pEndCall){m_pEndCall=pEndCall;};

	void RegisterAttempt();
	void Unregister();

// Implementación
protected:

	CMapStringToOb	m_msoCalls;

	CSIPSocket* m_pSocket;

	CString m_csNonce;

	CString m_csPassword;
	CString m_csLogin;
	CString m_csLocalIP;
	CString m_csPath;

	DWORD	m_dwCookie;

	bool m_bRegistered;
	UINT	m_nRegisterAttempts;

	static void WINAPI TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	tagCBStartCall	m_pStartCall;
	tagCBEndCall	m_pEndCall;

	void Register(long lExpires);
	void ReRegister(CString cs401,long lExpires);
	void AcceptInvitation(CString csInvitation);
	void SendMultimediaData(CString csAckData);

	void OnReceiveData(char* pBufData,DWORD dwLenData,int nErrorCode);
	static void OnEvent(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);

	void AckBye(CString csBye);

	CString GetCallId(CString csData);
	CString GetBranch(CString csData);
	CString GetFrom(CString csData);
	CString GetFromTag(CString csData);
};
