// VideoCall.cpp: archivo de implementación
//

#include "stdafx.h"
#include "VideoCall.h"

#include "SIPSocket.h"
#include "Frame263.h"
#include "WriteAvi.h"
#include "MD5Checksum.h"
#include "decoder\tmndec.h"


CVideoCall::CVideoCall(CString csStartTime, CString csFrom, CString csLocalIP, CString csPath, CString csCallId, CString csBranch, CString csFromTag, int nToTag)
{
	m_csStartTime=csStartTime;
	m_csFrom=csFrom;

	m_csLocalIP=csLocalIP;
	m_csPath=csPath;

	m_csCallId=csCallId;
	m_csBranch=csBranch;
	m_csFromTag=csFromTag;
	m_nToTag=nToTag;

	m_pSocketAudio=NULL;
	m_pSocketAudioCtrl=NULL;
	m_pSocketVideo=NULL;
	m_pSocketVideoCtrl=NULL;

	m_bVideoReceived=true;
}

CVideoCall::~CVideoCall()
{
	if(m_pSocketAudio)
	{
		m_pSocketAudio->Close();
		delete m_pSocketAudio;
		m_pSocketAudio=NULL;
	}
	if(m_pSocketAudioCtrl)
	{
		m_pSocketAudioCtrl->Close();
		delete m_pSocketAudioCtrl;
		m_pSocketAudioCtrl=NULL;
	}

	if(m_pSocketVideo)
	{
		m_pSocketVideo->Close();
		delete m_pSocketVideo;
		m_pSocketVideo=NULL;
	}
	if(m_pSocketVideoCtrl)
	{
		m_pSocketVideoCtrl->Close();
		delete m_pSocketVideoCtrl;
		m_pSocketVideoCtrl=NULL;
	}
}

void CVideoCall::CreateSockets()
{
	bool bRes;
	CString csAddr;

	m_pSocketAudio=new CSIPSocket(OnEventAudio,(DWORD)this);
	bRes=m_pSocketAudio->Create(0,SOCK_DGRAM,FD_READ|FD_WRITE,m_csLocalIP);
	m_pSocketAudio->GetSockNameEx(csAddr,m_nLocalAudioPort);
	m_pSocketAudioCtrl=new CSIPSocket(OnEventAudioCtrl,(DWORD)this);
	bRes=m_pSocketAudioCtrl->Create(m_nLocalAudioPort+1,SOCK_DGRAM,FD_READ|FD_WRITE,m_csLocalIP);

	m_pSocketVideo=new CSIPSocket(OnEventVideo,(DWORD)this);
	bRes=m_pSocketVideo->Create(0,SOCK_DGRAM,FD_READ|FD_WRITE,m_csLocalIP);
	m_pSocketVideo->GetSockNameEx(csAddr,m_nLocalVideoPort);
	m_pSocketVideoCtrl=new CSIPSocket(OnEventVideoCtrl,(DWORD)this);
	bRes=m_pSocketVideoCtrl->Create(m_nLocalVideoPort+1,SOCK_DGRAM,FD_READ|FD_WRITE,m_csLocalIP);
}

