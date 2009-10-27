// MMSSender.cpp : implementation file
//

#include "stdafx.h"
#include "MMSSender.h"

#include <afxinet.h>
#include <wininet.h>

CMMSSender::CMMSSender()
{
	m_csServer = "multimedia.movistar.es";

	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,SessionFlags);

	CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322; .NET CLR 2.0.50727)";
	char strUserAgent[255];
	strcpy(strUserAgent,csUserAgent);
	m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
}

CMMSSender::~CMMSSender()
{
	m_pSession->Close();
	delete m_pSession;
	m_pSession=NULL;
}

CString CMMSSender::ReadData(CHttpFile* pFile)
{
	CString csRespuesta;
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

	csRespuesta=strBuf;
	free(strBuf);

	return csRespuesta;
}

CString CMMSSender::GetCookies(CString csHeaders)
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
			csCookieAux.Replace(";Path=/","");
			csCookieAux.Replace("; Path=/","");
			csCookieAux.Replace(";Domain=.movistar.es","");
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

CString CMMSSender::GetLocation(CString csHeaders)
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

UINT CMMSSender::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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
	pConnection=m_pSession->GetHttpConnection(csServer,0, nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_NO_AUTO_REDIRECT);

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

UINT CMMSSender::GetHTTP(CString csURL, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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
	pConnection=m_pSession->GetHttpConnection(csServer,0,nPort, NULL, NULL);

	if(pConnection)
	{
		BOOL bResult=FALSE;
		CHttpFile* pFile = NULL;
		pFile=pConnection->OpenRequest(CHttpConnection::HTTP_VERB_GET,csPath,NULL,1,NULL,"HTTP/1.1",INTERNET_FLAG_NO_AUTO_REDIRECT);

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

CString CMMSSender::GetParameter(CString csURL,CString csParam)
{
	CString csPar;
	int nPosPar=csURL.Find(csParam);
	if(nPosPar!=-1)
	{
		int nPosEnd=csURL.Find("&",nPosPar);
		if(nPosEnd!=-1)
			csPar=csURL.Mid(nPosPar+csParam.GetLength(),nPosEnd-nPosPar-csParam.GetLength());
		else
			csPar=csURL.Mid(nPosPar+csParam.GetLength());
	}

	return csPar;
}

// Performs login to MMS service
// Input:	csLogin=String with user's telephone number
//			csPwd=String with user's password
// Returns:	UserId
CString CMMSSender::Login(CString csLogin, CString csPwd)
{
	if(m_pSession)
	{
		m_pSession->Close();
		delete m_pSession;
		m_pSession = NULL;

		DWORD SessionFlags = INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
		m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,SessionFlags);

		CString csUserAgent="Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; .NET CLR 1.1.4322; .NET CLR 2.0.50727)";
		char strUserAgent[255];
		strcpy(strUserAgent,csUserAgent);
		m_pSession->SetOption(INTERNET_OPTION_USER_AGENT,(void*)strUserAgent,strlen(strUserAgent),0);
	}

	CString csData;
	CString csURL;

	CString csCookie;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

    // We try to access http://multimedia.movistar.es/
    csHeaders = "Connection: Keep-Alive\r\n";

	csURL.Format("http://%s/",m_csServer);
	UINT nCode=GetHTTP(csURL,csHeaders,csRetHeaders,csRetData);
	if(nCode == 302)
	{
        //We are redirected
		csCookie = GetCookies(csRetHeaders);
		CString csCookieSession=csCookie;

		// Login data posting
		csData.Format("TM_ACTION=LOGIN&variant=mensajeria&locale=sp-SP&client=html-msie-7-winxp&directMessageView=&uid=&uidl=&folder=&remoteAccountUID=&login=1&TM_LOGIN=%s&TM_PASSWORD=%s",csLogin,csPwd);
		csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
					"Accept-Encoding: identity\r\n"
					"Connection: Keep-Alive\r\n";

		CString csCookieSessionValue;
		int nPosValue=csCookieSession.Find("=");
		if(nPosValue!=-1)
		{
			csCookieSessionValue=csCookieSession.Mid(nPosValue+1);
			m_csUser = csCookieSessionValue;
		}
		csURL.Format("http://%s/do/dologin;jsessionid=%s",m_csServer,csCookieSessionValue);
		nCode=PostHTTP(csURL,(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

		CString csMoreCookies=GetCookies(csRetHeaders);
		m_csCookie=csCookieSession + "; " + csMoreCookies;

		csHeaders = "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
					"Accept-Encoding: identity\r\n"
					"Connection: Keep-Alive\r\n";
		csHeaders += "Cookie: "+m_csCookie+"\r\n";
		csURL.Format("http://%s/do/multimedia/create?l=sp-SP&v=mensajeria",m_csServer);
		nCode=GetHTTP(csURL,csHeaders,csRetHeaders,csRetData);
	}

	m_csLogin = csLogin;

	return m_csUser;
}

// Inserts an image object in MMS message (this function is provided for backward compatibility)
// Input:	csObjName=String with object name
//			csObjPath=String with file path
void CMMSSender::InsertImage(CString csObjName, CString csObjPath)
{
	InsertImage(csObjPath);
}

// Inserts an image object in MMS message
// Input:	csObjPath=String with file path
void CMMSSender::InsertImage(CString csObjPath)
{
	CMapStringToString mssContentType;
	mssContentType.SetAt("gif","image/gif");
	mssContentType.SetAt("jpg","image/pjpeg");
	mssContentType.SetAt("jpeg","image/pjpeg");
	mssContentType.SetAt("png","image/x-png");
	mssContentType.SetAt("bmp","image/bmp");

	CString csContentType;
	int nPosPunto=csObjPath.ReverseFind('.');
	if(nPosPunto!=-1)
	{
		CString csExtension=csObjPath.Mid(nPosPunto+1);
		mssContentType.Lookup(csExtension,csContentType);

		if(csContentType.IsEmpty())
			return;
	}

	CString csData;
	CString csURL;

	CString csCookie;

	CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	BYTE* pbtFile=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtFile,0,OBJ_BUFFER_SIZE);

	BYTE* pbtContents=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtContents,0,OBJ_BUFFER_SIZE);
	int nPosicionDatos=0;

	CString csSeparator="---------------------------7d8219060180";

	// Object data generation
	CString csFilenamePart;
	csFilenamePart.Format("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",csSeparator,csObjPath,csContentType);
	memcpy(&pbtContents[nPosicionDatos],csFilenamePart.GetBuffer(0),csFilenamePart.GetLength());
	nPosicionDatos+=csFilenamePart.GetLength();

	CFile fl;
	BOOL bRes=fl.Open(csObjPath,CFile::modeRead);
	if(bRes)
	{
		int nLeidos=fl.Read(pbtFile,OBJ_BUFFER_SIZE);
		fl.Close();

		memcpy(&pbtContents[nPosicionDatos],pbtFile,nLeidos);
		nPosicionDatos+=nLeidos;
	}
	//memcpy(&pbtContents[nPosicionDatos],"\xff\x0d8\xff\xe0\x00\x10\x4a\x46\x49\x46\x00\x01\x01\x01\x00\x60",16);
	//nPosicionDatos+=16;

	CString csFinal;
	csFinal.Format("\r\n--%s--\r\n",csSeparator);
	memcpy(&pbtContents[nPosicionDatos],csFinal.GetBuffer(0),csFinal.GetLength());
	nPosicionDatos+=csFinal.GetLength();

    CString csContentType2;
	csContentType2.Format("multipart/form-data; boundary=%s",csSeparator);
	csHeaders = "Accept-Encoding: gzip, deflate\r\n"
				"Accept-Language: es\r\n"
				"Accept: */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Content-type: "+csContentType2+"\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	csURL.Format("http://%s/do/multimedia/uploadEnd",m_csServer);
	UINT nCode=PostHTTP(csURL,(BYTE*)pbtContents,nPosicionDatos,csHeaders,csRetHeaders,csRetData);

	free(pbtFile);
	free(pbtContents);
}

// Inserts an audio object in MMS message (this function is provided for backward compatibility)
// Input:	csObjName=String with object name
//			csObjPath=String with file path
void CMMSSender::InsertAudio(CString csObjName, CString csObjPath)
{
	InsertAudio(csObjPath);
}

// Inserts an audio object in MMS message
// Input:	csObjPath=String with file path
void CMMSSender::InsertAudio(CString csObjPath)
{
	CMapStringToString mssContentType;
	mssContentType.SetAt("mid","audio/mid");
	mssContentType.SetAt("wav","audio/wav");
	mssContentType.SetAt("mp3","audio/mpeg");

	CString csContentType;
	int nPosPunto=csObjPath.ReverseFind('.');
	if(nPosPunto!=-1)
	{
		CString csExtension=csObjPath.Mid(nPosPunto+1);
		mssContentType.Lookup(csExtension,csContentType);

		if(csContentType.IsEmpty())
			return;
	}

	CString csData;
	CString csURL;

	CString csCookie;

	CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	BYTE* pbtFile=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtFile,0,OBJ_BUFFER_SIZE);

	BYTE* pbtContents=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtContents,0,OBJ_BUFFER_SIZE);
	int nPosicionDatos=0;

	CString csSeparator="---------------------------7d77df567a4b9";

	// Object data generation
	CString csFilenamePart;
	csFilenamePart.Format("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",csSeparator,csObjPath,csContentType);
	memcpy(&pbtContents[nPosicionDatos],csFilenamePart.GetBuffer(0),csFilenamePart.GetLength());
	nPosicionDatos+=csFilenamePart.GetLength();

	CFile fl;
	BOOL bRes=fl.Open(csObjPath,CFile::modeRead);
	if(bRes)
	{
		int nLeidos=fl.Read(pbtFile,OBJ_BUFFER_SIZE);
		fl.Close();

		memcpy(&pbtContents[nPosicionDatos],pbtFile,nLeidos);
		nPosicionDatos+=nLeidos;
	}

	CString csFinal;
	csFinal.Format("\r\n--%s--\r\n",csSeparator);
	memcpy(&pbtContents[nPosicionDatos],csFinal.GetBuffer(0),csFinal.GetLength());
	nPosicionDatos+=csFinal.GetLength();

    CString csContentType2;
	csContentType2.Format("multipart/form-data; boundary=%s",csSeparator);
	csHeaders = "Accept-Encoding: gzip, deflate\r\n"
				"Accept-Language: es\r\n"
				"Accept: */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Content-type: "+csContentType2+"\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	csURL.Format("http://%s/do/multimedia/uploadEnd",m_csServer);
	UINT nCode=PostHTTP(csURL,pbtContents,nPosicionDatos,csHeaders,csRetHeaders,csRetData);

	free(pbtFile);
	free(pbtContents);
}

