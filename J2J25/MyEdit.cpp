// MyEdit.cpp : 구현 파일입니다.
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



// CMyEdit 메시지 처리기입니다.



void CMyEdit::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
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
		{	// 디렉토리인 경우에만 1개에 한하여 업데이트
			SetWindowText(szFile);
			AfxGetMainWnd()->SendMessage(WM_USER_EDIT_UPDATED, (WPARAM)szFile);
			delete [] szFile;
			break;
		}
		delete [] szFile;
	}

	CEdit::OnDropFiles(hDropInfo);
}
