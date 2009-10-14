#include "StdAfx.h"
#include "cv.h"
#include "cvaux.h"
#include "highgui.h"
#include "iPee.h"
#include "iPeeCV.h"

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
		!(iRes = cvNamedWindow("iPeeFG")) ||
		!(iRes = cvNamedWindow("iPeeAux"))) {
		SendErrEvt(_T("Failed to create CV windows"), iRes);
		return;
	}

	cvSetMouseCallback("iPeeOrig", OnCVMouse, this);
	
	//load settings from file
	try {
		CStdioFile fSettings(theApp.GetWithPath(_T("settings.dat")), 
			CFile::modeRead | CFile::typeBinary);
		fSettings.Read((void*)&m_iMNRL, sizeof(int));
		fSettings.Read((void*)&m_iModMin, sizeof(int));
		fSettings.Read((void*)&m_iModMax, sizeof(int));
		fSettings.Read((void*)&m_iBounds, sizeof(int));
		
		fSettings.Read((void*)&m_iRho, sizeof(int));
		fSettings.Read((void*)&m_iHoughThreshold, sizeof(int));
		fSettings.Read((void*)&m_iMinLineLen, sizeof(int));
		fSettings.Read((void*)&m_iMaxLineGap, sizeof(int));
		
		fSettings.Read((void*)&m_iAngleFilter, sizeof(int));
		fSettings.Read((void*)&m_iPeeStartPos, sizeof(int));

		fSettings.Read((void*)&m_iXOffset, sizeof(int));
		fSettings.Read((void*)&m_iYOffset, sizeof(int));
		fSettings.Read((void*)&m_iWFactor, sizeof(int));
		fSettings.Read((void*)&m_iHFactor, sizeof(int));

		fSettings.Close();
	}
	catch (...) {
		m_iMNRL = 10;
		m_iModMin = 10;
		m_iModMax = 10;
		m_iBounds = 10;

		m_iRho = 10;
		m_iHoughThreshold = 20;
		m_iMinLineLen = 30;
		m_iMaxLineGap = 20;

		m_iAngleFilter = 30;
		m_iPeeStartPos = 30;

		m_iXOffset = 20;
		m_iYOffset = 61;
		m_iWFactor = 93;
		m_iHFactor = 10;
	}
		
	//create the paramaters track-bars
	cvCreateTrackbar("MNRL", "iPeeFG", &m_iMNRL, 10, NULL);
	cvCreateTrackbar("ModMin", "iPeeFG", &m_iModMin, 100, NULL);
	cvCreateTrackbar("ModMax", "iPeeFG", &m_iModMax, 100, NULL);
	cvCreateTrackbar("Bounds", "iPeeFG", &m_iBounds, 100, NULL);

	cvCreateTrackbar("Rho", "iPeeOrig", &m_iRho, 50, NULL);
	cvCreateTrackbar("Threshold", "iPeeOrig", &m_iHoughThreshold, 100, NULL);
	cvCreateTrackbar("Min Line Len.", "iPeeOrig", &m_iMinLineLen, 150, NULL);
	cvCreateTrackbar("Max Line Gap", "iPeeOrig", &m_iMaxLineGap, 150, NULL);

	cvCreateTrackbar("X Offset", "iPeeAux", &m_iXOffset, 100, NULL);
	cvCreateTrackbar("Y Offset", "iPeeAux", &m_iYOffset, 100, NULL);
	cvCreateTrackbar("W Factor", "iPeeAux", &m_iWFactor, 100, NULL);
	cvCreateTrackbar("H Factor", "iPeeAux", &m_iHFactor, 100, NULL);
	
	//create the thread's sync event
	m_hEvtFinished = CreateEvent(NULL, TRUE, TRUE, NULL);

	//startup winsock services
	WSADATA wsaData;
	if ((iRes = WSAStartup(MAKEWORD(2,2), &wsaData)) != ERROR_SUCCESS) {
		SendErrEvt(_T("Failed to start winsock"), iRes);
		return;
	}
	m_sock = NULL;
}

