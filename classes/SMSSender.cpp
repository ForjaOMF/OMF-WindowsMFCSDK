// SMSSender.cpp : implementation file
//

#include "stdafx.h"
#include "SMSSender.h"

#include <afxinet.h>
#include <wininet.h>

CSMSSender::CSMSSender()
{
	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,SessionFlags);

	CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322)";
	char strUserAgent[255];
	strcpy(strUserAgent,csUserAgent);
	m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
}

CSMSSender::~CSMSSender()
{
	m_pSession->Close();
	delete m_pSession;
	m_pSession=NULL;
}

CString CSMSSender::ReadData(CHttpFile* pFile)
{
	CString csResponse;
	DWORD dwLength=5000;
	char TempBuffer[5001];
	char* strBuf=(char*)malloc(dwLength);
	memset(strBuf,0,dwLength);

	UINT nCount=pFile->Read(TempBuffer,dwLength);

	DWORD dwFileSize=0;
	while(nCount)
	{
		memcpy((void*)(strBuf+dwFileSize),(void*)TempBuffer,nCount);
		dwFileSize+=nCount;
		strBuf=(char*)realloc(strBuf,dwFileSize+dwLength);
		memset((void*)(strBuf+dwFileSize),0,dwLength);
		nCount=pFile->Read(TempBuffer,dwLength);
	}

	csResponse=strBuf;
	free(strBuf);

	return csResponse;
}

UINT CSMSSender::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode;

	DWORD dwService;
	CString csServer;
	CString csPath;
	INTERNET_PORT nPort;
	CString csUser;
	CString csPwd;
	AfxParseURLEx(csURL,dwService,csServer,csPath,nPort,csUser,csPwd,0);

	CHttpConnection* pConnection = NULL;
	pConnection=m_pSession->GetHttpConnection(csServer,INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION, nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_AUTO_REDIRECT);

		if(pFile)
		{
			try
			{
				bResult=pFile->SendRequest(csHeaders, strPostData, lDataSize);
				if(bResult)
				{
					CString csCode;
					pFile->QueryInfo(HTTP_QUERY_STATUS_CODE,csCode);
					nCode=atoi(csCode);

					CString csHeaders;
					pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF,csRetHeaders);

					csRetData=ReadData(pFile);
				}
			}
			catch(CInternetException* e)
			{
			}

			pFile->Close();
			delete pFile;
		}
		pConnection->Close();
		delete pConnection;
	}

	return nCode;
}

UINT CSMSSender::GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode;

	DWORD dwService;
	CString csServer;
	CString csPath;
	INTERNET_PORT nPort;
	CString csUser;
	CString csPwd;
	AfxParseURLEx(csURL,dwService,csServer,csPath,nPort,csUser,csPwd,0);

	CHttpConnection* pConnection = NULL;
	pConnection=m_pSession->GetHttpConnection(csServer,INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION,nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_AUTO_REDIRECT);

		if(pFile)
		{
			try
			{
				bResult=pFile->SendRequest(csHeaders, NULL, 0);
				if(bResult)
				{
					CString csCode;
					pFile->QueryInfo(HTTP_QUERY_STATUS_CODE,csCode);
					nCode=atoi(csCode);

					pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF,csRetHeaders);

					csRetData=ReadData(pFile);
				}
			}
			catch(CInternetException* e)
			{
			}

			pFile->Close();
			delete pFile;
		}

		pConnection->Close();
		delete pConnection;
	}

	return nCode;
}

UINT CSMSSender::PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode;

	DWORD dwService;
	CString csServer;
	CString csPath;
	INTERNET_PORT nPort;
	CString csUser;
	CString csPwd;
	AfxParseURLEx(csURL,dwService,csServer,csPath,nPort,csUser,csPwd,0);

	CHttpConnection* pConnection = NULL;
	pConnection=m_pSession->GetHttpConnection(csServer,INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_SECURE,nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_SECURE);

		if(pFile)
		{
			try
			{
				bResult=pFile->SendRequest(csHeaders, strPostData, lDataSize);
				if(bResult)
				{
					CString csCode;
					pFile->QueryInfo(HTTP_QUERY_STATUS_CODE,csCode);
					nCode=atoi(csCode);

					pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF,csRetHeaders);

					csRetData=ReadData(pFile);
				}
			}
			catch(CInternetException* e)
			{
			}

			pFile->Close();
			delete pFile;
		}

		pConnection->Close();
		delete pConnection;
	}

	return nCode;
}

UINT CSMSSender::GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode;

	DWORD dwService;
	CString csServer;
	CString csPath;
	INTERNET_PORT nPort;
	CString csUser;
	CString csPwd;
	AfxParseURLEx(csURL,dwService,csServer,csPath,nPort,csUser,csPwd,0);

	CHttpConnection* pConnection = NULL;
	pConnection=m_pSession->GetHttpConnection(csServer,INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_SECURE,nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_EXISTING_CONNECT|INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_KEEP_CONNECTION|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_SECURE);

		if(pFile)
		{
			try
			{
				bResult=pFile->SendRequest(csHeaders, NULL, 0);
				if(bResult)
				{
					CString csCode;
					pFile->QueryInfo(HTTP_QUERY_STATUS_CODE,csCode);
					nCode=atoi(csCode);

					pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF,csRetHeaders);

					csRetData=ReadData(pFile);
				}
			}
			catch(CInternetException* e)
			{
			}

			pFile->Close();
			delete pFile;
		}

		pConnection->Close();
		delete pConnection;
	}

	return nCode;
}


// Performs HTTP transactions against opensms server
// Input:	csLogin=String with user's telephone number
//			csPwd=String with user's password
//			csDest=String with destination telephone number
//			csMsg=String with message text
CString CSMSSender::SendMessage(CString csLogin, CString csPwd, CString csDest, CString csMsg)
{
	CString csData;

	CString csCookie;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	csData.Format("TM_ACTION=AUTHENTICATE&TM_LOGIN=%s&TM_PASSWORD=%s&to=%s&message=%s",csLogin,csPwd,csDest,csMsg);
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Accept-Encoding: gzip, deflate\r\n"
				"Host: opensms.movistar.es\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
				"Connection: Keep-Alive\r\n";

	UINT nCode=PostHTTPS("https://opensms.movistar.es/aplicacionpost/loginEnvio.jsp",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

	return csRetData;
}
