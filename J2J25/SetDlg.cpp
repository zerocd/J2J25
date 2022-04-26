// SetDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "J2J25.h"
#include "SetDlg.h"


#define J2J_REG_PATH _T("Software\\J2J25")
#define J2J_EXT _T("Extension")

// CSetDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CSetDlg, CDialog)

CSetDlg::CSetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSetDlg::IDD, pParent)
{

}

CSetDlg::~CSetDlg()
{
}

void CSetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_eExt);
}


BEGIN_MESSAGE_MAP(CSetDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSetDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSetDlg 메시지 처리기입니다.

void CSetDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_eExt.GetWindowTextW(m_szExt, MAX_PATH);
	if(IsValidPath(m_szExt))
	{
		AccessRegistry(m_szExt, FALSE);
		OnOK();
	}
}

BOOL CSetDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	m_eExt.SetLimitText(10);
	m_szExt[0] = _T('\0');
	AccessRegistry(m_szExt, TRUE);
	m_eExt.SetWindowTextW(m_szExt);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

/*!
 * szExt는 MAX_PATH의 크기를 가져야 한다.
*/
void CSetDlg::AccessRegistry(TCHAR *szExt, BOOL bLoad)
{
	HKEY hK;
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_CURRENT_USER, J2J_REG_PATH, 0, KEY_ALL_ACCESS, &hK) != ERROR_SUCCESS)
	{
		if(RegCreateKey(HKEY_CURRENT_USER, J2J_REG_PATH, &hK) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("키를 생성하지 못하였습니다."));
			return;
		}
	}

	if(bLoad)
	{
		dwSize = MAX_PATH * sizeof(TCHAR);
		if(RegQueryValueEx(hK, J2J_EXT, 0, NULL, (LPBYTE)szExt, &dwSize) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("설정정보를 불러오지 못하였습니다."));
			_tcscpy_s(szExt, MAX_PATH, _T("J2J"));
		}
		else
		{
			if(!IsValidPath(szExt))
			{
				_tcscpy_s(szExt, MAX_PATH, _T("J2J"));
			}
		}
	}
	else
	{
		dwSize = _tcslen(szExt) * 2 + 2;	// 널문자 포함 byte수
		if(RegSetValueEx(hK, J2J_EXT, 0, REG_SZ, (LPBYTE)szExt, dwSize) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("설정정보를 저장하지 못하였습니다."));
		}
	}
	RegCloseKey(hK);
}

BOOL CSetDlg::IsValidPath(TCHAR *szExt)
{
	size_t nLen = _tcslen(szExt);
	if(nLen > 10)
	{
		AfxMessageBox(_T("확장자가 너무 깁니다.(10자 초과)"));
		return FALSE;
	}
	for(size_t i = 0; i < nLen; i++)
	{
		switch(szExt[i])
		{
		case _T('\\'):
		case _T('/'):
		case _T(':'):
		case _T('*'):
		case _T('?'):
		case _T('"'):
		case _T('<'):
		case _T('>'):
		case _T('|'):
			AfxMessageBox(_T("확장자에 사용할 수 없는 문자가 있습니다."));
			return FALSE;
		}
	}
	return TRUE;
}

TCHAR* CSetDlg::GetExt()
{
	return m_szExt;
}