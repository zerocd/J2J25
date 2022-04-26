// J2J25Dlg.h : ��� ����
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "MyEdit.h"
#include "Crc32.h"
#include "mylistctrl.h"

namespace gui_event
{
	enum Enum
	{
		EncryptSelected,	// �����ϱ� ���õ�
		DecryptSelected,	// �����ϱ� ���õ�
		CheckSelected,	// �˻��ϱ� ���õ�
		OverwriteSelected,		// ����� ���õ�
		SamePathSelected,	// ���� ������ ����ϱ�
		JobStarting,	// �۾� ����
		JobFinished,	// �۾� ����
		TargetAdded,	// ������� ������
	};
}

namespace list_icon
{
	enum Enum
	{
		Normal,	// �ƹ��͵� ���� ����
		Encrypted,	// ������
		Success,	// ���� �۾� ����
		Error,	// ����/���� �۾� ����
		Unknown,	// üũ ����
	};
}

// CJ2J25Dlg ��ȭ ����
class CJ2J25Dlg : public CDialog
{
// �����Դϴ�.
public:
	CJ2J25Dlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_J2J25_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	// �۾� ����
	UINT m_nWorkType;
	CButton m_bOverwrite;	// ���� �����
	CMyEdit m_eTargetPath;
	CButton m_bOK;
	CButton m_bCancel;
	CButton m_bSamePath;	// ���� ������ ����ϱ�
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedOk();
	CString m_sTargetPath;
	BOOL m_bEnc;	// �����ϱ�?
	static UINT myThread(LPVOID lp);
	BOOL m_bWrite;	// �����?
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	BOOL m_bStandby;
	CProgressCtrl m_pStat;
	afx_msg void OnDropFiles(HDROP hDropInfo);
	CEdit m_ePassword;
	TCHAR m_szPassword[64];
	CButton m_brEnc;	// ���� �����ϱ�
	CButton m_brDec;
	CButton m_brCheck;
	void RefreshItems(int nEvent, BOOL bCheck = TRUE);	// GUI ���� �̺�Ʈ�� ���� ������ ����
	afx_msg void OnBnClickedRadio3();
	ULONGLONG m_llSplitSize;	// ������ ũ��
	BOOL m_bJoin;	// �������� ��ġ��?
	DWORD m_dwShareRight;	// ���Ͽ���� ����
	int m_nFiles;	// ���ҵ� ���ϰ���
	afx_msg void OnBnClickedHelp();
	int m_nMethod;	// �����ȣ -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2
	CButton m_bCleanListSuccess;
	CButton m_bCleanListAll;
	afx_msg void OnBnClickedClearlistsuccess();
	afx_msg void OnBnClickedClearlistall();
	CMyListCtrl m_lstSource;
	afx_msg void OnBnClickedCancel();
private:
	BOOL m_bWorking;	// �۾��� ����������?
	BOOL m_bHalt;	// ��������� ����?
	CString m_strTargetInfo;	// ��� ��ο� ������ �ȳ���
	BOOL m_bForce;	// ���� ����
	TCHAR m_szExt[MAX_PATH];
public:
	afx_msg void OnNMRclickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuRemove();
	afx_msg LRESULT OnMessageEditUpdated(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnMenuForce();
	CButton m_bAddFile;
	CButton m_bAddPath;
	CButton m_bSet;
	afx_msg void OnBnClickedButtonAddTarget();
	afx_msg void OnBnClickedButtonPath();
	afx_msg void OnBnClickedButtonSet();
};
