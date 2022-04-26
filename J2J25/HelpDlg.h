#pragma once
#include "afxwin.h"


// CHelpDlg 대화 상자입니다.

class CHelpDlg : public CDialog
{
	DECLARE_DYNAMIC(CHelpDlg)

public:
	CHelpDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CHelpDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CString m_sText;	// 도움말 내용
	CEdit m_eHelp;
};
