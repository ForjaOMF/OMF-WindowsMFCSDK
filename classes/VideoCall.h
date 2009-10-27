// VideoCallReceiver.h: archivo de encabezado
//

class CSIPSocket;
class CFrame263;

class CVideoCall : public CObject
{
// Construcción
public:
	CVideoCall(CString csStartTime, CString csFrom, CString csLocalIP, CString csPath, CString csCallId, CString csBranch, CString csFromTag, int nToTag);
	virtual ~CVideoCall();

	void CreateSockets();

	void SendMultimediaData(CString csAckData);

	static void OnEventAudio(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);
	static void OnEventAudioCtrl(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);
	static void OnEventVideo(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);
	static void OnEventVideoCtrl(DWORD dwCookie,UINT nCode,char* pBufData,DWORD dwLenData,int nErrorCode);

	void Terminate();

	void Echo(char* pBufData,DWORD dwLenData);
	void SaveAudioData(char* pBufData,DWORD dwLenData);

	void Mirror(char* pBufData,DWORD dwLenData);
	void SaveVideoData(char* pBufData,DWORD dwLenData);

	short DecodeByte(BYTE mulaw);
	void DecodeULaw(BYTE* pData, DWORD dwLen, BYTE* pRes);
	void DecodeALaw(BYTE* pData, DWORD dwLen, BYTE* pRes);

	DWORD ExportAudio();

	void InsertFrame(CObList& olFrames, CFrame263* pFrame, DWORD& dwPreviousFrame);
	void RenderFrame(BYTE* pDecodedFrame, CFrame263* pFrame, DWORD dwFileSize, BYTE* pVideoBuffer);
	void ExportVideo(DWORD dwAudioDuration);

	UINT GetAudioPort(){return m_nLocalAudioPort;};
	UINT GetVideoPort(){return m_nLocalVideoPort;};

	CString	GetCallId(){return m_csCallId;};
	CString	GetBranch(){return m_csBranch;};
	CString	GetFromTag(){return m_csFromTag;};
	int	GetToTag(){return m_nToTag;};

	CString GetStartTime(){return m_csStartTime;};
	CString GetFrom(){return m_csFrom;};

// Implementación
protected:

	CSIPSocket* m_pSocketAudio;
	CSIPSocket* m_pSocketAudioCtrl;
	CSIPSocket* m_pSocketVideo;
	CSIPSocket* m_pSocketVideoCtrl;

	UINT m_nLocalAudioPort;
	UINT m_nLocalVideoPort;

	UINT m_nPortAudio;
	UINT m_nPortVideo;

	CString	m_csCallId;
	CString	m_csBranch;
	CString	m_csFromTag;
	int	m_nToTag;

	CString m_csStartTime;
	CString m_csFrom;

	CString m_csLocalIP;
	CString m_csPath;

	bool m_bVideoReceived;
};
