// VideoCallReceiver.cpp: archivo de implementación
//

#include "stdafx.h"
#include "VideoCallReceiver.h"

#include "SIPSocket.h"
#include "Frame263.h"
#include "WriteAvi.h"
#include "MD5Checksum.h"
#include "decoder\tmndec.h"


CVideoCallReceiver::CVideoCallReceiver(CString csLogin, CString csPassword, CString csLocalIP, CString csPath, DWORD dwCookie)
{
	m_csLogin=csLogin;
	m_csPassword=csPassword;
	m_csLocalIP=csLocalIP;
	m_csPath=csPath;

	m_dwCookie=dwCookie;

	m_bRegistered=false;
	m_nRegisterAttempts=1;

	m_pStartCall=NULL;
	m_pEndCall=NULL;

	bool bRes;
	m_pSocket=new CSIPSocket(OnEvent,(DWORD)this);
	if(csLocalIP.IsEmpty())
	{
		bRes=m_pSocket->Create(5061,SOCK_DGRAM,FD_READ|FD_WRITE);

		CString csHost;
		UINT nPort;
		m_pSocket->GetSockNameEx(csHost, nPort);

		hostent* pRemoteHost = gethostbyname(csHost.GetBuffer());
		if(pRemoteHost)
		{
			in_addr addr;
			addr.s_addr = *(u_long *) pRemoteHost->h_addr_list[0];
			m_csLocalIP=inet_ntoa(addr);
		}
	}
	else
	{
		bRes=m_pSocket->Create(5061,SOCK_DGRAM,FD_READ|FD_WRITE,m_csLocalIP);
	}
}

CVideoCallReceiver::~CVideoCallReceiver()
{
	if(m_pSocket)
	{
		m_pSocket->Close();
		delete m_pSocket;
		m_pSocket=NULL;
	}
}

void CVideoCallReceiver::OnReceiveData(char* pBufData,DWORD dwLenData,int nErrorCode)
{
	char strData[65000];
	memset(strData,0,65000);
	memcpy(strData,pBufData,dwLenData);

	CString csData=strData;
	int nPos401=csData.Find("401 Unauthorized");
	if (nPos401!=-1)
	{
		if(m_bRegistered)
			ReRegister(csData,0);
		else
			ReRegister(csData,3600);
	}

	int nPos200=csData.Find("200 OK");
	if (nPos200!=-1)
	{
		if(!m_bRegistered)
			AfxMessageBox("Conectado");
		m_bRegistered = true;
		SetTimer(AfxGetMainWnd()->m_hWnd,(UINT)this,45000,TimerProc);
		// Registrado!!
	}

	if(csData.Left(6)=="INVITE")
	{
		AcceptInvitation(csData);
	}
	else if(csData.Left(3)=="ACK")
	{
		SendMultimediaData(csData);
	}
	else if(csData.Left(3)=="BYE")
	{
		AckBye(csData);
	}
}


void CVideoCallReceiver::OnEvent(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	CVideoCallReceiver* pThis=(CVideoCallReceiver*)dwCookie;
	if(pThis)
	{
		switch(nCode)
		{
			case FSOCK_RECEIVE:
				pThis->OnReceiveData(pBufData,dwLenData,nErrorCode);
				break;
			case FSOCK_ACCEPT:
				break;
			case FSOCK_CLOSE:
				//pThis->OnCloseSocket(nErrorCode);
				break;
		}
	}
}

void CVideoCallReceiver::RegisterAttempt()
{
	CString csRegister1;
	csRegister1.Format(
		"REGISTER sip:195.76.180.160 SIP/2.0\r\n"
		"Via: SIP/2.0/UDP %s:5061;rport;branch=z9hG4bK21898\r\n"
		"From: <sip:%s@movistar.es>;tag=13939\r\n"
		"To: <sip:%s@movistar.es>\r\n"
		"Call-ID: 9979@%s\r\n"
		"CSeq: %d REGISTER\r\n"
		"Contact: <sip:%s@%s:5061>\r\n"
		"Max-Forwards: 70\r\n"
		"Expires: 3600\r\n"
		"Allow-Events: presence\r\n"
		"Event: registration\r\n"
		"User-Agent: Intellivic/PC\r\n"
		"Allow: ACK, BYE, CANCEL, INVITE, MESSAGE, OPTIONS, REFER, PRACK\r\n"
		"Content-Length: 0\r\n\r\n", m_csLocalIP,
			m_csLogin,
			m_csLogin,
			m_csLocalIP,
			m_nRegisterAttempts++,
			m_csLogin, m_csLocalIP);

	if(m_pSocket)
		m_pSocket->SendTo((void*)csRegister1.GetBuffer(0),csRegister1.GetLength(), 5060, "195.76.180.160");
}