void CVideoCall::SendMultimediaData(CString csAckData)
{
	CString csPortAudio;
	int nPosAudio=csAckData.Find("m=audio");
	if(nPosAudio!=-1)
	{
		int nPosAudioEnd=csAckData.Find("RTP",nPosAudio+7);
		if(nPosAudioEnd!=-1)
		{
			csPortAudio=csAckData.Mid(nPosAudio+7,nPosAudioEnd-nPosAudio-7);
			csPortAudio.TrimLeft();
			csPortAudio.TrimRight();
		}
	}
	CString csPortVideo;
	int nPosVideo=csAckData.Find("m=video");
	if(nPosVideo!=-1)
	{
		int nPosVideoEnd=csAckData.Find("RTP",nPosVideo+7);
		if(nPosVideoEnd!=-1)
		{
			csPortVideo=csAckData.Mid(nPosVideo+7,nPosVideoEnd-nPosVideo-7);
			csPortVideo.TrimLeft();
			csPortVideo.TrimRight();
		}
	}

	if( (csPortAudio.IsEmpty()) || (csPortVideo.IsEmpty()) )
		return;

	m_nPortAudio=atoi(csPortAudio);
	m_nPortVideo=atoi(csPortVideo);

	// Audio Control
	BYTE pDataAudio[44]={0x80, 0xc9, 0x00, 0x01, 0x52, 0xb1, 0x9e, 0xd0,
						0x81, 0xca, 0x00, 0x06, 0x52, 0xb1, 0x9e, 0xd0,
						0x01, 0x0e, 0x46, 0x72, 0x61, 0x6e, 0x5f, 0x6d,
						0x40, 0x46, 0x72, 0x61, 0x6e, 0x5f, 0x6d, 0x31,
						0x00, 0x00, 0x00, 0x00, 0x81, 0xcb, 0x00, 0x01,
						0x52, 0xb1, 0x9e, 0xd0};
	if(m_pSocketAudioCtrl)
		m_pSocketAudioCtrl->SendTo(pDataAudio,44,m_nPortAudio+1,"195.76.180.160");

	// VideoControl
	BYTE pDataVideo[44]={0x80, 0xc9, 0x00, 0x01, 0x73, 0xaa, 0xdf, 0xe2,
						0x81, 0xca, 0x00, 0x06, 0x52, 0xb1, 0x9e, 0xd0,
						0x01, 0x0e, 0x46, 0x72, 0x61, 0x6e, 0x5f, 0x6d,
						0x40, 0x46, 0x72, 0x61, 0x6e, 0x5f, 0x6d, 0x31,
						0x00, 0x00, 0x00, 0x00, 0x81, 0xcb, 0x00, 0x01,
						0x73, 0xaa, 0xdf, 0xe2};
	if(m_pSocketVideoCtrl)
		m_pSocketVideoCtrl->SendTo(pDataVideo,44,m_nPortVideo+1,"195.76.180.160");

	// Audio
	BYTE pDataAudioData[12]={0x80, 0x80, 0x04, 0xae, 0xbd, 0xb7, 0xc1, 0x40,
							0x52, 0xb1, 0x9e, 0xd0};
	if(m_pSocketAudio)
		m_pSocketAudio->SendTo(pDataAudioData,12,m_nPortAudio,"195.76.180.160");

	// Video
	BYTE pDataVideoData[21]={0x80, 0x22, 0x8e, 0x9c, 0xef, 0xdb, 0xaf, 0x08,
							0x73, 0xaa, 0xdf, 0xe2, 0x00, 0x40, 0x00, 0x00,
							0x00, 0x80, 0x02, 0x08, 0x08};
	if(m_pSocketVideo)
		m_pSocketVideo->SendTo(pDataVideoData,21,m_nPortVideo,"195.76.180.160");
}

void CVideoCall::Echo(char* pBufData,DWORD dwLenData)
{
	if(m_pSocketAudio)
		m_pSocketAudio->SendTo(pBufData,dwLenData,m_nPortAudio,"195.76.180.160");
}

void CVideoCall::SaveAudioData(char* pBufData,DWORD dwLenData)
{
	if(!m_bVideoReceived)
		return;

	CFile fl;
	BOOL bRes=fl.Open(m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".raw",CFile::modeWrite);
	if(!bRes)
		bRes=fl.Open(m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".raw",CFile::modeWrite|CFile::modeCreate);
	if(bRes)
	{
		fl.SeekToEnd();
		fl.Write(pBufData,dwLenData);
		fl.Close();
	}
}

void CVideoCall::OnEventAudio(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	if(nCode==FSOCK_RECEIVE)
	{
		CVideoCall* pThis=(CVideoCall*)dwCookie;
		//pThis->Echo(pBufData,dwLenData);

		BYTE btPayload=pBufData[1]&0x7f;

		if(dwLenData>12)
		{
			if(btPayload==0)
			{
				BYTE* pRes=NULL;
				pRes = (BYTE*)malloc((dwLenData-12) * 2);
				pThis->DecodeULaw((BYTE*)&pBufData[12],dwLenData-12,pRes);
				pThis->SaveAudioData((char*)pRes,(dwLenData-12)*2);
				free(pRes);
			}
			else if(btPayload==8)
			{
				BYTE* pRes=NULL;
				pRes = (BYTE*)malloc((dwLenData-12) * 2);
				pThis->DecodeALaw((BYTE*)&pBufData[12],dwLenData-12,pRes);
				pThis->SaveAudioData((char*)pRes,(dwLenData-12)*2);
				free(pRes);
			}
		}
	}
}

