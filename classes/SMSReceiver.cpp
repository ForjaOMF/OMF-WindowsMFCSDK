// SMSReceiver.cpp : implementation file
//

#include "stdafx.h"
#include "SMSReceiver.h"


// Class CMySocket

/////////////////////////////////////////////////////////////////////////////
// CMySocket
CMySocket::CMySocket() : CAsyncSocket()
{
	m_pCallbackFunction = NULL;
	m_dwCookie = 0;
}

CMySocket::CMySocket(tagCallbackFn pCallbackFunction,DWORD dwCookie) : CAsyncSocket()
{
	m_pCallbackFunction = pCallbackFunction;
	m_dwCookie = dwCookie;
}

CMySocket::~CMySocket()
{
}


/////////////////////////////////////////////////////////////////////////////
// CMySocket member functions

void CMySocket::OnReceive(int nErrorCode) 
{
	Sleep(400);

	BYTE data[65000];
	memset(data,0,65000);
	int nLong=0;
	int nReceived=0;
	do
	{
		nLong=Receive(&data[nReceived],65000);
		if(nLong>0)
			nReceived+=nLong;
	}
	while(nLong>0);

	if(m_pCallbackFunction)
		m_pCallbackFunction(m_dwCookie,FSOCK_RECEIVE,(char*)data,nReceived,nErrorCode);

	CAsyncSocket::OnReceive(nErrorCode);
}

void CMySocket::OnConnect(int nErrorCode) 
{
	if(m_pCallbackFunction)
		m_pCallbackFunction(m_dwCookie,FSOCK_CONNECT,NULL,0,nErrorCode);

	CAsyncSocket::OnConnect(nErrorCode);
}

void CMySocket::OnAccept(int nErrorCode) 
{
	if(m_pCallbackFunction)
		m_pCallbackFunction(m_dwCookie,FSOCK_ACCEPT,NULL,0,nErrorCode);

	CAsyncSocket::OnAccept(nErrorCode);
}

void CMySocket::OnClose(int nErrorCode) 
{
	if(m_pCallbackFunction)
		m_pCallbackFunction(m_dwCookie,FSOCK_CLOSE,NULL,0,nErrorCode);
}


// Class CSMSReceiver

CSMSReceiver::CSMSReceiver(tagCBMessages pCallbackFunction, DWORD dwCookie, CString csServer, CString csLogin, CString csPassword)
{
	m_csServer=csServer;
	m_csLogin=csLogin;
	m_csPassword=csPassword;

	m_pCallbackFunction=pCallbackFunction;
	m_dwCookie=dwCookie;

	m_pSocket=new CMySocket(OnEvent,(DWORD)this);
	bool bRes=m_pSocket->Create(0,SOCK_STREAM,FD_READ|FD_WRITE|FD_OOB|FD_ACCEPT|FD_CONNECT|FD_CLOSE);

	m_bConnected = false;

	m_nStatus = POP3_STATUS_INIT;

	m_nNumMessages = 0;
	m_nMessageNum = 0;
}

CSMSReceiver::~CSMSReceiver()
{
	delete m_pSocket;
}

void CSMSReceiver::Start()
{
	bool bRes=m_pSocket->Connect(m_csServer,110);
}

void CSMSReceiver::OnCloseSocket(int nErrorCode)
{
	m_bConnected=false;

	m_pSocket->Close();
}

void CSMSReceiver::OnConnectSocket(int nErrorCode)
{
	m_bConnected=true;
}

