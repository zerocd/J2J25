#pragma once
#include "afxcmn.h"


// CMiniDlg ��ȭ �����Դϴ�.

class CMiniDlg : public CDialog
{
	DECLARE_DYNAMIC(CMiniDlg)

public:
	CMiniDlg(CString strSource, int nWorkType, CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CMiniDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

	DECLARE_MESSAGE_MAP()
public:
	CProgressCtrl m_pStat;
	virtual BOOL OnInitDialog();
	static UINT myThread(LPVOID lp);
private:
	CString m_strSource;
	int m_nWorkType;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
