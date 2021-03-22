
// Honyaku_No_Hojo.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CHonyaku_No_HojoApp:
// See Honyaku_No_Hojo.cpp for the implementation of this class
//

class CHonyaku_No_HojoApp : public CWinAppEx
{
public:
	CHonyaku_No_HojoApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CHonyaku_No_HojoApp theApp;