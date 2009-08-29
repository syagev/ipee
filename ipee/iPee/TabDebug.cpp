#include "stdafx.h"
#include "iPee.h"
#include "iPeeDlg.h"
#include "TabDebug.h"

IMPLEMENT_DYNAMIC(CTabDebug, CDialog)


/////////////////////////////////////////////
// CTabDebug dialog - the tab with controls for debug
//		includes CAM simulation

//default constructor
CTabDebug::CTabDebug(CWnd* pParent /*=NULL*/)
	: CDialog(CTabDebug::IDD, pParent)
{
	m_piPeeDlg = (CiPeeDlg*)pParent;
}


// -- Event Handlers -------------------------------

//message map implementation
BEGIN_MESSAGE_MAP(CTabDebug, CDialog)
	ON_BN_CLICKED(IDC_BTN_ADD, &CTabDebug::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDC_BTN_REMOVE, &CTabDebug::OnBnClickedBtnRemove)
	ON_BN_CLICKED(IDC_BTN_UP, &CTabDebug::OnBnClickedBtnUp)
	ON_BN_CLICKED(IDC_BTN_DOWN, &CTabDebug::OnBnClickedBtnDown)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CTabDebug::OnBnClickedBtnPlay)
	ON_BN_CLICKED(IDC_BTN_STOP, &CTabDebug::OnBnClickedBtnStop)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//initDialog - called by the framework when dlg is loaded
BOOL CTabDebug::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CStdioFile fFiles(theApp.GetWithPath(_T("AVIfiles.txt")), 
		CFile::modeRead | CFile::typeText);
	
	if (fFiles.m_hFile)
	{
		CString sPath;
		while (fFiles.ReadString(sPath))
		{
			m_lstPaths.AddTail(sPath);
			m_lstFiles.AddString(sPath.Right(sPath.GetLength() - 
				sPath.ReverseFind(_T('\\')) - 1));
		}
		
		fFiles.Close();
	}

	return TRUE;
}

//DDX support
void CTabDebug::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_FILES, m_lstFiles);
}

//add button - pick a file and add it to the list and array
void CTabDebug::OnBnClickedBtnAdd()
{
	CFileDialog dlgFile(TRUE, _T("avi"), _T("*.avi"),	//open-file system dialog
		OFN_FILEMUSTEXIST, _T("Movie Files (*.avi)|*.avi||"));
		
	//open the dialog, exit if canceled
	if (dlgFile.DoModal() == IDOK)
	{
		//add the file to the list
		m_lstPaths.AddTail(dlgFile.GetPathName());
		m_lstFiles.InsertString(-1, dlgFile.GetFileName());
	}
}

//remove button - remove the selected file from list and array
void CTabDebug::OnBnClickedBtnRemove()
{
	int iCurSel = m_lstFiles.GetCurSel();

	if (iCurSel == LB_ERR)
		AfxMessageBox(_T("Select a file to remove"), MB_OK | MB_ICONERROR);
	else
	{
		m_lstPaths.RemoveAt(m_lstPaths.FindIndex(iCurSel));
		m_lstFiles.DeleteString(iCurSel);
	}
}

//up-button - swap between the current and the file above
void CTabDebug::OnBnClickedBtnUp()
{
	int iCurSel = m_lstFiles.GetCurSel();

	//only go up if it is not the first item or no item
	if (iCurSel > 0)
	{
		CString sFile;
		m_lstFiles.GetText(iCurSel, sFile);
		m_lstFiles.InsertString(iCurSel - 1, sFile);
		m_lstFiles.DeleteString(iCurSel + 1);
		m_lstFiles.SetCurSel(iCurSel - 1);

		POSITION pos = m_lstPaths.FindIndex(iCurSel);
		POSITION posOrig = pos;

		m_lstPaths.GetPrev(pos);
		m_lstPaths.InsertBefore(pos, sFile);
		m_lstPaths.RemoveAt(posOrig);
	}
}

//down-button - swap between the current and the file below
void CTabDebug::OnBnClickedBtnDown()
{
	int iCurSel = m_lstFiles.GetCurSel();

	//only go up if it is not the first item or no item
	if (iCurSel < m_lstPaths.GetCount() - 1)
	{
		CString sFile;
		m_lstFiles.GetText(iCurSel, sFile);
		m_lstFiles.InsertString(iCurSel + 2, sFile);
		m_lstFiles.DeleteString(iCurSel);
		m_lstFiles.SetCurSel(iCurSel + 1);
		
		POSITION pos = m_lstPaths.FindIndex(iCurSel);
		POSITION posOrig = pos;
		
		m_lstPaths.GetNext(pos);
		m_lstPaths.InsertAfter(pos, sFile);
		m_lstPaths.RemoveAt(posOrig);
	}
}

//play button - start the engine 
void CTabDebug::OnBnClickedBtnPlay()
{
	//check that the user selected a file to start from
	if (m_lstPaths.GetCount() > 0 && m_lstFiles.GetCurSel() != LB_ERR) {
		if (m_piPeeDlg->m_piPeeCV)
			m_piPeeDlg->m_piPeeCV->StartFromFiles(&CaptureFileEnumProc, this);
		else
			m_piPeeDlg->AppendStatus(_T("Cannot play. iPeeCV engine down"));
	}
	else
		AfxMessageBox(_T("Select a file to start from"), MB_ICONERROR | MB_OK);
}

//file enum proc - called by the iPee CV engine to enumerate the next file
//to capture from. advance along the list of files
bool CaptureFileEnumProc(void* pTag, CString& sFile)
{
	CTabDebug* pThis = (CTabDebug*)pTag;
	int iCurSel = pThis->m_lstFiles.GetCurSel();

	//check if there is a current selection on the list
	if (iCurSel != LB_ERR)
	{
		//get the path of the currently selected file
		sFile = pThis->m_lstPaths.GetAt(pThis->m_lstPaths.FindIndex(iCurSel));

		//set the selection to the next item or to none if this was the last
		pThis->m_lstFiles.SetCurSel(
			iCurSel < pThis->m_lstFiles.GetCount() - 1 ? iCurSel + 1 : -1);

		return true;
	}
	else
		return false;	//the end of the enumeration
}

//stop button
void CTabDebug::OnBnClickedBtnStop()
{
	m_piPeeDlg->m_piPeeCV->Stop();
}

//window closing - save the file list to the data file
void CTabDebug::OnDestroy()
{
	CDialog::OnDestroy();
	
	CStdioFile fFiles(theApp.GetWithPath(_T("AVIfiles.txt")), CFile::modeCreate | 
		CFile::modeWrite | CFile::typeText);
	
	POSITION pos = m_lstPaths.GetHeadPosition();
	for (int i = 0; i < m_lstPaths.GetCount(); i++)
	{
	   fFiles.WriteString(m_lstPaths.GetNext(pos));
	   fFiles.WriteString(_T("\n"));
	}
	
	fFiles.Close();
}
