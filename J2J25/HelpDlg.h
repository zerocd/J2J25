#pragma once
#include "afxwin.h"


// CHelpDlg ��ȭ �����Դϴ�.

class CHelpDlg : public CDialog
{
	DECLARE_DYNAMIC(CHelpDlg)

public:
	CHelpDlg(CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CHelpDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CString m_sText;	// ���� ����
	CEdit m_eHelp;
};
