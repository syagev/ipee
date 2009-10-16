#include "stdafx.h"
#include "iPeeGamer.h"
#include "iPeeGamerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////
// CiPeeGamerDlg dialog - the iPee gamer proxy main dialog
//		contains all the of the gamer's logic and is infact the main class

//default constructor
CiPeeGamerDlg::CiPeeGamerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CiPeeGamerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


// -- Event Handlers -------------------------------

//message map implementation
BEGIN_MESSAGE_MAP(CiPeeGamerDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CiPeeGamerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_txtStatus);
}

BOOL CiPeeGamerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	int iRes = 0;

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//find the game window
	m_hWndGame = FindGameWindow();
	
	//startup winsock
	WSADATA wsaData;
	if ((iRes = WSAStartup(MAKEWORD(2,0), &wsaData)) != ERROR_SUCCESS) {
		AfxMessageBox(_T("Failed to start winsock"));
		return TRUE;
	}
	
	//create the listening socket
	m_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, NULL);
	
	//bind the listening socket to all addresses
	sockaddr_in addr;
	addr.sin_port = htons(IPEE_GAMER_PORT);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if ((iRes = bind(m_sock, (sockaddr*)&addr, sizeof(addr))) != ERROR_SUCCESS) {
		AfxMessageBox(_T("Failed to bind listening socket"));
		closesocket(m_sock);
		return TRUE;
	}

	//start listening on the socket
	if ((iRes = listen(m_sock, 5)) != ERROR_SUCCESS) {
		AfxMessageBox(_T("Failed to listen on socket"));
		closesocket(m_sock);
		return TRUE;
	}

	//start the comm thread
	m_bTerminate = false;
	AfxBeginThread(SockThread, this);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//close event - signal thread to exit
void CiPeeGamerDlg::OnClose()
{
	m_bTerminate = true;
	
	CDialog::OnClose();
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CiPeeGamerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CiPeeGamerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//--- Methods ----------------------------------------

//find the flash player's window
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	TCHAR sWndName[64];
	GetWindowText(hwnd, sWndName, 64);

	if (wcsstr(sWndName, _T("Flash Player")) != NULL)
	{
		((CiPeeGamerDlg*)lParam)->m_hWndGame = hwnd;
		return FALSE;
	}
	else 
	{
		((CiPeeGamerDlg*)lParam)->m_hWndGame = NULL;
		return TRUE;
	}
}

//find the game window for sending mouse events
HWND CiPeeGamerDlg::FindGameWindow(void)
{
	EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)this);

	return m_hWndGame;
}

// CiPeeGamerDlg message handlers
UINT SockThread(LPVOID pParam)
{
	CiPeeGamerDlg* pThis = (CiPeeGamerDlg*)pParam;
	int iRes = 0;

	int buf[256];	//recv buffer
	int iLen = 0;	//length of input buffer
	int x, y;		//extracted x,y coordinates
	
	SOCKET sock;	//the comm socket
	fd_set fd;		//fd-array for the listening socket
	fd.fd_array[0] = pThis->m_sock;
	fd.fd_count = 1;
	
	//main loop that run's between connections
	while (!pThis->m_bTerminate) {
		pThis->m_txtStatus.SetWindowText(_T("Awaiting connection"));

		//select to block until an incoming connection request is present
		if ((iRes = select(NULL, &fd, NULL, NULL, NULL)) == SOCKET_ERROR) {
			pThis->m_txtStatus.SetWindowText(_T("Failed to select on listening socket"));
			return -1;
		}
		
		//accept the incoming connection to a new socket
		if ((sock = accept(pThis->m_sock, NULL, NULL)) == INVALID_SOCKET) {
			sock = NULL;
			continue;
		}
		
		pThis->m_txtStatus.SetWindowText(_T("Active"));

		//communication loop
		do {
			//recieve a buffer, if failed we are disconnected
			if ((iRes = recv(sock, (char*)buf + iLen, 1024 - iLen, NULL)) > 0) {
			
				iLen += iRes;

				//process the messages in the buffer
				for (int i = 0; iLen >= IPEE_MSG_SIZE; iLen -= IPEE_MSG_SIZE, i += 2) {
					//extract coordinates and pass to game window
					x = buf[i];
					y = buf[i + 1];
					
					if (pThis->m_hWndGame != NULL)
						SendMessage(pThis->m_hWndGame, WM_MOUSEMOVE, NULL, MAKELPARAM(x, y));
					else
						pThis->FindGameWindow();
				}
			}
		} while (!pThis->m_bTerminate && iRes > 0);
	}
	
	pThis->m_txtStatus.SetWindowText(_T("Terminated"));

	return NULL;
}