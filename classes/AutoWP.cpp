// AutoWP.cpp : implementation file
//

#include "stdafx.h"
#include "AutoWP.h"

#include <afxinet.h>
#include <wininet.h>

CAutoWP::CAutoWP()
{
	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,SessionFlags);

	CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322)";
	char strUserAgent[255];
	strcpy(strUserAgent,csUserAgent);
	m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
}

CAutoWP::~CAutoWP()
{
	m_pSession->Close();
	delete m_pSession;
	m_pSession=NULL;
}

CString CAutoWP::ReadData(CHttpFile* pFile)
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

CString CAutoWP::GetCookies(CString csHeaders)
{
	CString csCookie;
	int nPosRet=0;
	CString csLine;
	while((csHeaders.GetLength()) && (nPosRet!=-1))
	{
		nPosRet=csHeaders.Find(_T("\r\n"),0);
		if(nPosRet!=-1)
			csLine=csHeaders.Left(nPosRet);
		else
			csLine=csHeaders;

		int nPosLocation=csLine.Find("Set-Cookie:");
		if(nPosLocation!=-1)
		{
			CString csCookieAux=csLine.Mid(nPosLocation+11);
			csCookieAux.TrimLeft();
			csCookieAux.TrimRight();

			csCookieAux.Replace("; Path=/","");
			csCookieAux.Replace("; Domain=.movistar.es","");

			if(csCookie.IsEmpty())
				csCookie+=csCookieAux;
			else
			{
				csCookie+="; ";
				csCookie+=csCookieAux;
			}
		}

		csHeaders=csHeaders.Mid(nPosRet+1);
	}

	return csCookie;
}

CString CAutoWP::GetLocation(CString csHeaders)
{
	int nPosRet=0;
	CString csLine;
	while((csHeaders.GetLength()) && (nPosRet!=-1))
	{
		nPosRet=csHeaders.Find(_T("\r\n"),0);
		if(nPosRet!=-1)
			csLine=csHeaders.Left(nPosRet);
		else
			csLine=csHeaders;

		int nPosLocation=csLine.Find("Location:");
		if(nPosLocation!=-1)
		{
			CString csLoc=csLine.Mid(nPosLocation+9);
			csLoc.TrimLeft();
			csLoc.TrimRight();
			return csLoc;
		}

		csHeaders=csHeaders.Mid(nPosRet+1);
	}

	return "";
}

UINT CAutoWP::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CAutoWP::GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CAutoWP::PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CAutoWP::GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

CString CAutoWP::URLEncode(CString csOriginal)
{
	CString csEncoded=csOriginal;

	csEncoded.Replace("&","%26");
	csEncoded.Replace("/","%2F");
	csEncoded.Replace(":","%3A");
	csEncoded.Replace("=","%3D");
	csEncoded.Replace("?","%3F");
	csEncoded.Replace(" ","+");

	csEncoded.Replace("á","%E1");
	csEncoded.Replace("é","%E9");
	csEncoded.Replace("í","%ED");
	csEncoded.Replace("ñ","%F1");
	csEncoded.Replace("ó","%F3");
	csEncoded.Replace("ú","%FA");
	csEncoded.Replace("ü","%FC");

	csEncoded.Replace("Á","%C1");
	csEncoded.Replace("É","%C9");
	csEncoded.Replace("Í","%CD");
	csEncoded.Replace("Ñ","%D1");
	csEncoded.Replace("Ó","%D3");
	csEncoded.Replace("Ú","%DA");
	csEncoded.Replace("Ü","%DC");

	return csEncoded;
}

// Sends a WapPush message to the user's own number
// Input	csLogin: String with user's telephone number
//			csPwd: String with user's password
//			csURL: String with message URL
//			csText: String with message text
// Returns:	Code received from service
UINT CAutoWP::SendAutoWP(CString csLogin, CString csPwd, CString csURL, CString csText)
{
	CString csData;

	CString csCookie;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	CString csEncodedURL=URLEncode(csURL);
	CString csEncodedText=URLEncode(csText);

	csData.Format("TME_USER=%s&TME_PASS=%s&WAP_Push_URL=%s&WAP_Push_Text=%s",csLogin,csPwd,csEncodedURL,csEncodedText);

    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Accept-Encoding: gzip, deflate\r\n"
				"Host: open.movilforum.com\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
				"Connection: Keep-Alive\r\n";

	UINT nCode=PostHTTP("http://open.movilforum.com/apis/autowap",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

	return atoi(csRetData);
}
