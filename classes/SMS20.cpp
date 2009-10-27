// SMS20.cpp : implementation file
//

#include "stdafx.h"
#include "SMS20.h"

#include <afxinet.h>
#include <wininet.h>

// Clase CContactoSMS20

CSMS20Contact::CSMS20Contact()
{
	m_bPresent=false;
}

CSMS20Contact::CSMS20Contact(CString csUserID, CString csAlias, bool bPresent)
{
	m_csUserID=csUserID;
	m_csAlias=csAlias;
	m_bPresent=bPresent;
}

CSMS20Contact::~CSMS20Contact()
{
}

// Clase CSMS20Helper

CSMS20Helper::CSMS20Helper()
{
}

CString CSMS20Helper::SearchValue(CString csData, CString csTag, UINT& nEndPos)
{
	CString csValue;

	UINT nTagPos=csData.Find("<"+csTag+">");
	if(nTagPos!=-1)
	{
		UINT nTagEndPos=csData.Find("</"+csTag+">");
		if(nTagEndPos!=-1)
		{
			csValue=csData.Mid(nTagPos+csTag.GetLength()+2,nTagEndPos-nTagPos-csTag.GetLength()-2);
			nEndPos=nTagEndPos+csTag.GetLength()+1;
		}
	}

	return csValue;
}

// Converts text from ASCII to UTF8
CString CSMS20Helper::ASCII2UTF8(CString csText)
{
	char strText[256];
	strcpy(strText,csText);
	char strUTF8Text[256];
	WCHAR wszText[256];
	// Convert text from UTF-8 to Unicode
	MultiByteToWideChar(CP_ACP,0,strText,strlen(strText)+1,wszText,sizeof(wszText)/sizeof(wszText[0]));
	// Convert text from Unicode to UTF-8
	WideCharToMultiByte(CP_UTF8,0,wszText,sizeof(wszText)/sizeof(wszText[0])+1,strUTF8Text,256,NULL,NULL);
	CString csUTF8Text=strUTF8Text;

	return csUTF8Text;
}

// Converts text from UTF8 to ASCII
CString CSMS20Helper::UTF82ASCII(CString csUTF8Text)
{
	char strUTF8Text[256];
	strcpy(strUTF8Text,csUTF8Text);
	char strText[256];
	WCHAR wszText[256];
	// Convert text from UTF-8 to Unicode
	MultiByteToWideChar(CP_UTF8,0,strUTF8Text,strlen(strUTF8Text)+1,wszText,sizeof(wszText)/sizeof(wszText[0]));
	// Convert text from Unicode to UTF-8
	WideCharToMultiByte(CP_ACP,0,wszText,sizeof(wszText)/sizeof(wszText[0])+1,strText,256,NULL,NULL);
	CString csText=strText;

	return csText;
}

// Obtains contacts presence status from received XML
CMapStringToString* CSMS20Helper::GetContactPresence(CString csData)
{
	CMapStringToString* pPresList = new CMapStringToString();

	CString csPresenceValue;

	UINT nEndPos=0;

	CString csPresence;
	do
	{
		csPresence = SearchValue(csData.Mid(nEndPos), "Presence", nEndPos);
		if(!csPresence.IsEmpty())
		{
			UINT nEndPos2=0;
			CString csUserId = SearchValue(csPresence,"UserID", nEndPos2);
			if(!csUserId.IsEmpty())
			{
				CString csAvailability = SearchValue(csPresence,"UserAvailability", nEndPos2);
				if(!csAvailability.IsEmpty())
					csPresenceValue = SearchValue(csAvailability,"PresenceValue", nEndPos2);

				pPresList->SetAt(csUserId,csPresenceValue);
			}
		}
	}
	while(!csPresence.IsEmpty());

	return pPresList;
}

// Retrieves UserID for the contact that wants to know our presence status
CString CSMS20Helper::SearchAuthorizePresence(CString csData)
{
	UINT nEndPos=0;
	CString csUserID;
	CString csPresence = SearchValue(csData, "PresenceAuth-Request", nEndPos);
	if(!csPresence.IsEmpty())
		csUserID = SearchValue(csPresence, "UserID", nEndPos);

	return csUserID;
}

// Retrieves TransactionID for an incoming request
CString CSMS20Helper::SearchTransactionId(CString csData)
{
	UINT nEndPos=0;
	CString csTransId = SearchValue(csData, "TransactionID", nEndPos);

	return csTransId;
}

