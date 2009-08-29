#pragma once
#include "afxwin.h"

#define MAX_FILES	32


/////////////////////////////////////////////
// CTabDebug dialog - the tab with controls for debug
//		includes CAM simulation

class CTabDebug : public CDialog
{
	DECLARE_DYNAMIC(CTabDebug)

public:
	//default constructor
	CTabDebug(CWnd* pParent = NULL);

	//dialog Data
	enum { IDD = IDD_TAB_DEBUG };

//-- Fields ----------------
public:
	CListBox m_lstFiles;					//the files list box
	CiPeeDlg* m_piPeeDlg;					//the parent iPee dlg control panel
	CList<CString, CString&> m_lstPaths;	//a list with paths to the files

//-- Event Handlers --------
protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnAdd();
	afx_msg void OnBnClickedBtnRemove();
	afx_msg void OnBnClickedBtnUp();
	afx_msg void OnBnClickedBtnDown();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnBnClickedBtnStop();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//message map implementation
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
};

bool CaptureFileEnumProc(void* pTag, CString& sFile);