void CSMSReceiver::OnReceiveData(char* pBufData,DWORD dwLenData,int nErrorCode)
{
	char strData[65000];
	memset(strData,0,65000);
	memcpy(strData,pBufData,dwLenData);

	CString csData=strData;
	if(csData.Left(3)=="+OK")
	{
		switch(m_nStatus)
		{
			case POP3_STATUS_INIT:
				EvolStatusInit(csData);
				break;
			case POP3_STATUS_LOGINSENT:
				EvolStatusLoginSent(csData);
				break;
			case POP3_STATUS_LOGIN:
				break;
			case POP3_STATUS_PASSSENT:
				EvolStatusPassSent(csData);
				break;
			case POP3_STATUS_PASS:
				break;
			case POP3_STATUS_STATSENT:
				EvolStatusStatSent(csData);
				break;
			case POP3_STATUS_STAT:
				break;
			case POP3_STATUS_RETRSENT:
				EvolStatusRetrSent(csData);
				break;
			case POP3_STATUS_RETR:
				break;
			case POP3_STATUS_QUITSENT:
				EvolStatusQuitSent(csData);
				break;
		}
	}
}

void CSMSReceiver::OnEvent(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	CSMSReceiver* pThis=(CSMSReceiver*)dwCookie;
	if(pThis)
	{
		switch(nCode)
		{
			case FSOCK_CONNECT:
				pThis->OnConnectSocket(nErrorCode);
				break;
			case FSOCK_RECEIVE:
				pThis->OnReceiveData(pBufData,dwLenData,nErrorCode);
				break;
			case FSOCK_ACCEPT:
				break;
			case FSOCK_CLOSE:
				pThis->OnCloseSocket(nErrorCode);
				break;
		}
	}
}

void CSMSReceiver::EvolStatusInit(CString csData)
{
	if(csData.Left(3)=="+OK")
	{
		CString csLogin;
		csLogin.Format("USER %s\r\n",m_csLogin);
		m_pSocket->Send(csLogin.GetBuffer(0),csLogin.GetLength());

		m_nStatus = POP3_STATUS_LOGINSENT;
	}
}

void CSMSReceiver::EvolStatusLoginSent(CString csData)
{
	if(csData.Left(3)=="+OK")
	{
		m_nStatus = POP3_STATUS_LOGIN;

		CString csPass;
		csPass.Format("PASS %s\r\n",m_csPassword);
		m_pSocket->Send(csPass.GetBuffer(0),csPass.GetLength());

		m_nStatus = POP3_STATUS_PASSSENT;
	}
	else
		m_nStatus = POP3_STATUS_INIT;
}

void CSMSReceiver::EvolStatusPassSent(CString csData)
{
	if(csData.Left(3)=="+OK")
	{
		m_nStatus = POP3_STATUS_PASS;

		char strStat[]="STAT\r\n";
		m_pSocket->Send(strStat,strlen(strStat));

		m_nStatus = POP3_STATUS_STATSENT;
	}
	else
		m_nStatus = POP3_STATUS_INIT;
}

void CSMSReceiver::EvolStatusStatSent(CString csData)
{
	if(csData.Left(3)=="+OK")
	{
		m_nStatus = POP3_STATUS_STAT;

		CString csMessages=csData.Mid(4);
		int nSpcPos=csMessages.Find(" ");
		if(nSpcPos!=-1)
		{
			m_nNumMessages = atoi(csMessages.Left(nSpcPos));

			if(m_nNumMessages)
			{
				m_nMessageNum=1;

				char strRetr[50];
				memset(strRetr,0,50);
				sprintf(strRetr,"RETR %d\r\n",m_nMessageNum);
				m_pSocket->Send(strRetr,strlen(strRetr));

				m_nStatus = POP3_STATUS_RETRSENT;
			}
		}
		else
			m_nNumMessages = 0;
	}
	else
		m_nStatus = POP3_STATUS_STAT;
}

void CSMSReceiver::EvolStatusRetrSent(CString csData)
{
	if(csData.Left(3)=="+OK")
	{
		CString csFrom;
		CString csMensaje;
		CString csFromMensaje=ReadSMS(csData);
		if(!csFromMensaje.IsEmpty())
			m_slMessages.AddTail(csFromMensaje);

		m_nStatus = POP3_STATUS_RETR;

		m_nMessageNum++;
		if(m_nMessageNum<=m_nNumMessages)
		{
			// Solicitamos el siguiente mensaje
			char strRetr[20];
			memset(strRetr,0,20);
			sprintf(strRetr,"RETR %d\r\n",m_nMessageNum);
			m_pSocket->Send(strRetr,strlen(strRetr));

			m_nStatus = POP3_STATUS_RETRSENT;
		}
		else
		{
			CString csQuit;
			csQuit.Format("QUIT\r\n",m_csLogin);
			m_pSocket->Send(csQuit.GetBuffer(0),csQuit.GetLength());

			m_nStatus = POP3_STATUS_QUITSENT;
		}
	}
	else
		m_nStatus = POP3_STATUS_RETR;
}