void CVideoCallReceiver::AcceptInvitation(CString csInvitation)
{
	CString csCallId=GetCallId(csInvitation);

	COleDateTime odtNow=COleDateTime::GetCurrentTime();
	CString csStartTime=odtNow.Format("%Y-%m-%d %H.%M.%S");

	CString csFrom=GetFrom(csInvitation);
	CString csBranch=GetBranch(csInvitation);
	CString csFromTag=GetFromTag(csInvitation);

	CString csTrying;
	csTrying.Format(
		"SIP/2.0 100 Trying\r\n"
		"Via: SIP/2.0/UDP 195.76.180.160:5060;branch=%s\r\n"
		"From: <sip:%s@movistar.es>;tag=%s\r\n"
		"To: <sip:%s@movistar.es:5060;user=phone>\r\n"
		"Call-Id: %s\r\n"
		"CSeq: 1 INVITE\r\n"
		"Content-Length: 0\r\n\r\n", csBranch, csFrom, csFromTag, m_csLogin, csCallId);
	if(m_pSocket)
		m_pSocket->SendTo((void*)csTrying.GetBuffer(0),csTrying.GetLength(), 5060, "195.76.180.160");

	int nToTag=10000*rand()/RAND_MAX;

	CString csRinging;
	csRinging.Format(
		"SIP/2.0 180 Ringing\r\n"
		"Via: SIP/2.0/UDP 195.76.180.160:5060;branch=%s\r\n"
		"From: <sip:%s@movistar.es>;tag=%s\r\n"
		"To: <sip:%s@movistar.es:5060;user=phone>;tag=%d\r\n"
		"Call-Id: %s\r\n"
		"CSeq: 1 INVITE\r\n"
		"Contact: <sip:%s@%s:5061>\r\n"
		"Content-Length: 0\r\n\r\n", csBranch, csFrom, csFromTag, m_csLogin, nToTag, csCallId, m_csLogin, m_csLocalIP);
	if(m_pSocket)
		m_pSocket->SendTo((void*)csRinging.GetBuffer(0),csRinging.GetLength(), 5060, "195.76.180.160");

	// Añadir llamada a la lista
	CVideoCall* pVCall=new CVideoCall(csStartTime, csFrom, m_csLocalIP, m_csPath, csCallId, csBranch, csFromTag, nToTag);
	if(m_pStartCall)
		m_pStartCall(m_dwCookie,pVCall);
	m_msoCalls.SetAt(csCallId,pVCall);
	pVCall->CreateSockets();

	int nRnd1=30000*rand()/RAND_MAX;
	int nRnd2=30000*rand()/RAND_MAX;
	CString csContent;
	csContent.Format(
		"v=0\r\n"
		"o=- %d %d IN IP4 %s\r\n"
		"s=-\r\n"
		"c=IN IP4 %s\r\n"
		"t=0 0\r\n"
		"m=audio %d RTP/AVP 101 99 0 8 104\r\n"
		"a=rtpmap:101 speex/16000\r\n"
		"a=fmtp:101 vbr=on;mode=6\r\n"
		"a=rtpmap:99 speex/8000\r\n"
		"a=fmtp:99 vbr=on;mode=3\r\n"
		"a=rtpmap:0 PCMU/8000\r\n"
		"a=rtpmap:8 PCMA/8000\r\n"
		"a=rtpmap:104 telephone-event/8000\r\n"
		"a=fmtp:104 0-15\r\n"
		"m=video %d RTP/AVP 97 34\r\n"
		"a=rtpmap:97 MP4V-ES/90000\r\n"
		"a=fmtp:97 profile-level-id=1\r\n"
		"a=rtpmap:34 H263/90000\r\n"
		"a=fmtp:34 QCIF=2 SQCIF=2/MaxBR=560\r\n"
		,nRnd1,nRnd2,m_csLocalIP,m_csLocalIP,pVCall->GetAudioPort(),pVCall->GetVideoPort());

	CString csSDP;
	csSDP.Format(
		"SIP/2.0 200 OK\r\n"
		"Via: SIP/2.0/UDP 195.76.180.160:5060;branch=%s\r\n"
		"From: <sip:%s@movistar.es>;tag=%s\r\n"
		"To: <sip:%s@movistar.es:5060;user=phone>;tag=%d\r\n"
		"Call-Id: %s\r\n"
		"CSeq: 1 INVITE\r\n"
		"Contact: <sip:%s@%s:5061>\r\n"
		"User-Agent: Intellivic/PC\r\n"
		"Supported: replaces\r\n"
		"Allow: ACK, BYE, CANCEL, INVITE, OPTIONS\r\n"
		"Content-Type: application/sdp\r\n"
		"Accept: application/sdp, application/media_control+xml, application/dtmf-relay\r\n"
		"Content-Length: %d\r\n\r\n%s", csBranch, csFrom, csFromTag, m_csLogin, nToTag
			, csCallId, m_csLogin, m_csLocalIP, csContent.GetLength(),csContent);
	if(m_pSocket)
		m_pSocket->SendTo((void*)csSDP.GetBuffer(0),csSDP.GetLength(), 5060, "195.76.180.160");

	//Register(3600);
}