// Retrieves sender and text of a received message
CString CSMS20Helper::SearchMessage(CString csData)
{
	UINT nEndPos=0;
	CString csNewMessage = SearchValue(csData, "NewMessage", nEndPos);
	if(!csNewMessage.IsEmpty())
	{
		CString csMessage;
		CString csUserID;

		CString csSender = SearchValue(csNewMessage, "Sender", nEndPos);
		if(!csNewMessage.IsEmpty())
			csUserID = SearchValue(csSender, "UserID", nEndPos);

		CString csContentData = SearchValue(csNewMessage, "ContentData", nEndPos);
		int nGTPos=csContentData.Find(">");
		if(nGTPos!=-1)
		{
			int nLTPos=csContentData.Find("&lt;/");
			if(nLTPos!=-1)
				csMessage=csContentData.Mid(nGTPos+1,nLTPos-nGTPos-1);
			else
				csMessage=csContentData.Mid(nGTPos+1);
		}
		else
			csMessage=csContentData;

		csMessage.TrimLeft();
		csMessage.TrimRight();
		return csUserID+"|"+csMessage;
	}

	return "";
}

// Clase CSMS20

CSMS20::CSMS20()
{
	m_pSession = NULL;

	DWORD SessionFlags = INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	m_pSession = new CInternetSession(NULL,1,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,SessionFlags);

	m_csHeaders =	"Content-type: application/vnd.wv.csp.xml\r\n"
					"Expect: 100-continue\r\n";
	m_csURL = "http://sms20.movistar.es/";

	HRESULT hr;
	hr = CoInitialize(NULL);
}

CSMS20::~CSMS20()
{
	m_pSession->Close();
	delete m_pSession;
	m_pSession=NULL;
}

CString CSMS20::ReadData(CHttpFile* pFile)
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

UINT CSMS20::PostHTTP(CString csURL, BYTE* strPostData, long lDataSize, CString csHeaders, CString& csRetHeaders, CString& csRetData)
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

CString CSMS20::SearchValue(CString csData, CString csTag, UINT& nEndPos)
{
	CString csValue;

	UINT nTagPos=csData.Find("<"+csTag+">");
	if(nTagPos!=-1)
	{
		UINT nTagEndPos=csData.Find("</"+csTag+">");
		if(nTagEndPos!=-1)
		{
			csValue=csData.Mid(nTagPos+csTag.GetLength()+2,nTagEndPos-nTagPos-csTag.GetLength()-2);
			nEndPos=nTagEndPos+csTag.GetLength()+1;
		}
	}

	return csValue;
}

CString CSMS20::GetAlias(CString csData, CString csLogin)
{
	CString csPresenceValue;

	UINT nEndPos=0;
	CString csPresence = SearchValue(csData, "Presence", nEndPos);
	if(!csPresence.IsEmpty())
	{
		CString csUserId = SearchValue(csPresence,"UserID", nEndPos);
		if(csUserId=="wv:" + csLogin + "@movistar.es")
		{
			CString csAlias = SearchValue(csPresence,"Alias", nEndPos);
			if(!csAlias.IsEmpty())
				csPresenceValue = SearchValue(csAlias,"PresenceValue", nEndPos);
		}
	}

	return csPresenceValue;
}

CMapStringToOb* CSMS20::GetContactList(CString csData)
{
	CMapStringToOb* pContactList = new CMapStringToOb();

	UINT nEndPos=0;
	CString csNickList = SearchValue(csData, "NickList", nEndPos);
	if(!csNickList.IsEmpty())
	{
		CString csNickName;
		nEndPos=0;
		do
		{
			csNickName = SearchValue(csNickList.Mid(nEndPos), "NickName", nEndPos);
			if(!csNickName.IsEmpty())
			{
				UINT nEndPos2;
				CString csName = SearchValue(csNickName,"Name", nEndPos2);
				CString csUserID = SearchValue(csNickName,"UserID", nEndPos2);

				CSMS20Contact* pContact = new CSMS20Contact(csUserID, csName);
				pContactList->SetAt(csUserID,pContact);
			}
		}
		while(!csNickName.IsEmpty());
	}

	return pContactList;
}