void CSMSReceiver::EvolStatusQuitSent(CString csData)
{
	m_nStatus = POP3_STATUS_INIT;

	if(m_pCallbackFunction)
		m_pCallbackFunction(m_dwCookie, m_slMessages);
}

CString CSMSReceiver::ReadSMS(CString csMensaje)
{
	CString csMovil;
	CString csTexto;

	CString csData;
	CString csCabeceras;
	// Encuentro el final de cabeceras (primera línea en blanco)
	int nPosRNRN=csMensaje.Find("\r\n\r\n");
	if(nPosRNRN!=-1)
	{
		csCabeceras=csMensaje.Left(nPosRNRN);
		csData=csMensaje.Mid(nPosRNRN+4);
	}
	else
		csData=csMensaje;

	bool bUTF8=false;
	int nPosUTF8=csCabeceras.Find("UTF-8");
	if(nPosUTF8!=-1)
		bUTF8=true;

	int nPosMovil=csData.Find("Movil:");
	if(nPosMovil!=-1)
	{
		int nPosRN=csData.Find("\r\n",nPosMovil+6);
		if(nPosRN!=-1)
			csMovil=csData.Mid(nPosMovil+6,nPosRN-nPosMovil-6);
		else
			csMovil=csData.Mid(nPosMovil+6);
	}

	int nPosTexto=csData.Find("Texto:");
	if(nPosTexto!=-1)
	{
		int nPosRN=csData.Find("\r\n",nPosTexto+6);
		if(nPosRN!=-1)
			csTexto=csData.Mid(nPosTexto+6,nPosRN-nPosTexto-6);
		else
			csTexto=csData.Mid(nPosTexto+6);
	}

	csTexto=ConvertText(csTexto, bUTF8);

	if( (csMovil.IsEmpty()) || (csTexto.IsEmpty()))
		return "";

	return csMovil+"|"+csTexto;
}

CString CSMSReceiver::ConvertText(CString csTexto, bool bUTF8)
{
	CString csMensaje=csTexto;;

	BYTE bChar;
	CString csHex;
	char* pstrHex;
	CString csAscii;
	int nPosIgual=csMensaje.Find("=");
	while(nPosIgual!=-1)
	{
		csHex=csMensaje.Mid(nPosIgual,3);
		pstrHex=csHex.GetBuffer(csHex.GetLength());
		sscanf(pstrHex,"=%02X",&bChar);

		csAscii.Format("%c",bChar);
		csMensaje.Replace(csHex,csAscii);

		nPosIgual=csMensaje.Find("=");
	}

	if(bUTF8)
		csMensaje=UTF82ASCII(csMensaje);

	return csMensaje;
}

CString CSMSReceiver::UTF82ASCII(CString csTextoUTF8)
{
	char strTextoUTF8[256];
	strcpy(strTextoUTF8,csTextoUTF8);
	char strTexto[256];
	WCHAR wszTexto[256];
	// Pasamos el texto de UTF-8 a Unicode
	MultiByteToWideChar(CP_UTF8,0,strTextoUTF8,strlen(strTextoUTF8)+1,wszTexto,sizeof(wszTexto)/sizeof(wszTexto[0]));
	// Pasamos el texto de Unicode a UTF-8
	WideCharToMultiByte(CP_ACP,0,wszTexto,sizeof(wszTexto)/sizeof(wszTexto[0])+1,strTexto,256,NULL,NULL);
	CString csTexto=strTexto;

	return csTexto;
}
