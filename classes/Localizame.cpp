// Localizame.cpp : implementation file
//

#include "stdafx.h"
#include "Localizame.h"

#include <afxinet.h>
#include <wininet.h>

CLocalizame::CLocalizame()
{
	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,SessionFlags);

	CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322; .NET CLR 2.0.50727)";
	char strUserAgent[255];
	strcpy(strUserAgent,csUserAgent);
	m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
}

CLocalizame::~CLocalizame()
{
	if(m_pSession)
	{
		m_pSession->Close();
		delete m_pSession;
		m_pSession=NULL;
	}
}

CString CLocalizame::ReadData(CHttpFile* pFile)
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

CString CLocalizame::GetCookies(CString csHeaders)
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

			csCookieAux.Replace("; path=/","");

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

UINT CLocalizame::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode=0;

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

UINT CLocalizame::GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
{
	UINT nCode=0;

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


// Performs login to the server
// Input:	csLogin=String with user's telephone number
//			csPwd=String with user's password
// Returns: true if the login was succesfully done
bool CLocalizame::Login(CString csLogin, CString csPwd)
{
	CString csHeaders;
	CString csData;

	CString csRetHeaders;
	CString csRetData;

	m_pSession->SetCookie("http://www.localizame.movistar.es/login.do","JSESSIONID","");

	csData.Format("usuario=%s&clave=%s&submit.x=36&submit.y=6",csLogin,csPwd);
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept-Language: es\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n";
	UINT nCode=PostHTTP("http://www.localizame.movistar.es/login.do",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

	m_csCookie=GetCookies(csRetHeaders);

	// We have to access to this page or we will not be allowed to authorize any other user.
    csHeaders = "Accept-Language: es\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n"
				"Referer: http://www.localizame.movistar.es/login.do";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	nCode=GetHTTP("http://www.localizame.movistar.es/nuevousuario.do",csHeaders,csRetHeaders,csRetData);

	if(m_csCookie.IsEmpty())
		return false;

	return true;
}

// Performs location search of a user
// Input:	csNumber=String with user's telephone number
// Returns:	Location text or error text
CString CLocalizame::Locate(CString csNumber)
{
	CString csHeaders;
	CString csData;

	CString csRetHeaders;
	CString csRetData;

	csData.Format("telefono=%s",csNumber);
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept-Language: es\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	UINT nCode=PostHTTP("http://www.localizame.movistar.es/buscar.do",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

	int nNumPos=csRetData.Find(csNumber);
	if(nNumPos!=-1)
	{
		int nMetrosPos=csRetData.Find("metros.",nNumPos+9);
		if(nMetrosPos!=-1)
			return csRetData.Mid(nNumPos,nMetrosPos-nNumPos+8);
	}
	else
	{
		int nErrorPos=csRetData.Find("Error de localizacion");
		if(nErrorPos!=-1)
			return csRetData.Mid(nErrorPos,21);
	}

	return csRetData;
}

// Authorizes another user to locate us
// Input:	csNumber=String with user's telephone number
void CLocalizame::Authorize(CString csNumber)
{
	CString csHeaders;
	CString csData;

	CString csRetHeaders;
	CString csRetData;

	CString csURL;
	csURL.Format("http://www.localizame.movistar.es/insertalocalizador.do?telefono=%s&submit.x=40&submit.y=5",csNumber);
    csHeaders = "Accept-Language: es\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n"
				"Referer: http://www.localizame.movistar.es/buscalocalizadorespermisos.do\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	UINT nCode=GetHTTP(csURL,csHeaders,csRetHeaders,csRetData);
}

// Unauthorizes another user to locate us
// Input:	csNumber=String with user's telephone number
void CLocalizame::Unauthorize(CString csNumber)
{
	CString csHeaders;
	CString csData;

	CString csRetHeaders;
	CString csRetData;

	CString csURL;
	csURL.Format("http://www.localizame.movistar.es/borralocalizador.do?telefono=%s&submit.x=44&submit.y=8",csNumber);
    csHeaders = "Accept-Language: es\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n"
				"Referer: http://www.localizame.movistar.es/buscalocalizadorespermisos.do\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	UINT nCode=GetHTTP(csURL,csHeaders,csRetHeaders,csRetData);
}

// Logs out from server
void CLocalizame::Logout()
{
	CString csHeaders;
	CString csData;

	CString csCabeceras;
	CString csDatos;

    csHeaders = "Accept-Language: es\r\n"
				"Host: www.localizame.movistar.es\r\n"
				"Accept-Encoding: identity\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	UINT nCode=GetHTTP("http://www.localizame.movistar.es/logout.do",csHeaders,csCabeceras,csDatos);

	m_csCookie="";
}
