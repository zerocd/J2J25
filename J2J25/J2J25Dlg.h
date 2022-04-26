// J2J25Dlg.h : 헤더 파일
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
		EncryptSelected,	// 변조하기 선택됨
		DecryptSelected,	// 복원하기 선택됨
		CheckSelected,	// 검사하기 선택됨
		OverwriteSelected,		// 덮어쓰기 선택됨
		SamePathSelected,	// 같은 폴더에 출력하기
		JobStarting,	// 작업 시작
		JobFinished,	// 작업 종료
		TargetAdded,	// 대상폴더 정해짐
	};
}

namespace list_icon
{
	enum Enum
	{
		Normal,	// 아무것도 않은 상태
		Encrypted,	// 변조됨
		Success,	// 복원 작업 성공
		Error,	// 변조/복원 작업 실패
		Unknown,	// 체크 실패
	};
}

// CJ2J25Dlg 대화 상자
class CJ2J25Dlg : public CDialog
{
// 생성입니다.
public:
	CJ2J25Dlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
	enum { IDD = IDD_J2J25_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	// 작업 종류
	UINT m_nWorkType;
	CButton m_bOverwrite;	// 원본 덮어쓰기
	CMyEdit m_eTargetPath;
	CButton m_bOK;
	CButton m_bCancel;
	CButton m_bSamePath;	// 같은 폴더에 출력하기
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedCheck2();
	afx_msg void OnBnClickedOk();
	CString m_sTargetPath;
	BOOL m_bEnc;	// 변조하기?
	static UINT myThread(LPVOID lp);
	BOOL m_bWrite;	// 덮어쓰기?
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	BOOL m_bStandby;
	CProgressCtrl m_pStat;
	afx_msg void OnDropFiles(HDROP hDropInfo);
	CEdit m_ePassword;
	TCHAR m_szPassword[64];
	CButton m_brEnc;	// 라디오 변조하기
	CButton m_brDec;
	CButton m_brCheck;
	void RefreshItems(int nEvent, BOOL bCheck = TRUE);	// GUI 관련 이벤트에 따라서 아이템 변경
	afx_msg void OnBnClickedRadio3();
	ULONGLONG m_llSplitSize;	// 분할할 크기
	BOOL m_bJoin;	// 분할파일 합치기?
	DWORD m_dwShareRight;	// 파일열기시 권한
	int m_nFiles;	// 분할된 파일갯수
	afx_msg void OnBnClickedHelp();
	int m_nMethod;	// 방법번호 -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2
	CButton m_bCleanListSuccess;
	CButton m_bCleanListAll;
	afx_msg void OnBnClickedClearlistsuccess();
	afx_msg void OnBnClickedClearlistall();
	CMyListCtrl m_lstSource;
	afx_msg void OnBnClickedCancel();
private:
	BOOL m_bWorking;	// 작업이 진행중인지?
	BOOL m_bHalt;	// 중지명령이 전달?
	CString m_strTargetInfo;	// 출력 경로에 보여줄 안내물
	BOOL m_bForce;	// 강제 복원
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