CMapStringToOb* CSMS20::GetPresenceList(CString csData)
{
	CMapStringToOb* pContactList = new CMapStringToOb();

	UINT nEndPos=0;
	CString csNickList = SearchValue(csData, "NickList", nEndPos);
	if(!csNickList.IsEmpty())
	{
		CString csNickName;
		nEndPos=0;
		do
		{
			csNickName = SearchValue(csNickList.Mid(nEndPos), "NickName", nEndPos);
			if(!csNickName.IsEmpty())
			{
				UINT nEndPos2;
				CString csName = SearchValue(csNickName,"Name", nEndPos2);
				CString csUserID = SearchValue(csNickName,"UserID", nEndPos2);

				CSMS20Contact* pContact = new CSMS20Contact(csUserID, csName);
				pContactList->SetAt(csUserID,pContact);
			}
		}
		while(!csNickName.IsEmpty());
	}

	return pContactList;
}


// Performs login to movistar web site
// Input:	csLogin=string with user's telephone number
//			csPassw=string with user's password
// Returns: Session Id (needed for all later requests)
CString CSMS20::Login(CString csLog, CString csPassw)
{
	CString csPostData;

    CString csHeaders;
	CString csRetHeaders;
	CString csRetData;

	csPostData.Format("TM_ACTION=AUTHENTICATE&TM_LOGIN=%s&TM_PASSWORD=%s&SessionCookie=ColibriaIMPS_367918656&ClientID=WV:InstantMessenger-1.0.2309.16485@COLIBRIA.PC-CLIENT",csLog,csPassw);
    csHeaders = "Content-type: application/x-www-form-urlencoded\r\n"
				"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/x-shockwave-flash, application/vnd.ms-excel, application/vnd.ms-powerpoint, application/msword, */*\r\n"
				"Connection: Keep-Alive\r\n";
	UINT nCode=PostHTTP("http://impw.movistar.es/tmelogin/tmelogin.jsp",(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),csHeaders,csRetHeaders,csRetData);

	m_nTransId=2;

	UINT nEndPos=0;
	CString csSession=SearchValue(csRetData,"SessionID", nEndPos);
	m_csSessionID = csSession;
    return csSession;
}

// Connects to SMS2.0 service
// Input:	csLog=string with user's tleephone number
//			csNickname=string with the nickname that we want to use (needed only first time)
// Returns: Contact list of the user
CMapStringToOb* CSMS20::Connect(CString csLog, CString csNickname)
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Access to sms20.movistar.es
	// Send <ClientCapability-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ClientCapability-Request><ClientID><URL>WV:InstantMessenger-1.0.2309.16485@COLIBRIA.PC-CLIENT</URL></ClientID><CapabilityList><ClientType>COMPUTER</ClientType><InitialDeliveryMethod>P</InitialDeliveryMethod><AcceptedContentType>text/plain</AcceptedContentType><AcceptedContentType>text/html</AcceptedContentType><AcceptedContentType>image/png</AcceptedContentType><AcceptedContentType>image/jpeg</AcceptedContentType><AcceptedContentType>image/gif</AcceptedContentType><AcceptedContentType>audio/x-wav</AcceptedContentType><AcceptedContentType>image/jpg</AcceptedContentType><AcceptedTransferEncoding>BASE64</AcceptedTransferEncoding><AcceptedContentLength>256000</AcceptedContentLength><MultiTrans>1</MultiTrans><ParserSize>300000</ParserSize><SupportedCIRMethod>STCP</SupportedCIRMethod><ColibriaExtensions>T</ColibriaExtensions></CapabilityList></ClientCapability-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <Service-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><Service-Request><ClientID><URL>WV:InstantMessenger-1.0.2309.16485@COLIBRIA.PC-CLIENT</URL></ClientID><Functions><WVCSPFeat><FundamentalFeat /><PresenceFeat /><IMFeat /><GroupFeat /></WVCSPFeat></Functions><AllFunctionsRequest>T</AllFunctionsRequest></Service-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <UpdatePresence-Request> to tell that we are using a IM client
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><UpdatePresence-Request><PresenceSubList xmlns=\"http://www.openmobilealliance.org/DTD/WV-PA1.2\"><OnlineStatus><Qualifier>T</Qualifier></OnlineStatus><ClientInfo><Qualifier>T</Qualifier><ClientType>COMPUTER</ClientType><ClientTypeDetail xmlns=\"http://imps.colibria.com/PA-ext-1.2\">PC</ClientTypeDetail><ClientProducer>Colibria As</ClientProducer><Model>TELEFONICA Messenger</Model><ClientVersion>1.0.2309.16485</ClientVersion></ClientInfo><CommCap><Qualifier>T</Qualifier><CommC><Cap>IM</Cap><Status>OPEN</Status></CommC></CommCap><UserAvailability><Qualifier>T</Qualifier><PresenceValue>AVAILABLE</PresenceValue></UserAvailability></PresenceSubList></UpdatePresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <GetList-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><GetList-Request /></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <GetPresence-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><GetPresence-Request><User><UserID>wv:%s@movistar.es</UserID></User></GetPresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	CString csMyAlias=GetAlias(csRetData,csLog);
	m_csMyAlias = csMyAlias;

	// Send <ListManage-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ListManage-Request><ContactList>wv:%s/~pep1.0_privatelist@movistar.es</ContactList><ReceiveList>T</ReceiveList></ListManage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	CMapStringToOb* pmsoAddressBook=GetContactList(csRetData);

	// Send <CreateList-Request>
	CString csListadenicks;
	if(pmsoAddressBook->GetCount())
	{
		csListadenicks="<NickList>";
		CString csUserID;
		for(POSITION pos=pmsoAddressBook->GetStartPosition();pos;)
		{
			CSMS20Contact* pContact;
			pmsoAddressBook->GetNextAssoc(pos,csUserID,(CObject*&)pContact);

			CString csNickName;
			csNickname.Format("<NickName><Name>%s</Name><UserID>%s</UserID></NickName>",pContact->GetAlias(),pContact->GetUserID());
			csListadenicks += csNickName;
		}
		csListadenicks="</NickList>";
	}
	else
		csListadenicks="<NickList />";
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><CreateList-Request><ContactList>wv:%s/~PEP1.0_subscriptions@movistar.es</ContactList>%s</CreateList-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog,csListadenicks);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <SubscribePresence-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><SubscribePresence-Request><ContactList>wv:%s/~PEP1.0_subscriptions@movistar.es</ContactList><PresenceSubList xmlns=\"http://www.openmobilealliance.org/DTD/WV-PA1.2\"><OnlineStatus /><ClientInfo /><FreeTextLocation /><CommCap /><UserAvailability /><StatusText /><StatusMood /><Alias /><StatusContent /><ContactInfo /></PresenceSubList><AutoSubscribe>T</AutoSubscribe></SubscribePresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <UpdatePresence-Request> to send our nick
	// (Only first time or when we want ot change it)
	if(!csNickname.IsEmpty())
	{
		csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><UpdatePresence-Request><PresenceSubList xmlns=\"http://www.openmobilealliance.org/DTD/WV-PA1.2\"><Alias><Qualifier>T</Qualifier><PresenceValue>%s</PresenceValue></Alias></PresenceSubList></UpdatePresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csNickname);
		nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

		m_csMyAlias = csNickname;
	}

	return pmsoAddressBook;
}

