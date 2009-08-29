#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


////////////////////////////////////////////
//	CiPeeApp : main MFC app class, see iPee.cpp for implementation

class CiPeeApp : public CWinApp
{
public:
	//default constructor
	CiPeeApp();

// -- Fields ------------
	CString m_sPath;			//the path to the application's exe

// -- Event Handlers -----------
	virtual BOOL InitInstance();

// -- Methods ------------------
	CString GetWithPath(CString sFile);
	CString GetWithPath(LPCTSTR sFile);

	//message-map implementation
	DECLARE_MESSAGE_MAP()
};

//the application instance
extern CiPeeApp theApp;