void CVideoCallReceiver::SendMultimediaData(CString csAckData)
{
	CString csCallId=GetCallId(csAckData);

	CVideoCall* pVCall=NULL;
	m_msoCalls.Lookup(csCallId,(CObject*&)pVCall);
	if(pVCall)
		pVCall->SendMultimediaData(csAckData);
}

void CVideoCallReceiver::ReRegister(CString cs401, long lExpires)
{
	int nPosNonce=cs401.Find("nonce=\"");
	if(nPosNonce!=-1)
	{
		int nPosNonceEnd=cs401.Find("\"",nPosNonce+8);
		if(nPosNonceEnd!=-1)
			m_csNonce=cs401.Mid(nPosNonce+7,nPosNonceEnd-nPosNonce-7);
		else
			m_csNonce=cs401.Left(nPosNonce+7);
	}

	Register(lExpires);
}

void CVideoCallReceiver::Register(long lExpires)
{
	CMD5Checksum* pmd5=new CMD5Checksum();

	CString csLine1;
	csLine1.Format("%s:movistar.es:%s",m_csLogin,m_csPassword);
	CString csHA1=pmd5->GetMD5((BYTE*)csLine1.GetBuffer(0),csLine1.GetLength());
	
	CString csLine2="REGISTER:sip:195.76.180.160";
	CString csHA2=pmd5->GetMD5((BYTE*)csLine2.GetBuffer(0),csLine2.GetLength());

	CString csLine3;
	csLine3.Format("%s:%s:00000001:473:auth:%s",csHA1,m_csNonce,csHA2);
	CString csResponse=pmd5->GetMD5((BYTE*)csLine3.GetBuffer(0),csLine3.GetLength());

	delete pmd5;

	int nTag=32000*rand()/RAND_MAX;
	int nCallId=10000*rand()/RAND_MAX;

	CString csRegister2;
	csRegister2.Format(
		"REGISTER sip:195.76.180.160 SIP/2.0\r\n"
		"Via: SIP/2.0/UDP %s:5061;rport;branch=z9hG4bK23080\r\n"
		"From: <sip:%s@movistar.es>;tag=%d\r\n"
		"To: <sip:%s@movistar.es>\r\n"
		"Call-ID: %d@%s\r\n"
		"CSeq: %d REGISTER\r\n"
		"Contact: <sip:%s@%s:5061>\r\n"
		"Authorization: digest username=\"%s\", realm=\"movistar.es\", nonce=\"%s\", uri=\"sip:195.76.180.160\", response=\"%s\", algorithm=md5, cnonce=\"473\", qop=auth, nc=00000001\r\n"
		"Max-Forwards: 70\r\n"
		"Expires: %d\r\n"
		"Allow-Events: presence\r\n"
		"Event: registration\r\n"
		"User-Agent: Intellivic/PC\r\n"
		"Allow: ACK, BYE, CANCEL, INVITE, MESSAGE, OPTIONS, REFER, PRACK\r\n"
		"Content-Length: 0\r\n\r\n", m_csLocalIP,
			m_csLogin, nTag,
			m_csLogin,
			nCallId, m_csLocalIP,
			m_nRegisterAttempts++,
			m_csLogin, m_csLocalIP,
			m_csLogin, m_csNonce, csResponse,
			lExpires);

	if(m_pSocket)
		m_pSocket->SendTo((void*)csRegister2.GetBuffer(0),csRegister2.GetLength(), 5060, "195.76.180.160");
}