// Inserts a video object in MMS message (this function is provided for backward compatibility)
// Input:	csObjName=String with object name
//			csObjPath=String with file path
void CMMSSender::InsertVideo(CString csObjName, CString csObjPath)
{
	InsertVideo(csObjPath);
}

// Inserts a video object in MMS message
// Input:	csObjPath=String with file path
void CMMSSender::InsertVideo(CString csObjPath)
{
	CMapStringToString mssContentType;
	mssContentType.SetAt("avi","video/avi");
	mssContentType.SetAt("asf","video/x-ms-asf");
	mssContentType.SetAt("mpg","video/mpeg");
	mssContentType.SetAt("mpeg","video/mpeg");
	mssContentType.SetAt("wmv","video/x-ms-wmv");

	CString csContentType;
	int nPosPunto=csObjPath.ReverseFind('.');
	if(nPosPunto!=-1)
	{
		CString csExtension=csObjPath.Mid(nPosPunto+1);
		mssContentType.Lookup(csExtension,csContentType);

		if(csContentType.IsEmpty())
			return;
	}

	CString csData;
	CString csURL;

	CString csCookie;

	CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	BYTE* pbtFile=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtFile,0,OBJ_BUFFER_SIZE);

	BYTE* pbtContents=(BYTE*)malloc(OBJ_BUFFER_SIZE);
	memset(pbtContents,0,OBJ_BUFFER_SIZE);
	int nPosicionDatos=0;

	CString csSeparator="---------------------------7d77df567a4b9";

	// Object data generation
	CString csFilenamePart;
	csFilenamePart.Format("--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",csSeparator,csObjPath,csContentType);
	memcpy(&pbtContents[nPosicionDatos],csFilenamePart.GetBuffer(0),csFilenamePart.GetLength());
	nPosicionDatos+=csFilenamePart.GetLength();

	CFile fl;
	BOOL bRes=fl.Open(csObjPath,CFile::modeRead);
	if(bRes)
	{
		int nLeidos=fl.Read(pbtFile,OBJ_BUFFER_SIZE);
		fl.Close();

		memcpy(&pbtContents[nPosicionDatos],pbtFile,nLeidos);
		nPosicionDatos+=nLeidos;
	}

	CString csFinal;
	csFinal.Format("\r\n--%s--\r\n",csSeparator);
	memcpy(&pbtContents[nPosicionDatos],csFinal.GetBuffer(0),csFinal.GetLength());
	nPosicionDatos+=csFinal.GetLength();

    CString csContentType2;
	csContentType2.Format("multipart/form-data; boundary=%s",csSeparator);
	csHeaders = "Accept-Encoding: gzip, deflate\r\n"
				"Accept-Language: es\r\n"
				"Accept: */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Content-type: "+csContentType2+"\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	csURL.Format("http://%s/do/multimedia/uploadEnd",m_csServer);
	UINT nCode=PostHTTP(csURL,pbtContents,nPosicionDatos,csHeaders,csRetHeaders,csRetData);

	free(pbtFile);
	free(pbtContents);
}

