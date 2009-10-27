// Copiagenda.cpp : implementation file
//

#include "stdafx.h"
#include "Copiagenda.h"

#include <afxinet.h>
#include <wininet.h>

CCopiagenda::CCopiagenda()
{
	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,SessionFlags);

	CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322)";
	char strUserAgent[255];
	strcpy(strUserAgent,csUserAgent);
	m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
}

CCopiagenda::~CCopiagenda()
{
	m_pSession->Close();
	delete m_pSession;
	m_pSession=NULL;
}

CString CCopiagenda::ReadData(CHttpFile* pFile)
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

CString CCopiagenda::GetCookies(CString csHeaders)
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

CString CCopiagenda::GetLocation(CString csHeaders)
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

UINT CCopiagenda::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CCopiagenda::GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CCopiagenda::PostHTTPS(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

UINT CCopiagenda::GetHTTPS(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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


CStringArray* CCopiagenda::ParseLine(CString csLine)
{
	CStringArray* psaContact = new CStringArray();

	csLine.Replace("\r","");
	csLine.Replace("\n","");

	int nPos=0;
	int nPosTab=csLine.Find("\t");
	while(nPosTab!=-1)
	{
		CString csDatum;
		if(nPosTab!=-1)
			csDatum=csLine.Mid(nPos,nPosTab-nPos+1);
		else
			csDatum=csLine.Mid(nPos);

		csDatum.Replace("\t","");
		csDatum.Replace("\"","");

		psaContact->Add(csDatum);

		nPos=nPosTab+1;
		nPosTab=csLine.Find("\t",nPos);
	}

	return psaContact;
}

CMapStringToOb* CCopiagenda::ParseResponse(CString csCode)
{
	CMapStringToOb* pmsoList = new CMapStringToOb();

	int nPos=csCode.Find("\r\n");
	int nPosRet=csCode.Find("\r\n",nPos+2);
	while(nPosRet!=-1)
	{
		CString csLine;
		if(nPosRet!=-1)
			csLine=csCode.Mid(nPos,nPosRet-nPos+1);
		else
			csLine=csCode.Mid(nPos);

		nPos=nPosRet+2;
		CStringArray* psaContact = ParseLine(csLine);

		if(psaContact)
			pmsoList->SetAt(psaContact->GetAt(3),psaContact);

		nPosRet=csCode.Find("\r\n",nPos);
	}

	return pmsoList;
}

// Performs contact list retrieval
// Input:	csLogin=String with user's telephone number
//			csPwd=String with user's password
// Output:	Contact list
CMapStringToOb* CCopiagenda::RetrieveContacts(CString csLogin, CString csPwd)
{
	CMapStringToOb* pmsoList=NULL;

	CString csData;

	CString csCookie;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	csData.Format("TM_ACTION=LOGIN&TM_LOGIN=%s&TM_PASSWORD=%s",csLogin,csPwd);
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Accept-Encoding: gzip, deflate\r\n"
				"Host: copiagenda.movistar.es\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
				"Connection: Keep-Alive\r\n";

	UINT nCode=PostHTTPS("https://copiagenda.movistar.es/cp/ps/Main/login/Agenda",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);
	if(nCode == 302)
	{
		csCookie=GetCookies(csRetHeaders);
		CString csURL=GetLocation(csRetHeaders);
		csHeaders = "Accept-Encoding: gzip, deflate\r\n"
					"Host: copiagenda.movistar.es\r\n"
					"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
					"Connection: Keep-Alive\r\n";
		csHeaders += "Cookie: "+csCookie+"\r\n";

		nCode=GetHTTPS(csURL,csHeaders,csRetHeaders,csRetData);

		// We are asked to re-authenticate with user data + cookie and we receive a session token
		int nPosPwd=csRetData.Find("password\" value=");
		if(nPosPwd!=-1)
		{
			CString csPwdAux=csRetData.Mid(nPosPwd+16);
			int nPosPwd2=csPwdAux.Find(">");
			if(nPosPwd2!=-1)
			{
				CString csPwd2=csPwdAux.Left(nPosPwd2);

				csData.Format("password=%s&u=%s&d=movistar.es",csPwd2,csLogin);
				csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
							"Accept-Encoding: gzip, deflate\r\n"
							"Host: copiagenda.movistar.es\r\n"
							"Referer: https://copiagenda.movistar.es/cp/ps/Main/login/Verificacion?d=movistar.es\r\n"
							"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
							"Connection: Keep-Alive\r\n";
				csHeaders += "Cookie: "+csCookie+"\r\n";

				UINT nCode=PostHTTPS("https://copiagenda.movistar.es/cp/ps/Main/login/Authenticate",(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

				int nPosTok=csRetData.Find("&t=");
				if(nPosTok!=-1)
				{
					CString csTokAux=csRetData.Mid(nPosTok+3);
					int nPosTok2=csTokAux.Find("\"");
					if(nPosTok2!=-1)
					{
						// We ask for the data in text format delimited with TABs
						CString csToken=csTokAux.Left(nPosTok2);
						CString csURL = "https://copiagenda.movistar.es/cp/ps/PSPab/preferences/ExportContacts?d=movistar.es&c=yes&u="+csLogin+"&t="+csToken;
						csData = "fileFormat=TEXT&charset=8859_1&delimiter=TAB";
						csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
									"Accept-Encoding: gzip, deflate\r\n"
									"Host: copiagenda.movistar.es\r\n"
									"Referer: https://copiagenda.movistar.es/cp/ps/Main/login/Verificacion?d=movistar.es\r\n"
									"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
									"Connection: Keep-Alive\r\n";
						csHeaders += "Cookie: "+csCookie+"\r\n";

						UINT nCode=PostHTTPS(csURL,(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);
						pmsoList = ParseResponse(csRetData);
					}
				}
			}
		}
	}

	return pmsoList;
}

// Function to search for a contact by its name
// Input:	csName=string with contact name or part of it
//			pmsoAddressBook=contact list to search in
// Returns: full contact with all its fields
CStringArray* CCopiagenda::SearchByName(CString csName, CMapStringToOb* pmsoAddressBook)
{
	if(!pmsoAddressBook)
		return NULL;

	CStringArray* psaContact;
	CString csKey;
	for(POSITION pos=pmsoAddressBook->GetStartPosition();pos;)
	{
		pmsoAddressBook->GetNextAssoc(pos,csKey,(CObject*&)psaContact);
		if(psaContact)
		{
			int nPos=csKey.Find(csName);
			if(nPos!=-1)
				return psaContact;
		}
	}

	return NULL;
}
