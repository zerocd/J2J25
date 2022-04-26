// SetDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "J2J25.h"
#include "SetDlg.h"


#define J2J_REG_PATH _T("Software\\J2J25")
#define J2J_EXT _T("Extension")

// CSetDlg ��ȭ �����Դϴ�.

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


// CSetDlg �޽��� ó�����Դϴ�.

void CSetDlg::OnBnClickedOk()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
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

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	m_eExt.SetLimitText(10);
	m_szExt[0] = _T('\0');
	AccessRegistry(m_szExt, TRUE);
	m_eExt.SetWindowTextW(m_szExt);

	return TRUE;  // return TRUE unless you set the focus to a control
	// ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}

/*!
 * szExt�� MAX_PATH�� ũ�⸦ ������ �Ѵ�.
*/
void CSetDlg::AccessRegistry(TCHAR *szExt, BOOL bLoad)
{
	HKEY hK;
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_CURRENT_USER, J2J_REG_PATH, 0, KEY_ALL_ACCESS, &hK) != ERROR_SUCCESS)
	{
		if(RegCreateKey(HKEY_CURRENT_USER, J2J_REG_PATH, &hK) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("Ű�� �������� ���Ͽ����ϴ�."));
			return;
		}
	}

	if(bLoad)
	{
		dwSize = MAX_PATH * sizeof(TCHAR);
		if(RegQueryValueEx(hK, J2J_EXT, 0, NULL, (LPBYTE)szExt, &dwSize) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("���������� �ҷ����� ���Ͽ����ϴ�."));
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
		dwSize = _tcslen(szExt) * 2 + 2;	// �ι��� ���� byte��
		if(RegSetValueEx(hK, J2J_EXT, 0, REG_SZ, (LPBYTE)szExt, dwSize) != ERROR_SUCCESS)
		{
			AfxMessageBox(_T("���������� �������� ���Ͽ����ϴ�."));
		}
	}
	RegCloseKey(hK);
}

BOOL CSetDlg::IsValidPath(TCHAR *szExt)
{
	size_t nLen = _tcslen(szExt);
	if(nLen > 10)
	{
		AfxMessageBox(_T("Ȯ���ڰ� �ʹ� ��ϴ�.(10�� �ʰ�)"));
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
			AfxMessageBox(_T("Ȯ���ڿ� ����� �� ���� ���ڰ� �ֽ��ϴ�."));
			return FALSE;
		}
	}
	return TRUE;
}

TCHAR* CSetDlg::GetExt()
{
	return m_szExt;
}