#pragma once
#include "afxcmn.h"


// CMiniDlg 대화 상자입니다.

class CMiniDlg : public CDialog
{
	DECLARE_DYNAMIC(CMiniDlg)

public:
	CMiniDlg(CString strSource, int nWorkType, CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CMiniDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_DIALOG2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

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
