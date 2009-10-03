#include "StdAfx.h"
#include "iPee.h"
#include "iPeeCV.h"
#include "cv.h"
#include "highgui.h"


////////////////////////////////////////////
//	CiPeeCV : the iPee CV engine

//construction (with a target window)
CiPeeCV::CiPeeCV(CWnd* pWndCP = NULL)
{
	int iRes;
	m_pWndCP = pWndCP;
	m_pThread = NULL;

	//create the CV windows
	if (!(iRes = cvNamedWindow("iPeeOrig")) ||
		!(iRes = cvNamedWindow("iPeePass1"))) {
		SendErrEvt(_T("Failed to create CV windows"), iRes);
		return;
	}

	//create the thread's sync event
	m_hEvtFinished = CreateEvent(NULL, TRUE, TRUE, NULL);
}

//default destruction
CiPeeCV::~CiPeeCV(void)
{
}


// -- Methods -----------------

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	TCHAR sWndName[64];
	GetWindowText(hwnd, sWndName, 64);

	if (wcsstr(sWndName, _T("Flash Player")) != NULL)
	{
		((CiPeeCV*)lParam)->m_hWndGame = hwnd;
		return FALSE;
	}
	else 
	{
		((CiPeeCV*)lParam)->m_hWndGame = NULL;
		return TRUE;
	}
}

//find the game window for sending mouse events
HWND CiPeeCV::FindGameWindow(void)
{
	EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)this);

	return m_hWndGame;
}

//starts the engine's worker thread with the specified cam as input
void CiPeeCV::StartFromCam(int iCamID)
{
	if (m_pThread)
		SendErrEvt(_T("Engine's thread already running"));
	else
	{
		//try to find the game window
		FindGameWindow();

		//set the running flags and start the CV engine's worker thread
		m_bTerminating = false;
		m_iCamID = iCamID;
		ResetEvent(m_hEvtFinished);

		m_pThread = AfxBeginThread((AFX_THREADPROC)iPeeCV, this);
	}
}

//starts the engine's worker thread with the specified file enumeration as input
void CiPeeCV::StartFromFiles(CAPFILEENUMPROC capFileEnumProc, void* pTag)
{
	if (m_pThread) 
		SendErrEvt(_T("Engine's thread already running"));
	else
	{
		//try to find the game window
		FindGameWindow();

		//set the files enumeration context
		m_capFileEnumProc = capFileEnumProc;
		m_pFileEnumTag = pTag;
		
		//set the running flags and start the CV engine's worker thread
		m_bTerminating = false;
		m_capFileEnumProc = capFileEnumProc;
		m_iCamID = -1;
		ResetEvent(m_hEvtFinished);
		
		m_pThread = AfxBeginThread((AFX_THREADPROC)iPeeCV, this);
	}
}

//terminates the worker thread
void CiPeeCV::Stop(void)
{
	//flag the worker thread to exit
	m_bTerminating = true;
	if (m_hEvtFinished)
	{
		WaitForSingleObject(m_hEvtFinished, 5000);
		SendStatusEvt(_T("iPee CV worker thread is down"));	
	}
}

//terminates the worker thread and cleans up resources
void CiPeeCV::Shutdown(void)
{
	//flag the worker thread to exit
	m_bTerminating = true;
	if (m_hEvtFinished)
		WaitForSingleObject(m_hEvtFinished, 5000);

	cvDestroyAllWindows();

	m_hEvtFinished = NULL;
	CloseHandle(m_hEvtFinished);
}

//sends the control panel a status event
void CiPeeCV::SendStatusEvt(LPCTSTR lpMsg, bool bNeedCleanup /*=false*/)
{
	iPeeStdMsg* pMsg = new iPeeStdMsg();
	pMsg->lpMsg = lpMsg;
	pMsg->bNeedCleanup = bNeedCleanup;

	m_pWndCP->SendMessage(WM_IPEE_EVT, IPEE_EVT_STATUS, (LPARAM)pMsg);
}