// Performs polling to search for new message notifications, contacts online, etc...
// Input:	none
// Returns: Full text of the response to search for different types of notification
CString CSMS20::Polling()
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <Polling-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID /></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><Polling-Request /></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	return csRetData;
}

// Adds a contact to the user's contact list
// Input:	csLog=string with user's telephone number
//			csContact=string with new contact's telephone number
// Returns: nickname of the new contact
CString CSMS20::AddContact(CString csLog, CString csContact)
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <Search-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><Search-Request><SearchPairList><SearchElement>USER_MOBILE_NUMBER</SearchElement><SearchString>%s</SearchString></SearchPairList><SearchLimit>50</SearchLimit></Search-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <GetPresence-Request> to get contact's presence status
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><GetPresence-Request><User><UserID>wv:%s@movistar.es</UserID></User><PresenceSubList xmlns=\"http://www.openmobilealliance.org/DTD/WV-PA1.2\"><OnlineStatus /><ClientInfo /><GeoLocation /><FreeTextLocation /><CommCap /><UserAvailability /><StatusText /><StatusMood /><Alias /><StatusContent /><ContactInfo /></PresenceSubList></GetPresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	CString csNickname=GetAlias(csRetData,csContact);

	// Send <ListManage-Request> for Subscriptions
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ListManage-Request><ContactList>wv:%s/~PEP1.0_subscriptions@movistar.es</ContactList><AddNickList><NickName><Name>%s</Name><UserID>wv:%s@movistar.es</UserID></NickName></AddNickList><ReceiveList>T</ReceiveList></ListManage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog,csNickname,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <ListManage-Request> for PrivateList
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ListManage-Request><ContactList>wv:%s/~PEP1.0_privatelist@movistar.es</ContactList><AddNickList><NickName><Name>%s</Name><UserID>wv:%s@movistar.es</UserID></NickName></AddNickList><ReceiveList>T</ReceiveList></ListManage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog,csNickname,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	return csNickname;
}

