// MyEdit.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "J2J25.h"
#include "MyEdit.h"


// CMyEdit

IMPLEMENT_DYNAMIC(CMyEdit, CEdit)

CMyEdit::CMyEdit()
{

}

CMyEdit::~CMyEdit()
{
}


BEGIN_MESSAGE_MAP(CMyEdit, CEdit)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()



// CMyEdit �޽��� ó�����Դϴ�.



void CMyEdit::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	INT_PTR qRet = -1;
	UINT nFile = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	CString filePathName;
	for(UINT i = 0; i < nFile; i++)
	{
		UINT nLength = DragQueryFile(hDropInfo, i, NULL, 0);
		TCHAR* szFile = new TCHAR[nLength + 2];
		DragQueryFile(hDropInfo, i, szFile, nLength + 2);
		DWORD dwAttr = GetFileAttributes(szFile);
		if((dwAttr != INVALID_FILE_ATTRIBUTES) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
		{	// ���丮�� ��쿡�� 1���� ���Ͽ� ������Ʈ
			SetWindowText(szFile);
			AfxGetMainWnd()->SendMessage(WM_USER_EDIT_UPDATED, (WPARAM)szFile);
			delete [] szFile;
			break;
		}
		delete [] szFile;
	}

	CEdit::OnDropFiles(hDropInfo);
}