void CVideoCall::OnEventAudioCtrl(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	int i=0;
}

void CVideoCall::Mirror(char* pBufData,DWORD dwLenData)
{
	if(m_pSocketVideo)
		m_pSocketVideo->SendTo(pBufData,dwLenData,m_nPortVideo,"195.76.180.160");
}

void CVideoCall::SaveVideoData(char* pBufData,DWORD dwLenData)
{
	m_bVideoReceived=true;

	CFile fl;
	BOOL bRes=fl.Open(m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".263",CFile::modeWrite);
	if(!bRes)
		bRes=fl.Open(m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".263",CFile::modeWrite|CFile::modeCreate);
	if(bRes)
	{
		fl.SeekToEnd();
		fl.Write(pBufData,dwLenData);
		fl.Close();
	}
}

void CVideoCall::OnEventVideo(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	if(nCode==FSOCK_RECEIVE)
	{
		CVideoCall* pThis=(CVideoCall*)dwCookie;
		pThis->SaveVideoData(pBufData,dwLenData);

		//pThis->Mirror(pBufData,dwLenData);
	}
}

void CVideoCall::OnEventVideoCtrl(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode)
{
	int i=0;
}

void CVideoCall::Terminate()
{
	// Audio Control
	BYTE pDataAudio[44]={0x80, 0xc9, 0x00, 0x01, 0x52, 0xb1, 0x9e, 0xd0,
						0x81, 0xca, 0x00, 0x06, 0x52, 0xb1, 0x9e, 0xd0,
						0x01, 0x0e, 0x53, 0x49, 0x50, 0x54, 0x53, 0x54,
						0x40, 0x53, 0x49, 0x50, 0x54, 0x53, 0x54, 0x31,
						0x00, 0x00, 0x00, 0x00, 0x81, 0xcb, 0x00, 0x01,
						0x52, 0xb1, 0x9e, 0xd0};
	if(m_pSocketAudioCtrl)
		m_pSocketAudioCtrl->SendTo(pDataAudio,44,m_nPortAudio+1,"195.76.180.160");

	// VideoControl
	BYTE pDataVideo[44]={0x80, 0xc9, 0x00, 0x01, 0x73, 0xaa, 0xdf, 0xe2,
						0x81, 0xca, 0x00, 0x06, 0x52, 0xb1, 0x9e, 0xd0,
						0x01, 0x0e, 0x53, 0x49, 0x50, 0x54, 0x53, 0x54,
						0x40, 0x53, 0x49, 0x50, 0x54, 0x53, 0x54, 0x31,
						0x00, 0x00, 0x00, 0x00, 0x81, 0xcb, 0x00, 0x01,
						0x73, 0xaa, 0xdf, 0xe2};
	if(m_pSocketVideoCtrl)
		m_pSocketVideoCtrl->SendTo(pDataVideo,44,m_nPortVideo+1,"195.76.180.160");

	if(m_pSocketAudio)
	{
		m_pSocketAudio->Close();
		delete m_pSocketAudio;
		m_pSocketAudio=NULL;
	}

	if(m_pSocketAudioCtrl)
	{
		m_pSocketAudioCtrl->Close();
		delete m_pSocketAudioCtrl;
		m_pSocketAudioCtrl=NULL;
	}

	if(m_pSocketVideo)
	{
		m_pSocketVideo->Close();
		delete m_pSocketVideo;
		m_pSocketVideo=NULL;
	}

	if(m_pSocketVideoCtrl)
	{
		m_pSocketVideoCtrl->Close();
		delete m_pSocketVideoCtrl;
		m_pSocketVideoCtrl=NULL;
	}

	DWORD dwDuration=ExportAudio();
	ExportVideo(dwDuration);
}

