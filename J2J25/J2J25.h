// J2J25.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CJ2J25App:
// �� Ŭ������ ������ ���ؼ��� J2J25.cpp�� �����Ͻʽÿ�.
//

class CJ2J25App : public CWinApp
{
public:
	CJ2J25App();

// �������Դϴ�.
	public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CJ2J25App theApp;

#define WM_USER_EDIT_UPDATED (WM_USER + 100)