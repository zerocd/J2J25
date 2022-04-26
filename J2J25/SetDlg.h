#pragma once
#include "afxwin.h"


// CSetDlg 대화 상자입니다.

class CSetDlg : public CDialog
{
	DECLARE_DYNAMIC(CSetDlg)

public:
	CSetDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CSetDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_DIALOG_SET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

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