void CVideoCall::DecodeALaw(BYTE* pData, DWORD dwLen, BYTE* pRes)
{
	short pcm_A2lin[256] = {
		 -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736, -7552, -7296, -8064,
		 -7808, -6528, -6272, -7040, -6784, -2752, -2624, -3008, -2880, -2240, -2112,
		 -2496, -2368, -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,-22016,
		-20992,-24064,-23040,-17920,-16896,-19968,-18944,-30208,-29184,-32256,-31232,
		-26112,-25088,-28160,-27136,-11008,-10496,-12032,-11520, -8960, -8448, -9984,
		 -9472,-15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,  -344,  -328,
		  -376,  -360,  -280,  -264,  -312,  -296,  -472,  -456,  -504,  -488,  -408,
		  -392,  -440,  -424,   -88,   -72,  -120,  -104,   -24,    -8,   -56,   -40,
		  -216,  -200,  -248,  -232,  -152,  -136,  -184,  -168, -1376, -1312, -1504,
		 -1440, -1120, -1056, -1248, -1184, -1888, -1824, -2016, -1952, -1632, -1568,
		 -1760, -1696,  -688,  -656,  -752,  -720,  -560,  -528,  -624,  -592,  -944,
		  -912, -1008,  -976,  -816,  -784,  -880,  -848,  5504,  5248,  6016,  5760,
		  4480,  4224,  4992,  4736,  7552,  7296,  8064,  7808,  6528,  6272,  7040,
		  6784,  2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,  3776,  3648,
		  4032,  3904,  3264,  3136,  3520,  3392, 22016, 20992, 24064, 23040, 17920,
		 16896, 19968, 18944, 30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
		 11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472, 15104, 14592, 16128,
		 15616, 13056, 12544, 14080, 13568,   344,   328,   376,   360,   280,   264,
		   312,   296,   472,   456,   504,   488,   408,   392,   440,   424,    88,
		    72,   120,   104,    24,     8,    56,    40,   216,   200,   248,   232,
		   152,   136,   184,   168,  1376,  1312,  1504,  1440,  1120,  1056,  1248,
		  1184,  1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,   688,   656,
		   752,   720,   560,   528,   624,   592,   944,   912,  1008,   976,   816,
		   784,   880,   848 };

	for (int i = 0; i < dwLen; i++)
	{
		//First byte is the less significant byte
		pRes[2 * i] = (BYTE)(pcm_A2lin[pData[i]] & 0xff);
		//Second byte is the more significant byte
		pRes[2 * i + 1] = (BYTE)(pcm_A2lin[pData[i]] >> 8);
	}
}

void CVideoCall::DecodeULaw(BYTE* pData, DWORD dwLen, BYTE* pRes)
{
	short pcm_u2lin[256] = {
		-32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,-23932,-22908,-21884,
		-20860,-19836,-18812,-17788,-16764,-15996,-15484,-14972,-14460,-13948,-13436,
		-12924,-12412,-11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316, -7932,
		 -7676, -7420, -7164, -6908, -6652, -6396, -6140, -5884, -5628, -5372, -5116,
		 -4860, -4604, -4348, -4092, -3900, -3772, -3644, -3516, -3388, -3260, -3132,
		 -3004, -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980, -1884, -1820,
		 -1756, -1692, -1628, -1564, -1500, -1436, -1372, -1308, -1244, -1180, -1116,
		 -1052,  -988,  -924,  -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
		  -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,  -372,  -356,  -340,
		  -324,  -308,  -292,  -276,  -260,  -244,  -228,  -212,  -196,  -180,  -164,
		  -148,  -132,  -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,   -56,
		   -48,   -40,   -32,   -24,   -16,    -8,     0, 32124, 31100, 30076, 29052,
		 28028, 27004, 25980, 24956, 23932, 22908, 21884, 20860, 19836, 18812, 17788,
		 16764, 15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412, 11900, 11388,
		 10876, 10364,  9852,  9340,  8828,  8316,  7932,  7676,  7420,  7164,  6908,
		  6652,  6396,  6140,  5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
		  3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,  2876,  2748,  2620,
		  2492,  2364,  2236,  2108,  1980,  1884,  1820,  1756,  1692,  1628,  1564,
		  1500,  1436,  1372,  1308,  1244,  1180,  1116,  1052,   988,   924,   876,
		   844,   812,   780,   748,   716,   684,   652,   620,   588,   556,   524,
		   492,   460,   428,   396,   372,   356,   340,   324,   308,   292,   276,
		   260,   244,   228,   212,   196,   180,   164,   148,   132,   120,   112,
		   104,    96,    88,    80,    72,    64,    56,    48,    40,    32,    24,
		    16,     8,     0};

	for (int i = 0; i < dwLen; i++)
	{
		//First byte is the less significant byte
		pRes[2 * i] = (BYTE)(pcm_u2lin[pData[i]] & 0xff);
		//Second byte is the more significant byte
		pRes[2 * i + 1] = (BYTE)(pcm_u2lin[pData[i]] >> 8);
	}
}