//sends the control panel an error event
void CiPeeCV::SendErrEvt(LPCTSTR lpMsg, int iErr /*=NULL*/, bool bNeedCleanup /*=false*/)
{
	iPeeStdMsg* pMsg = new iPeeStdMsg();
	pMsg->iErrCode = iErr;
	pMsg->lpMsg = lpMsg;
	pMsg->bNeedCleanup = bNeedCleanup;

	m_pWndCP->SendMessage(WM_IPEE_EVT, IPEE_EVT_ERROR, (LPARAM)pMsg);
}


//transofrms and sends the game the specified mouse input
void SendMouseInput(int x, int y)
{
	if (m_hWndGame != NULL)
		SendMessage(pThis->m_hWndGame, WM_MOUSEMOVE, NULL, MAKELPARAM(x++, 300));
}

// -- CV Worker Thread --------
UINT iPeeCV(CiPeeCV* pThis)
{
	int iRes = 0;
	pThis->SendStatusEvt(_T("iPee CV worker thread is up"));

	CvCapture* capture = NULL;		//the CV capture
	CString sFile;					//the currently playing input file
	IplImage* iplFrame = 0;
	IplImage* laplace = 0;
	IplImage* colorlaplace = 0;
	IplImage* iplCanny = 0;
	IplImage* iplGray = 0;
    IplImage* planes[3] = { 0, 0, 0 };
    int i;
	
	//this loop enumerates through the input files, of capturing from cam
	//only one iteration of this loop will run
	while (!pThis->m_bTerminating &&
		(pThis->m_iCamID >= 0 || pThis->m_capFileEnumProc(pThis->m_pFileEnumTag, sFile)))
	{
		if (pThis->m_iCamID >= 0)	//start capturing from cam
			capture = cvCaptureFromCAM(0);
		else						//start capturing from the file
			capture = cvCaptureFromFile(CT2A(sFile));
		
		if (!capture)
		{
			iRes = -1;
			break;
		}
		
		//CvVideoWriter* cvWriter = cvCreateAVIWriter(
		//	CT2A(theApp.GetWithPath(_T("01_laplace.avi"))), 
		//	CV_FOURCC('M','J','P','G'), 30, cvSize(320,240));

		//main thread loop
		while (!pThis->m_bTerminating)
		{
			//grab and retrieve a frame from capture
			iplFrame = cvQueryFrame( capture );
			if( !iplFrame )
				break;	//this means the end of the file
			
			if( !laplace )
			{
				for( i = 0; i < 3; i++ )
					planes[i] = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), 8, 1 );
				laplace = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), IPL_DEPTH_16S, 1 );
				colorlaplace = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), 8, 3 );
				
				iplGray = cvCreateImage( cvSize(iplFrame->width, iplFrame->height), IPL_DEPTH_8U, 1 );
				
				iplCanny = cvCreateImage(cvSize(iplFrame->width,iplFrame->height), IPL_DEPTH_8U, 1);
			}
			
			cvCvtPixToPlane( iplFrame, planes[0], planes[1], planes[2], 0 );
			for( i = 0; i < 3; i++ )
			{
				cvLaplace( planes[i], laplace, 3 );
				cvConvertScaleAbs( laplace, planes[i], 1, 0 );
			}
			cvCvtPlaneToPix( planes[0], planes[1], planes[2], 0, colorlaplace );
			colorlaplace->origin = iplFrame->origin;
			
			//convert original color image (3 channel rgb color image) to gray-level image
			cvCvtColor( iplFrame, iplGray, CV_BGR2GRAY );
			
			//cvCanny(iplGray, iplCanny, 100, 200);

			cvShowImage("iPeeOrig", iplFrame);
			cvShowImage("iPeePass1", colorlaplace );
			//cvWriteToAVI(cvWriter, colorlaplace);

			if( cvWaitKey(10) >= 0 )
				break;
		}

		//cvReleaseVideoWriter(&cvWriter);

		//cleanup the capture
		cvReleaseCapture( &capture );
	}

	pThis->m_bTerminating = false;
	pThis->m_pThread = NULL;

	SetEvent(pThis->m_hEvtFinished);
	pThis->SendStatusEvt(_T("iPee CV worker thread is down"));	
	
	AfxEndThread(iRes);
	return iRes;	//return thread exit code
}