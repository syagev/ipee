#pragma once
#include "afxwin.h"


/////////////////////////////////////////////
// CiPeeGamerDlg dialog - the iPee gamer proxy main dialog
//		contains all the of the gamer's logic and is infact the main class

//-- define some gamer constants
#define IPEE_GAMER_PORT	10101
#define IPEE_MSG_SIZE	8

class CiPeeGamerDlg : public CDialog
{
public:
	//default constructor
	CiPeeGamerDlg(CWnd* pParent = NULL);

	//dialog Data
	enum { IDD = IDD_IPEEGAMER_DIALOG };

//-- Fields ----------------
protected:
	HICON m_hIcon;
public:
	SOCKET m_sock;
	CEdit m_txtStatus;
	HWND m_hWndGame;
	bool m_bTerminate;
	

//-- Methods ---------------
public:
	HWND FindGameWindow(void);

//-- Event Handlers --------
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
};

UINT SockThread(LPVOID pParam);