DWORD CVideoCall::ExportAudio()
{
	CString csFile=m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".raw";

	CFile fl;
	BOOL bRes=fl.Open(csFile,CFile::modeRead);
	if(bRes)
	{
		size_t flsize=fl.GetLength();
		BYTE* pBuffer=(BYTE*)malloc(flsize);
		fl.Read(pBuffer,flsize);
		fl.Close();

		CString csFileSave=m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".wav";

		CFile fl2;
		BOOL bRes2=fl2.Open(csFileSave,CFile::modeWrite|CFile::modeCreate);
		if(bRes2)
		{
			BYTE strHead[]={0x52, 0x49, 0x46, 0x46, 0xA4, 0x95, 0x08, 0x00,
							0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
							0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0x40, 0x1F, 0x00, 0x00, 0x80, 0x3E, 0x00, 0x00,
							0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61};
			fl2.Write(strHead,40);
			fl2.Write(&flsize,4);
			fl2.Write(pBuffer,flsize);

			fl2.Close();

			try { CFile::Remove(csFile); }
			catch(CFileException) {}
		}
		free(pBuffer);
		return (DWORD)(flsize/16);
	}
	return 0;
}

void CVideoCall::InsertFrame(CObList& olFrames, CFrame263* pFrame, DWORD& dwPreviousFrame)
{
	if( (pFrame->GetFrameNumber()>dwPreviousFrame) || (dwPreviousFrame==0) )
	{
		pFrame->SetTimerValue( (pFrame->GetFrameNumber()-dwPreviousFrame) * 30 );
		olFrames.AddTail(pFrame);
		dwPreviousFrame=pFrame->GetFrameNumber();
	}
	else if(dwPreviousFrame!=0)
	{
		pFrame->SetTimerValue( pFrame->GetFrameNumber() * 30 );
		olFrames.AddTail(pFrame);
		dwPreviousFrame=pFrame->GetFrameNumber();
	}
	else
		delete pFrame;
}

void CVideoCall::RenderFrame(BYTE* pDecodedFrame, CFrame263* pFrame, DWORD dwFileSize, BYTE* pVideoBuffer)
{
	if(pFrame->GetDecoded())
	{
		memcpy(pDecodedFrame,pFrame->GetDecoded(),pFrame->GetDecodedSize());
	}
	else
	{
		DWORD dwSize=dwFileSize-(pFrame->GetBuffer()-pVideoBuffer);
		int nRet;
		pFrame->GetBuffer()[0]=0;
		nRet=DecompressFrame(pFrame->GetBuffer(),pFrame->GetBufferSize(),pDecodedFrame,80000,0);

		pFrame->SetDecoded(pDecodedFrame,80000);
	}
}