void CVideoCallReceiver::AckBye(CString csBye)
{
	CString csCallId=GetCallId(csBye);

	CVideoCall* pVCall=NULL;
	m_msoCalls.Lookup(csCallId,(CObject*&)pVCall);
	if(pVCall)
	{
		if(m_pEndCall)
			m_pEndCall(m_dwCookie,pVCall);
		m_msoCalls.RemoveKey(csCallId);
		pVCall->Terminate();

		CString csBranch=GetBranch(csBye);

		CString csByeAck;
		csByeAck.Format(
			"SIP/2.0 200 OK\r\n"
			"Via: SIP/2.0/UDP 195.76.180.160:5060;branch=%s\r\n"
			"From: <sip:%s@movistar.es>;tag=%s\r\n"
			"To: <sip:%s@movistar.es:5060;user=phone>;tag=%d\r\n"
			"Call-Id: %s\r\n"
			"CSeq: 2 Bye\r\n"
			"Content-Length: 0\r\n\r\n", csBranch, pVCall->GetFrom(), pVCall->GetFromTag(), m_csLogin, pVCall->GetToTag(), pVCall->GetCallId());
		if(m_pSocket)
			m_pSocket->SendTo((void*)csByeAck.GetBuffer(0),csByeAck.GetLength(), 5060, "195.76.180.160");

		delete pVCall;
		pVCall=NULL;
	}

	//Register(3600);
}

void CVideoCallReceiver::Unregister()
{
	KillTimer(AfxGetMainWnd()->m_hWnd,(UINT)this);
	Register(0);
}

CString CVideoCallReceiver::GetCallId(CString csData)
{
	CString csCallId;
	int nPosCallId=csData.Find("Call-ID: ");
	if(nPosCallId!=-1)
	{
		int nPosCallIdEnd=csData.Find("\r\n",nPosCallId+10);
		if(nPosCallIdEnd!=-1)
			csCallId=csData.Mid(nPosCallId+9,nPosCallIdEnd-nPosCallId-9);
		else
			csCallId=csData.Left(nPosCallId+9);
	}

	return csCallId;
}

CString CVideoCallReceiver::GetBranch(CString csData)
{
	CString csBranch;
	int nPosBranch=csData.Find("branch=");
	if(nPosBranch!=-1)
	{
		int nPosBranchEnd=csData.Find("\r\n",nPosBranch+8);
		if(nPosBranchEnd!=-1)
			csBranch=csData.Mid(nPosBranch+7,nPosBranchEnd-nPosBranch-7);
		else
			csBranch=csData.Left(nPosBranch+7);
	}

	return csBranch;
}

CString CVideoCallReceiver::GetFrom(CString csData)
{
	CString csFrom;
	int nPosFrom=csData.Find("From: <sip:");
	if(nPosFrom!=-1)
	{
		int nPosFromEnd=csData.Find("@",nPosFrom+11);
		if(nPosFromEnd!=-1)
			csFrom=csData.Mid(nPosFrom+11,nPosFromEnd-nPosFrom-11);
		else
			csFrom=csData.Left(nPosFrom+11);
	}

	return csFrom;
}

CString CVideoCallReceiver::GetFromTag(CString csData)
{
	CString csFrom=GetFrom(csData);

	CString csSearch;
	csSearch.Format("From: <sip:%s@movistar.es>;tag=",csFrom);

	CString csFromTag;
	int nPosTag=csData.Find(csSearch);
	if(nPosTag!=-1)
	{
		int nPosTagEnd=csData.Find("\r\n",nPosTag+csSearch.GetLength()+1);
		if(nPosTagEnd!=-1)
			csFromTag=csData.Mid(nPosTag+csSearch.GetLength(),nPosTagEnd-nPosTag-csSearch.GetLength());
		else
			csFromTag=csData.Left(nPosTag+csSearch.GetLength());
	}

	return csFromTag;
}

void WINAPI CVideoCallReceiver::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if(uMsg==WM_TIMER)
	{
		// Lanzar Registro
		CVideoCallReceiver* pThis=(CVideoCallReceiver*)idEvent;
		KillTimer(AfxGetMainWnd()->m_hWnd,(UINT)pThis);
		pThis->Register(3600);
		SetTimer(AfxGetMainWnd()->m_hWnd,idEvent,45000,TimerProc);
	}
}
