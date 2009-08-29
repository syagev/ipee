#include "stdafx.h"
#include "iPee.h"
#include "iPeeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////
// CiPeeApp - the main application class. only
//		one instance of the app is created in 'theApp'


//the one and only CiPeeApp object
CiPeeApp theApp;

//default construction
CiPeeApp::CiPeeApp()
{
}


// -- Event Handlers ---------------

BEGIN_MESSAGE_MAP(CiPeeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

//application initialization
BOOL CiPeeApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	AfxEnableControlContainer();

	//set the module's running directory
	GetModuleFileName(m_hInstance, m_sPath.GetBuffer(MAX_PATH), MAX_PATH);
	m_sPath.ReleaseBuffer();
	m_sPath = m_sPath.Left(m_sPath.ReverseFind(_T('\\')) + 1);
	
	//open the main dialog - the control panel
	CiPeeDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


//-- Methods ----------------
CString CiPeeApp::GetWithPath(CString sFile)
{
	return m_sPath + sFile;
}

CString CiPeeApp::GetWithPath(LPCTSTR sFile)
{
	CString sTemp = m_sPath;
	sTemp.Append(sFile);
	return sTemp;
}