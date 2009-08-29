#include "stdafx.h"
#include "iPee.h"
#include "iPeeDlg.h"
#include "iPeeCV.h"
#include "TabDebug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////
// CiPeeDlg dialog - the main application's dialog, makes up
//		for the iPee's system control panel

//default constructor
CiPeeDlg::CiPeeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CiPeeDlg::IDD, pParent)
{
	//member init
	m_piPeeCV = NULL;

	//get the app's icon from resource
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//------ UI init --------------

	//background brush for the status text box
	m_pBgBrush = new CBrush(RGB(230,230,230));
		
	//create and initialize the tab-pages
	m_iCurTab = 0;
	m_pdlgTabs[0] = new CTabDebug(this);
}


//-- Methods ---------------

//appends a line to the status edit control
void CiPeeDlg::AppendStatus(LPCTSTR lpStatus)
{
	m_txtStatus.SetSel(MAXINT, MAXINT);
	m_txtStatus.ReplaceSel(lpStatus);
	m_txtStatus.ReplaceSel(_T("\n"));
}


// -- Event Handlers -------------------------------

//message map implementation
BEGIN_MESSAGE_MAP(CiPeeDlg, CDialog)
	ON_WM_PAINT()
	{ WM_IPEE_EVT, 0, 0, 0, AfxSig_v_w_l, 		//ON_WM_IPEE
		(AFX_PMSG)(AFX_PMSGW) 
		(static_cast< void (AFX_MSG_CALL CWnd::*)(WPARAM wParam, LPARAM lParam) > 
		( &CiPeeDlg :: OniPeeEvt)) },
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_MNU_LOADENGINE, &CiPeeDlg::OnMnuLoadEngine)
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_COMMAND(ID_MNU_UNLOADENGINE, &CiPeeDlg::OnMnuUnloadEngine)
	ON_COMMAND(ID_IPEE_RESTARTENGINE, &CiPeeDlg::OnMnuRestartEngine)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CiPeeDlg::OnTcnSelchangeTab)
	ON_COMMAND(ID_IPEE_STARTCAM, &CiPeeDlg::OnMnuStartCAM)
	ON_COMMAND(ID_IPEE_STOPCAM, &CiPeeDlg::OnMnuStopCAM)
END_MESSAGE_MAP()

//initDialog - called by the framework when dlg is loaded
BOOL CiPeeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	//set the big and small icons (respectively) for this dialog
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	//-- UI init ----------------------

	//calculate positioning for the tab dialogs
	CRect rectClient, rectWnd;
	m_tabCtrl.GetClientRect(rectClient);
	m_tabCtrl.AdjustRect(FALSE, rectClient);
	m_tabCtrl.GetWindowRect(rectWnd);
	ScreenToClient(rectWnd);
	rectClient.OffsetRect(rectWnd.left,rectWnd.top);

	//create and position the tab dialogs and hide them
	BOOL b = m_pdlgTabs[0]->Create(IDD_TAB_DEBUG, this);
	m_pdlgTabs[0]->SetWindowPos(&CWnd::wndTop, rectClient.left, rectClient.top + 20, 
		rectClient.Width(), rectClient.Height() - 20, SWP_SHOWWINDOW);
	
	//insert the tabs to the tab control
	m_tabCtrl.InsertItem(0, _T("Debug"));

	OnMnuLoadEngine();
	
	return TRUE;  // return TRUE unless we set the focus to a control
}

//dialog destruction - unload all the acquired resource
void CiPeeDlg::OnDestroy()
{
	CDialog::OnDestroy();
	OnMnuUnloadEngine();
	
	m_pdlgTabs[0]->DestroyWindow();
	delete m_pdlgTabs[0];

	delete m_pBgBrush;

	//terminate abruptly because comm-dialogs mess up and dead-lock highgui
	TerminateProcess(GetCurrentProcess(), NULL);
}

//paint - required to draw the icon when the dialog is minimized
void CiPeeDlg::OnPaint()
{
	//if we're minimized draw the icon, otherwise, forward event to framework
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting
		
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		//center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		//draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//ctlColor - for UI coloring
HBRUSH CiPeeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_TXT_STATUS:	//status edit-control
		pDC->SetBkColor(RGB(230,230,230));
		pDC->SetTextColor(RGB(50,50,50));
		return (HBRUSH)m_pBgBrush;
	
	default:
		return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}


//the system calls this function to obtain the cursor to display while the user drags
//the minimized window. simply return the app's icon
HCURSOR CiPeeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//DDX support
void CiPeeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TXT_STATUS, m_txtStatus);
	DDX_Control(pDX, IDC_TAB, m_tabCtrl);
}

//load engine menu - create an instance of the iPee CV engine
void CiPeeDlg::OnMnuLoadEngine()
{
	if (!m_piPeeCV) {
		m_piPeeCV = new CiPeeCV(this);
		AppendStatus(_T("iPee CV engine instance created"));
	}
	else
		AppendStatus(_T("iPee CV engine already up"));
}

//unload engine menu - destroy the instance of the iPee CV engine
void CiPeeDlg::OnMnuUnloadEngine()
{
	if (m_piPeeCV)
	{
		m_piPeeCV->Shutdown();
		delete m_piPeeCV;
		m_piPeeCV = NULL;

		AppendStatus(_T("iPee CV engine instance destroyed"));
	}
	else
		AppendStatus(_T("iPee CV engine already down"));
}

//restart engine menu - like clicking unload and then load
void CiPeeDlg::OnMnuRestartEngine()
{
	OnMnuUnloadEngine();
	OnMnuLoadEngine();
}

//start the engine capturing from cam
void CiPeeDlg::OnMnuStartCAM()
{
	if (m_piPeeCV)
		m_piPeeCV->StartFromCam(0);
	else
		AppendStatus(_T("Cannot start capture, engine is down"));
}

//stop the engine from capturing cam
void CiPeeDlg::OnMnuStopCAM()
{
	if (m_piPeeCV)
		m_piPeeCV->Stop();
	else
		AppendStatus(_T("Cannot stop capture, engine is down"));
}


//iPeeEvt - the iPee engine sends these events for status updates etc.
void CiPeeDlg::OniPeeEvt(WPARAM iMsgCode, LPARAM pMsgData)
{
	iPeeStdMsg* pStdMsg = NULL;

	switch (iMsgCode) {
		case IPEE_EVT_STATUS:		//simple status update, add a line to the text box
			pStdMsg = (iPeeStdMsg*)pMsgData;
			AppendStatus(pStdMsg->lpMsg);

			break;
		
		case IPEE_EVT_ERROR:		//error update, add a formated string to the textbox
			pStdMsg = (iPeeStdMsg*)pMsgData;
			
			//only format a string if the err code is not null, otherwise simple status
			if (pStdMsg->iErrCode)
			{
				CString sMsg(pStdMsg->lpMsg);
				sMsg.AppendFormat(_T(" [Code: %d]\n"), pStdMsg->iErrCode);
				AppendStatus(sMsg);
			}
			else
				AppendStatus(pStdMsg->lpMsg);

			break;
	}

	if (pStdMsg) {
		if (pStdMsg->bNeedCleanup)
			free((void*)pStdMsg->lpMsg);
		delete pStdMsg;
	}
}

//selchange - show and hide the relevant tabs as the user changes selection
void CiPeeDlg::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_pdlgTabs[m_iCurTab]->ShowWindow(SW_HIDE);
	m_pdlgTabs[m_iCurTab = m_tabCtrl.GetCurSel()]->ShowWindow(SW_SHOW);

	*pResult = 0;
}