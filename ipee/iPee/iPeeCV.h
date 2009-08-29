#pragma once
#include "afxwin.h"


////////////////////////////////////////////
//	CiPeeCV : the iPee CV engine

//-- Define the engine's event constants and structures
#define WM_IPEE_EVT		WM_USER+1	//WM code for iPee engine events
#define IPEE_EVT_STATUS	1			//evt code for status updates
#define IPEE_EVT_ERROR	2			//evt code for errors

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

// -- Methods -----------------
public:
	void StartFromCam(int iCamID);
	void StartFromFiles(CAPFILEENUMPROC capFileEnumProc, void* pTag);
	void Stop(void);
	void Shutdown(void);

	void SendStatusEvt(LPCTSTR lpMsg, bool bNeedCleanup = false);
	void SendErrEvt(LPCTSTR lpMsg, int iErr = NULL, bool bNeedCleanup = false);
};

// -- CV Worker Thread --------
UINT iPeeCV(CiPeeCV* pThis);