//default destruction
CiPeeCV::~CiPeeCV(void)
{
}


// -- Methods -----------------

//searches for the flash player's window
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

//connects the socket to the iPee gamer proxy
void CiPeeCV::ConnectGamer(LPCTSTR sIP, int iPort)
{
	int iRes = 0;

	//disconnect current socket if exists
	if (m_sock != NULL) {
		closesocket(m_sock);
		m_sock = NULL;
		Sleep(3000);
	}

	//if no IP, run localy, else attempt a connection to the iPeeGamer
	if (sIP == NULL || *sIP == NULL) {
		if (FindGameWindow() != NULL)
			SendStatusEvt(_T("Flash game window found and bound"));
	}
	else {
		
		//set up a TCP/IP socket
		m_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, NULL);
		
		//connect to the IP address specified
		sockaddr_in addr;
		addr.sin_addr.s_addr = inet_addr(CT2A(sIP));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(iPort);
		
		if ((iRes = connect(m_sock, (sockaddr*)&addr, sizeof(sockaddr_in))) != ERROR_SUCCESS)
			SendErrEvt(_T("Could not connect to iPee gamer"), iRes);

		SendStatusEvt(_T("iPee gamer connected"));
	}
}

//starts the engine's worker thread with the specified cam as input
void CiPeeCV::StartFromCam(int iCamID)
{
	if (m_pThread)
		SendErrEvt(_T("Engine's thread already running"));
	else
	{
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
	
	//disconnect gamer if there is one
	if (m_sock) {
		closesocket(m_sock);
		m_sock = NULL;
	}

	cvDestroyAllWindows();

	//save the settings to file
	CStdioFile fSettings(theApp.GetWithPath(_T("settings.dat")), CFile::modeCreate | 
		CFile::modeWrite | CFile::typeBinary);
	fSettings.Write(&m_iMNRL, sizeof(int));
	fSettings.Write(&m_iModMin, sizeof(int));
	fSettings.Write(&m_iModMax, sizeof(int));
	fSettings.Write(&m_iBounds, sizeof(int));

	fSettings.Write(&m_iRho, sizeof(int));
	fSettings.Write(&m_iHoughThreshold, sizeof(int));
	fSettings.Write(&m_iMinLineLen, sizeof(int));
	fSettings.Write(&m_iMaxLineGap, sizeof(int));

	fSettings.Write(&m_iAngleFilter, sizeof(int));
	fSettings.Write(&m_iPeeStartPos, sizeof(int));
	
	fSettings.Write(&m_iXOffset, sizeof(int));
	fSettings.Write(&m_iYOffset, sizeof(int));
	fSettings.Write(&m_iWFactor, sizeof(int));
	fSettings.Write(&m_iHFactor, sizeof(int));
	
	fSettings.Close();

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


//transofrms and sends the gamer the specified mouse input
void CiPeeCV::SendMouseInput(WSABUF* pBuf)
{
	int iRes;

	//if there is a socket attempt to send the event
	if (m_sock != NULL) {
		DWORD dw;
		if ((iRes = WSASend(m_sock, pBuf, 1, &dw, NULL, NULL, NULL)) != ERROR_SUCCESS) {
			SendErrEvt(_T("Failed to send data to gamer, re-connect"), iRes);
			closesocket(m_sock);
			m_sock = NULL;
		}
	}
	
	//if there is a gamer window send to by message
	else if (m_hWndGame != NULL)
		SendMessage(m_hWndGame, WM_MOUSEMOVE, NULL, 
			MAKELPARAM(*(int*)pBuf->buf, *((int*)pBuf->buf + 4)));
}

// -- CV Worker Thread --------
UINT iPeeCV(CiPeeCV* pThis)
{
	int iRes = 0;
	int i;
	
	pThis->SendStatusEvt(_T("iPee CV worker thread is up"));
	
	CvCapture* capture = NULL;		//the CV capture
	CString sFile;					//the currently playing input file
	
	IplImage* iplCap = NULL;		//raw captured image
	IplImage* iplFlip = NULL;		//flipped image
	IplImage* iplOrig = NULL;		//original frame
	IplImage* iplFG = NULL;			//image after background subtraction
	
	//IplImage* iplGray = NULL;		//gray-scale version of image
	//IplImage* iplCanny = NULL;	//canny edge detection result
	//IplImage* iplCanny32b = NULL;	//canny edge detection result
	//IplImage* iplCanny8b = NULL;	//canny edge detection result
	//IplImage* iplBack32b = NULL;	//16-bit background
	//IplImage* iplBack8b  = NULL;	//8-bit background
	//IplImage* iplTemp = NULL;		
	//IplImage* iplTemp1 = NULL;		
	//IplImage* iplTemp2 = NULL;	
	//IplImage* iplSqr = NULL;		
	//IplImage* iplSqrAvg = NULL;	
	//IplImage* iplStd  = NULL;	
	//IplImage* laplace = 0;
	//IplImage* colorlaplace = 0;
	//IplImage* planes[3] = { 0, 0, 0 };

	CvMemStorage* storage = cvCreateMemStorage(0);	//memory storage for line-detection res
	CvSeq* seqLines = 0;							//sequence of lines from Hough
	CvPoint* line;		//the current line

	bool bLearningImg;	//has the learning model been commited
	
	//CvSeq* seq = cvCreateSeq(0, sizeof(CvSeq), sizeof(CvPoint), storage);
	//CvPoint apnt[] = {CvPoint(), CvPoint(), CvPoint(), CvPoint(), CvPoint()};
	//CvPoint pnt, pnt1;
	//pnt.x = pnt.y = 0;
	//pnt1.x = pnt1.y = 0;
	//int dx, dy, n = 0;
	//pThis->m_iX = pThis->m_iY = 0;

	//create the background code book model
	CvBGCodeBookModel* model = cvCreateBGCodeBookModel();

	//create a buffer for the result point (use WSABUF for optimizing to winsock)
	int iPnt[2];
	WSABUF buf;
	buf.buf = (char*)iPnt;
	buf.len = sizeof(int) * 2;

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
		
		//set capture properties for image size and frame rate
		cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, IPEE_FRM_HEIGHT);
		cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, IPEE_FRM_WIDTH);
		cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, IPEE_FRM_RATE);
		
		bLearningImg = false;

		/*CvVideoWriter* cvWriter = cvCreateAVIWriter(
			CT2A(theApp.GetWithPath(_T("01_laplace.avi"))), 
			CV_FOURCC('M','J','P','G'), 30, cvSize(320,240));
		CvMat* mat = NULL;
		CvMat* mat2 = NULL;*/
		
		//main thread loop
		for (int iFrame = 0; !pThis->m_bTerminating; iFrame++)
		{
			//grab and retrieve a frame from capture
			iplCap = cvQueryFrame(capture);
			if(!iplCap)
				break;	//this means the end of the file
			
			//if this is the first frame create image containers
			if (!iplOrig) {
				iplOrig = cvCreateImage(cvSize(iplCap->height, iplCap->width), 
					iplCap->depth, iplCap->nChannels);
				iplFlip = cvCreateImage(cvSize(iplOrig->width, iplOrig->height), 
					iplCap->depth, iplCap->nChannels);
				iplFG	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
					IPL_DEPTH_8U, 1);
			}
			
			//if (pThis->m_iX | pThis->m_iY) {
			//	if (!mat)
			//		mat = cvCreateMat(iplOrig->width / 2, iplOrig->height / 2, CV_8UC3); 
			//	cvGetSubArr(iplCap, mat, cvRect(pThis->m_iX, pThis->m_iY, 
			//		iplOrig->height / 2, iplOrig->width / 2));
			//	if (!mat2)
			//		mat2 = cvCreateMat(mat->cols, mat->rows, CV_8UC3);
			//	cvTranspose(mat, mat2);
			//	cvResize(mat2, iplOrig);
			//}
			//else
			//	cvCopy(iplCap, iplOrig);
			
			//transpose and flip the image
			cvTranspose(iplCap, iplOrig);
			cvFlip(iplOrig, iplFlip, 0);
			cvCopy(iplFlip, iplOrig);
			//cvCopy(iplCap, iplOrig);			

			//if this is the first frame create the image objects
			//if(!iplCanny)
			//{
			//	for(i = 0; i < 3; i++)
			//		planes[i] = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), 8, 1 );
			//	laplace = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), IPL_DEPTH_16S, 1 );
			//	colorlaplace = cvCreateImage( cvSize(iplFrame->width,iplFrame->height), 8, 3 );

			//	iplGray		= cvCreateImage(cvSize(iplOrig->width, iplOrig->height), 
			//		IPL_DEPTH_8U, 1);
			//	iplCanny	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_8U, 1);
			//	iplBack32b	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_32F, 1);
			//	iplBack8b	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_8U, 1);
			//	iplCanny32b	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_32F, 1);
			//	iplCanny8b	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		iplOrig->depth, 1);
			//					
			//	iplTemp	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_8U, 1);
			//	iplTemp1	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		iplOrig->depth, iplOrig->nChannels);
			//	iplTemp2	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_8U, 1);
			//	iplSqr	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_32F, 1);
			//	iplSqrAvg	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_32F, 1);
			//	iplStd	= cvCreateImage(cvSize(iplOrig->width, iplOrig->height),
			//		IPL_DEPTH_32F, 1);
			//}

			//cvCvtPixToPlane( iplFrame, planes[0], planes[1], planes[2], 0 );
			//for(i = 0; i < 3; i++)
			//{
			//	cvLaplace( planes[i], laplace, 3 );
			//	cvConvertScaleAbs( laplace, planes[i], 1, 0 );
			//}
			//cvCvtPlaneToPix( planes[0], planes[1], planes[2], 0, colorlaplace );
			//colorlaplace->origin = iplFrame->origin;

			//convert the image's color space
			//cvCvtColor(iplOrig, iplGray, CV_BGR2GRAY);
			cvCvtColor(iplOrig, iplOrig, CV_BGR2YCrCb);
			
			//cvAbsDiff(iplGray, iplBack8b, iplFG);
			//cvCopy(iplGray, iplBack8b);
			
			if (pThis->m_bLearning) {
				//cvRunningAvg(iplGray, iplBack32b, 0.1);

				//set the learning parameters
				model->cbBounds[0] = model->cbBounds[1] = 
					model->cbBounds[2] = pThis->m_iBounds;
				
				//learn
				cvBGCodeBookUpdate(model, iplOrig);
				bLearningImg = false;
			}
			else {
				if (!bLearningImg) {
					//cvConvert(iplBack32b, iplBack8b);
					cvBGCodeBookClearStale( model, model->t / pThis->m_iMNRL);
					bLearningImg = true;
				}
			}
			
			//set the BG difference parameters
			model->modMax[0] = model->modMax[1] = model->modMax[2] = pThis->m_iModMax;
			model->modMin[0] = model->modMin[1] = model->modMin[2] = pThis->m_iModMin;
			
			//make the difference
			cvBGCodeBookDiff(model, iplOrig, iplFG);

			//cvConvert(iplBack32b, iplBack8b);
			//cvAbsDiff(iplGray, iplBack8b, iplFG);
			//
			//double dMin, dMax;
			//cvMinMaxLoc(iplGray, &dMin, &dMax);
			//cvNormalize(iplGray, iplGray, dMin, dMax, CV_MINMAX);

			//cvConvert(iplGray, iplTemp);
			//
			//cvConvert(iplBack32b, iplBack8b);
			//if (iFrame > 0) {
			//	cvAbsDiff(iplTemp2, iplBack8b, iplCanny);
			//	cvThreshold(iplCanny, iplCanny, 100, 255, CV_THRESH_BINARY);
			//}
			//cvCopy(iplTemp2, iplBack8b);
			//
			//cvRunningAvg(iplTemp2, iplBack32b, 0.9);
			//
			//cvConvert(iplGray, iplTemp2);
			//			
			//cvMul(iplBack32b, iplBack32b, iplSqr);
			//cvSub(iplSqrAvg, iplSqr, iplTemp);
			//cvPow(iplTemp, iplStd, 0.5);
			//cvSet(iplTemp, cvScalar(1));
			//cvMax(iplStd, iplTemp, iplStd); 
			//
			//cvAbsDiff(iplTemp2, iplBack32b, iplTemp);
			//cvDiv(iplTemp, iplStd, iplTemp1);
			//cvConvert(iplTemp1, iplFG);
			//
			//cvRunningAvg(iplFG, iplBack8b, 0.2);
			//
			//cvMul(iplTemp2, iplTemp2, iplSqr);
			//cvRunningAvg(iplSqr, iplSqrAvg, 0.1);
			//cvRunningAvg(iplTemp2, iplBack32b, 0.1);
			//			
			//cvConvert(iplCanny32b, iplCanny8b);
			//cvSub(iplFG, iplCanny8b, iplTemp);
			//
			//cvSmooth(iplFG, iplTemp1, CV_BLUR, 3);
			//cvThreshold(iplTemp1, iplCanny, (double)pThis->m_iCannyThreshold1 / 10.0, 255, CV_THRESH_BINARY);
			//
			//cvRunningAvg(iplCanny, iplCanny32b, 0.2);
			//	
			////perform canny edge detection
			//cvCanny(iplFG, iplCanny, pThis->m_iCannyThreshold1,
			//	pThis->m_iCannyThreshold2);
			
			//perform a probabilistic Hough transform
			seqLines = cvHoughLines2(iplFG, storage, CV_HOUGH_PROBABILISTIC, pThis->m_iRho, 
				CV_PI/180, pThis->m_iHoughThreshold, 
				pThis->m_iMinLineLen, pThis->m_iMaxLineGap);
			
			//iterate lines
			for(i = 0; i < seqLines->total; i++)
			{
				line = (CvPoint*)cvGetSeqElem(seqLines, i);

				//check that line meets angle and starting position filter
				if (90 - cvFastArctan(fabsf((float)(line[0].y - line[1].y)), 
					fabsf((float)(line[0].x - line[1].x))) < (float)pThis->m_iAngleFilter &&
					max(line[0].y, line[1].y) > pThis->m_iPeeStartPos) {

					
					//we need the upper point first
					if (line[1].y < line[0].y) {
						//cvSeqPush(seq, &line[1]);
						//pnt.x += line[1].x;
						//pnt.y += line[1].y;

						//dx = abs(pnt.x - line[1].x);
						//dy = abs(pnt.y - line[1].y);
						//if (pnt.x == 0 || cvSqrt((float)(dx*dx + dy*dy)) < 20) {
						//	pnt.x += line[1].x;
						//	pnt.y += line[1].y;
						//}

						//track the point with a cirlce and calculate X,Y transform
						cvCircle(iplOrig, line[1], 10, CV_RGB(0,0,0), 3, 8);
						iPnt[0] = (int)(
							(double)line[1].x * (double)pThis->m_iWFactor / 5.0 + 
							((double)pThis->m_iXOffset - 50) * 10.0);
						iPnt[1] = (int)(
							(double)line[1].y * (double)pThis->m_iHFactor / 10.0 + 
							((double)pThis->m_iYOffset - 50) * 10.0);
					}
					else {
						//cvSeqPush(seq, &line[0]);
						//pnt.x += line[0].x;
						//pnt.y += line[0].y;

						//dx = abs(pnt.x - line[0].x);
						//dy = abs(pnt.y - line[0].y);
						//if (pnt.x == 0 || cvSqrt((float)(dx*dx + dy*dy)) < 20) {
						//	pnt.x += line[0].x;
						//	pnt.y += line[0].y;
						//	n++;
						//}

						//track the point with a cirlce and calculate X,Y transform
						cvCircle(iplOrig, line[0], 10, CV_RGB(0,0,0), 3, 8);
						iPnt[0] = (int)(
							(double)line[0].x * (double)pThis->m_iWFactor / 5.0 + 
							((double)pThis->m_iXOffset - 50) * 10.0);
						iPnt[1] = (int)(
							(double)line[0].y * (double)pThis->m_iHFactor / 10.0 + 
							((double)pThis->m_iYOffset - 50) * 10.0);
					}
					
					pThis->SendMouseInput(&buf);
					//cvLine(iplOrig, line[0], line[1], CV_RGB(255,255,255), 3);
					
					//we only need Hough's best line
					break;
				}
			}		

			//cvPyrSegmentation(iplCanny, iplTemp2, storage, &seq, 3, pThis->m_iRho, 
			//	pThis->m_iHoughThreshold);
			//for (i = 0; i < seq->total; i++) {
			//	CvConnectedComp* con = (CvConnectedComp*)cvGetSeqElem(seq, i);
			//}

			//if (iFrame % 5 == 0) {
			//	if (seq->total > 0) {
			//		pnt.x /= seq->total;
			//		pnt.y /= seq->total;
			//		CvPoint* ppnt;
			//		CvPoint pnt2;
			//		pnt2.x = pnt2.y = 0;
			//		n = 0;
			//		ppnt = (CvPoint*)cvGetSeqElem(seq, 0);
			//		for (i = 0; i < seq->total; i++) {
			//			dx = abs(ppnt[i].x - pnt.x);
			//			dy = abs(ppnt[i].y - pnt.y);
			//			if (cvSqrt((float)(dx*dx + dy*dy)) < 40) {
			//				pnt2.x += ppnt[i].x;
			//				pnt2.y += ppnt[i].y;
			//				n++;
			//			} 
			//		}
			//		if (n > 0) {
			//			pnt1.x = pnt2.x / n;
			//			pnt1.y = pnt2.y / n;
			//		}

			//		pnt.x = pnt.y = 0;
			//		cvClearSeq(seq);
			//	}
			//}
			//if ((pnt1.y | pnt1.x) != 0)
			//	cvCircle(iplOrig, pnt1, 10, CV_RGB(255,0,0), 3, 8);

			//if (iFrame % 5 == 0)
			//	pnt.x = pnt.y = 0;

			cvShowImage("iPeeOrig", iplOrig);
			cvShowImage("iPeeFG", iplFG);
			//cvShowImage("iPeeAux", iplCanny);

			//cvWriteToAVI(cvWriter, colorlaplace);
			//if (INT_MAX - iFrame < 10)
			//	iFrame = iFrame % 5;

			if( cvWaitKey(33) >= 0 )
				break;
		}	

		//cvReleaseVideoWriter(&cvWriter);

		//cleanup the images
		cvReleaseImage(&iplOrig);
		cvReleaseImage(&iplFG);
		cvReleaseImage(&iplFlip);
		
		//cleanup the capture
		cvReleaseCapture(&capture);
	}

	pThis->m_bTerminating = false;
	pThis->m_pThread = NULL;

	SetEvent(pThis->m_hEvtFinished);
	pThis->SendStatusEvt(_T("iPee CV worker thread is down"));	

	AfxEndThread(iRes);
	return iRes;	//return thread exit code
}

void OnCVMouse(int evt, int x, int y, int flags, void* param) 
{
	/*CiPeeCV* pThis = (CiPeeCV*)param;
	if (evt == CV_EVENT_LBUTTONDOWN) {
		pThis->m_iX = x;
		pThis->m_iY = y;
	}
	else if (evt == CV_EVENT_LBUTTONUP) {
		pThis->m_iW = x - pThis->m_iX;
		pThis->m_iH = y - pThis->m_iY;
	}*/
}