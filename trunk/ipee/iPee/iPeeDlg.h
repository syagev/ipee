#pragma once
#include "afxwin.h"
#include "iPeeCV.h"
#include "afxcmn.h"


/////////////////////////////////////////////
// CiPeeDlg dialog - the main application's dialog, makes up
//		for the iPee's system control panel

class CiPeeDlg : public CDialog
{
public:
	//default constructor
	CiPeeDlg(CWnd* pParent = NULL);

	//dialog Data
	enum { IDD = IDD_IPEE_DIALOG };

//-- Fields ----------------
protected:
	HICON m_hIcon;			//the app's icon (loaded from resource)
	CBrush* m_pBgBrush;		//background brush for UI coloring
	CEdit m_txtStatus;		//status textbox
	CTabCtrl m_tabCtrl;		//main tab-control
	CDialog* m_pdlgTabs[4];	//the tabs in the main dlg
	int m_iCurTab;			//the currently selected tab
public:
	CiPeeCV* m_piPeeCV;		//the iPee CV engine instance

//-- Methods ---------------
public:
	void AppendStatus(LPCTSTR lpStatus);

//-- Event Handlers --------
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg HCURSOR OnQueryDragIcon();
public:
	afx_msg void OnMnuLoadEngine();
	afx_msg void OnMnuUnloadEngine();
	afx_msg void OnMnuRestartEngine();
	afx_msg void OnMnuStartCAM();
	afx_msg void OniPeeEvt(WPARAM iMsgCode, LPARAM pMsgData);
	afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
	
	//message map implementation
	DECLARE_MESSAGE_MAP()
	afx_msg void OnMnuStopCAM();
};