#pragma once
#include "afxwin.h"


// CSetDlg ��ȭ �����Դϴ�.

class CSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CSetDlg)

public:
	CSetDlg(CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CSetDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_DIALOG_SET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_eExt;
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	TCHAR* GetExt();
	void AccessRegistry(TCHAR* szExt, BOOL bLoad);
private:
	TCHAR m_szExt[MAX_PATH];
	BOOL IsValidPath(TCHAR* szExt);
};
