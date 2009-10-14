#pragma once
#include "afxwin.h"


////////////////////////////////////////////
//	CiPeeCV : the iPee CV engine

//-- Define the engine's event constants and structures
#define WM_IPEE_EVT		WM_USER+1	//WM code for iPee engine events
#define IPEE_EVT_STATUS	1			//evt code for status updates
#define IPEE_EVT_ERROR	2			//evt code for errors

//-- Define some more engine constants
#define IPEE_FRM_WIDTH	320
#define IPEE_FRM_HEIGHT	240
#define IPEE_FRM_RATE	30

//a standard message struct to be passed away from the engine (to the control panel)
struct iPeeStdMsg {
	int iErrCode;
	LPCTSTR lpMsg;
	bool bNeedCleanup;
};

//callback for enumerating file names of capture source
typedef bool (*CAPFILEENUMPROC) (void* pTag, CString& sFile);


class CiPeeCV
{
public:
	//construction (with a control panel window) and default destruction
	CiPeeCV(CWnd* pWndCP);
	~CiPeeCV(void);

// -- Fields ------------------
public:
	CWnd* m_pWndCP;			//the control panel's window
	CWinThread* m_pThread;	//the engine's worker thread instance
	void* m_hEvtFinished;	//an event set when the thread is finished running
	bool m_bTerminating;	//set when the worker thread needs to terminate
	CAPFILEENUMPROC m_capFileEnumProc;	//enumeration proc for capture files
	void* m_pFileEnumTag;	//tag data to be passed to the file enum proc
	int m_iCamID;			//the cam ID used for input
	HWND m_hWndGame;		//the handle to the game window recieving mouse events
	SOCKET m_sock;			//the socket to the iPee gamer for sending events
	
	//image processing parameters
	int m_iMNRL;
	int m_iModMin;
	int m_iModMax;
	int m_iBounds;
	
	int m_iRho;
	int m_iHoughThreshold;
	int m_iMinLineLen;
	int m_iMaxLineGap;

	bool m_bLearning;
	int m_iAngleFilter;
	int m_iPeeStartPos;
	
	int m_iXOffset;
	int m_iYOffset;
	int m_iWFactor;
	int m_iHFactor;


// -- Methods -----------------
public:
	HWND FindGameWindow(void);
	void SendMouseInput(WSABUF* pBuf);
	void StartFromCam(int iCamID);
	void StartFromFiles(CAPFILEENUMPROC capFileEnumProc, void* pTag);
	void Stop(void);
	void Shutdown(void);
	void ConnectGamer(LPCTSTR sIP, int iPort);

	void SendStatusEvt(LPCTSTR lpMsg, bool bNeedCleanup = false);
	void SendErrEvt(LPCTSTR lpMsg, int iErr = NULL, bool bNeedCleanup = false);
};

// -- CV Worker Thread --------
UINT iPeeCV(CiPeeCV* pThis);

//cv mouse input callback
void OnCVMouse(int evt, int x, int y, int flags, void* param);