// Authorizes a contact to be informed about our presence status
// Input:	csUser=user id for the authorized contact (wv:6xxxxxxxx@movistar.es)
//			csTransaction=transaction id received in authorization request
// Returns: none
void CSMS20::AuthorizeContact(CString csUser, CString csTransaction)
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <GetPresence-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><GetPresence-Request><User><UserID>%s</UserID></User><PresenceSubList xmlns=\"http://www.openmobilealliance.org/DTD/WV-PA1.2\"><OnlineStatus /><ClientInfo /><GeoLocation /><FreeTextLocation /><CommCap /><UserAvailability /><StatusText /><StatusMood /><Alias /><StatusContent /><ContactInfo /></PresenceSubList></GetPresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csUser);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <Status> ack of the request
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Response</TransactionMode><TransactionID>%s</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><Status><Result><Code>200</Code></Result></Status></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,csTransaction);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <PresenceAuth-User>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><PresenceAuth-User><UserID>%s</UserID><Acceptance>T</Acceptance></PresenceAuth-User></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csUser);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);
}

// Deletes a contact
// Input:	csLog=string with user's telephone number
//			csContact=user id for the contact to delete (wv:6xxxxxxxx@movistar.es)
// Returns: none
void CSMS20::DeleteContact(CString csLog, CString csContact)
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <ListManage-Request> to delete contact from Subscriptions
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ListManage-Request><ContactList>wv:%s/~PEP1.0_subscriptions@movistar.es</ContactList><RemoveNickList><UserID>%s</UserID></RemoveNickList><ReceiveList>T</ReceiveList></ListManage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <ListManage-Request> to delete contact from PrivateList
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><ListManage-Request><ContactList>wv:%s/~PEP1.0_privatelist@movistar.es</ContactList><RemoveNickList><UserID>%s</UserID></RemoveNickList><ReceiveList>T</ReceiveList></ListManage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csLog,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <UnsubscribePresence-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><UnsubscribePresence-Request><User><UserID>%s</UserID></User></UnsubscribePresence-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <DeleteAttributeList-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><DeleteAttributeList-Request><UserID>%s</UserID><DefaultList>F</DefaultList></DeleteAttributeList-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	// Send <CancelAuth-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><CancelAuth-Request><UserID>%s</UserID></CancelAuth-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csContact);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);
}

// Sends a message to the destination contact
// Input:	csLog=string with user's telephone number
//			csDestination=string with the destination user id (wv:6xxxxxxxx@movistar.es)
//			csMessage=text of the message
// Returns: none
void CSMS20::SendMessage(CString csLog, CString csDestination, CString csMessage)
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <SendMessage-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><SendMessage-Request><DeliveryReport>F</DeliveryReport><MessageInfo><ContentType>text/html</ContentType><ContentSize>148</ContentSize><Recipient><User><UserID>%s</UserID></User></Recipient><Sender><User><UserID>%s@movistar.es</UserID></User></Sender></MessageInfo><ContentData>&lt;span style=\"color:#000000;font-family:\"Microsoft Sans Serif\";font-style:normal;font-weight:normal;font-size:12px;\"&gt;%s&lt;/span&gt;</ContentData></SendMessage-Request></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++,csDestination,csLog,csMessage);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);
}

// Disconnects from SMS2.0 service
// Input:	none
// Returns: none
void CSMS20::Disconnect()
{
	CString csPostData;
	CString csRetHeaders;
	CString csRetData;

	UINT nCode;

	// Send <Logout-Request>
	csPostData.Format("<?xml version=\"1.0\" encoding=\"utf-8\"?><WV-CSP-Message xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns=\"http://www.openmobilealliance.org/DTD/WV-CSP1.2\"><Session><SessionDescriptor><SessionType>Inband</SessionType><SessionID>%s</SessionID></SessionDescriptor><Transaction><TransactionDescriptor><TransactionMode>Request</TransactionMode><TransactionID>%d</TransactionID></TransactionDescriptor><TransactionContent xmlns=\"http://www.openmobilealliance.org/DTD/WV-TRC1.2\"><Logout-Request /></TransactionContent></Transaction></Session></WV-CSP-Message>",m_csSessionID,m_nTransId++);
	nCode=PostHTTP(m_csURL,(BYTE*)csPostData.GetBuffer(0),csPostData.GetLength(),m_csHeaders,csRetHeaders,csRetData);

	m_csSessionID = "";
}