void CVideoCall::ExportVideo(DWORD dwAudioDuration)
{
	CString csFile=m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".263";

	CFile fl;
	size_t flsize=0;
	BYTE* pBuffer=NULL;
	BOOL bRes=fl.Open(csFile,CFile::modeRead);
	if(!bRes)
		return;

	CObList olFrames;

	flsize=fl.GetLength();
	pBuffer=(BYTE*)malloc(flsize);
	fl.Read(pBuffer,flsize);
	fl.Close();

	DWORD dwValue;
	DWORD dwPosFirst=0;

	if(!flsize)
	{
		free(pBuffer);
		return;
	}

	DWORD dwPreviousFrame=0;
	DWORD dwDuration=0;
	for(DWORD dwPos=0;dwPos<flsize-3;dwPos++)
	{
		dwValue=pBuffer[dwPos];
		dwValue<<=8;
		dwValue+=pBuffer[dwPos+1];
		dwValue<<=8;
		dwValue+=pBuffer[dwPos+2];
		dwValue>>=2;

		if(dwValue==0x20)
		{
			if(!dwPosFirst)
				dwPosFirst=dwPos;
			CFrame263* pFrame=new CFrame263(&pBuffer[dwPos],flsize-dwPos);
			DWORD dwTime=pFrame->GetTimerValue();
			if( (dwTime>0) && (dwTime<10000) )
				dwDuration+=dwTime;
			InsertFrame(olFrames, pFrame, dwPreviousFrame);
		}
	}
	if(dwAudioDuration)
		dwDuration=dwAudioDuration;

	InitH263Decoder();
	BYTE* pDecodedFrame=(BYTE*)malloc(80000);

	CString csFileSave=m_csPath+"\\"+m_csStartTime+" "+m_csFrom+".avi";
	CAVIFile	oAviFile(csFileSave,176,144);

	DWORD dwFrameMeanDuration=dwDuration/olFrames.GetCount();
	oAviFile.SetFrameRate(1000/dwFrameMeanDuration);

	CPaintDC dcMem(AfxGetMainWnd());
	for(POSITION pos=olFrames.GetHeadPosition();pos;)
	{
		CFrame263* pFrame=(CFrame263*)olFrames.GetNext(pos);
		RenderFrame(pDecodedFrame, pFrame, flsize, pBuffer);

		BYTE* pDecoded=pFrame->GetDecoded();

		CBitmap bmp;
		bmp.CreateCompatibleBitmap(&dcMem,176,144);

		CDC dcMemory;
		dcMemory.CreateCompatibleDC(&dcMem);
		dcMemory.SelectObject(&bmp);

		if(pDecoded)
		{
			DWORD dwPos=0;
			for(int i=0;i<144;i++)
			{
				for(int j=0;j<176;j++)
				{
					COLORREF clrPixel=RGB(pDecoded[dwPos],pDecoded[dwPos+1],pDecoded[dwPos+2]);

					BYTE bRLeft=j?pDecoded[dwPos-3]:pDecoded[dwPos];
					BYTE bRUp=i?pDecoded[dwPos-(3*176)]:pDecoded[dwPos];
					BYTE bRRight=(j<175)?pDecoded[dwPos+3]:pDecoded[dwPos];
					BYTE bRDown=(i<143)?pDecoded[dwPos+(3*176)]:pDecoded[dwPos];
					BYTE bR=(bRLeft+bRUp+bRRight+bRDown)/4;

					BYTE bGLeft=j?pDecoded[dwPos-3+1]:pDecoded[dwPos+1];
					BYTE bGUp=i?pDecoded[dwPos-(3*176)+1]:pDecoded[dwPos+1];
					BYTE bGRight=(j<175)?pDecoded[dwPos+3+1]:pDecoded[dwPos+1];
					BYTE bGDown=(i<143)?pDecoded[dwPos+(3*176)+1]:pDecoded[dwPos+1];
					BYTE bG=(bGLeft+bGUp+bGRight+bGDown)/4;

					BYTE bBLeft=j?pDecoded[dwPos-3+2]:pDecoded[dwPos+2];
					BYTE bBUp=i?pDecoded[dwPos-(3*176)+2]:pDecoded[dwPos+2];
					BYTE bBRight=(j<175)?pDecoded[dwPos+3+2]:pDecoded[dwPos+2];
					BYTE bBDown=(i<143)?pDecoded[dwPos+(3*176)+2]:pDecoded[dwPos+2];
					BYTE bB=(bBLeft+bBUp+bBRight+bBDown)/4;

					COLORREF clrPixelAntialiasing=RGB(bR,bG,bB);
					dcMemory.SetPixel(j,i,clrPixelAntialiasing);
					dwPos+=3;
				}
			}
		}

		dcMem.BitBlt(0,0,176,144,&dcMemory,0,0,SRCCOPY);

		oAviFile.AddFrame(bmp);
	}

	try { CFile::Remove(csFile); }
	catch(CFileException) {}

	free(pDecodedFrame);
	free(pBuffer);
}