// Performs MMS sending
// Input:	csSubject=String with message subject
//			csDest=String with destination telephone number
//			csMsg=String with message text
bool CMMSSender::SendMessage(CString csSubject, CString csDest, CString csMsg)
{
	CString csData;
	CString csURL;

	CString csCookie;

	CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	CString csSeparator="-----------------------------7d834539300664";

	// Generating object data
    CString csBasefolderPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"basefolder\"\r\n\r\n\r\n";
    CString csFolderPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"folder\"\r\n\r\n\r\n";
    CString csIdPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"id\"\r\n\r\n\r\n";
    CString csPublicPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"public\"\r\n\r\n\r\n";
    CString csNamePart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"name\"\r\n\r\n\r\n";
    CString csUrlPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"url\"\r\n\r\n\r\n";
    CString csOwnerPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"owner\"\r\n\r\n\r\n";
    CString csDeferredDatePart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"deferredDate\"\r\n\r\n\r\n";
    CString csRequestReturnReceiptPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"requestReturnReceipt\"\r\n\r\n\r\n";
    CString csToPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"to\"\r\n\r\n"+csDest+"\r\n";
    CString csSubjectPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"subject\"\r\n\r\n"+csSubject+"\r\n";
    CString csTextPart = "--" + csSeparator + "\r\nContent-Disposition: form-data; name=\"text\"\r\n\r\n"+csMsg+"\r\n";

    CString csFinal = "--" + csSeparator + "--\r\n";

	csData=csBasefolderPart+csFolderPart+csIdPart+csPublicPart+csNamePart+csUrlPart+csOwnerPart+csDeferredDatePart+csRequestReturnReceiptPart+csToPart+csSubjectPart+csTextPart+csFinal;

	CString csContentType2;
	csContentType2.Format("multipart/form-data; boundary=%s",csSeparator);
	csHeaders = "Accept-Encoding: gzip, deflate\r\n"
				"Accept-Language: es\r\n"
				"Accept: */*\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Content-type: "+csContentType2+"\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";
	csURL.Format("http://%s/do/multimedia/send?l=sp-SP&v=mensajeria",m_csServer);
	UINT nCode=PostHTTP(csURL,(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);

	int nPosSent=csRetData.Find("Tu mensaje ha sido enviado");
	if(nPosSent!=-1)
		return true;

	return false;
}

// Performs MMS service logout
void CMMSSender::Logout()
{
	CString csData;
	CString csURL;

	CString csCookie;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	// Logout
	csData = "TM_ACTION=LOGOUT";
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Accept-Encoding: identity\r\n"
				"Connection: Keep-Alive\r\n";
	csHeaders += "Cookie: "+m_csCookie+"\r\n";

	csURL.Format("http://%s/do/logout?l=sp-SP&v=mensajeria",m_csServer);
	UINT nCode=PostHTTP(csURL,(BYTE*)csData.GetBuffer(0),csData.GetLength(),csHeaders,csRetHeaders,csRetData);
}
