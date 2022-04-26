// J2J25Dlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "J2J25.h"
#include "J2J25Dlg.h"
//#include "Crc32.h"
#include "HelpDlg.h"
#include "EncDec.h"
#include "Shlobj.h"
#include "SetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CJ2J25Dlg 대화 상자




CJ2J25Dlg::CJ2J25Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJ2J25Dlg::IDD, pParent)
	, m_nWorkType(0)
	, m_sTargetPath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strTargetInfo = _T("출력할 폴더를 드랙 & 드롭하세요");
}

void CJ2J25Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, (int&)m_nWorkType);
	DDX_Control(pDX, IDC_CHECK1, m_bOverwrite);
	DDX_Control(pDX, IDC_EDIT2, m_eTargetPath);
	DDX_Control(pDX, IDOK, m_bOK);
	DDX_Control(pDX, IDCANCEL, m_bCancel);
	DDX_Control(pDX, IDC_CHECK2, m_bSamePath);
	DDX_Text(pDX, IDC_EDIT2, m_sTargetPath);
	DDX_Control(pDX, IDC_PROGRESS1, m_pStat);
	DDX_Control(pDX, IDC_EDIT3, m_ePassword);
	DDX_Control(pDX, IDC_RADIO1, m_brEnc);
	DDX_Control(pDX, IDC_RADIO2, m_brDec);
	DDX_Control(pDX, IDC_RADIO3, m_brCheck);
	DDX_Control(pDX, ID_CLEARLISTSUCCESS, m_bCleanListSuccess);
	DDX_Control(pDX, ID_CLEARLISTALL, m_bCleanListAll);
	DDX_Control(pDX, IDC_LIST1, m_lstSource);
	DDX_Control(pDX, IDC_BUTTON_ADD_TARGET, m_bAddFile);
	DDX_Control(pDX, IDC_BUTTON_PATH, m_bAddPath);
	DDX_Control(pDX, IDC_BUTTON_SET, m_bSet);
}

BEGIN_MESSAGE_MAP(CJ2J25Dlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK1, &CJ2J25Dlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CJ2J25Dlg::OnBnClickedCheck2)
	ON_BN_CLICKED(IDOK, &CJ2J25Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO1, &CJ2J25Dlg::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, &CJ2J25Dlg::OnBnClickedRadio2)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_RADIO3, &CJ2J25Dlg::OnBnClickedRadio3)
	ON_BN_CLICKED(ID_HELP, &CJ2J25Dlg::OnBnClickedHelp)
	ON_BN_CLICKED(ID_CLEARLISTSUCCESS, &CJ2J25Dlg::OnBnClickedClearlistsuccess)
	ON_BN_CLICKED(ID_CLEARLISTALL, &CJ2J25Dlg::OnBnClickedClearlistall)
	ON_BN_CLICKED(IDCANCEL, &CJ2J25Dlg::OnBnClickedCancel)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CJ2J25Dlg::OnNMRclickList1)
	ON_COMMAND(ID_MENU_REMOVE, &CJ2J25Dlg::OnMenuRemove)
	ON_MESSAGE(WM_USER_EDIT_UPDATED, &CJ2J25Dlg::OnMessageEditUpdated)
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_MENU_FORCE, &CJ2J25Dlg::OnMenuForce)
	ON_BN_CLICKED(IDC_BUTTON_ADD_TARGET, &CJ2J25Dlg::OnBnClickedButtonAddTarget)
	ON_BN_CLICKED(IDC_BUTTON_PATH, &CJ2J25Dlg::OnBnClickedButtonPath)
	ON_BN_CLICKED(IDC_BUTTON_SET, &CJ2J25Dlg::OnBnClickedButtonSet)
END_MESSAGE_MAP()


// CJ2J25Dlg 메시지 처리기

BOOL CJ2J25Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// 형식 선언
	typedef BOOL (WINAPI *CHANGEWINDOWMESSAGEFILTEREX)(HWND hWNd, UINT message, DWORD action, LPVOID lp);

	// 낮은 권한으로부터도 메시지를 수신하기 위해
	CHANGEWINDOWMESSAGEFILTEREX ChangeWindowMessageFilterEx = NULL;
	HINSTANCE hM;
	hM = LoadLibrary(_T("USER32.DLL"));
	if(hM)
	{
		ChangeWindowMessageFilterEx = (CHANGEWINDOWMESSAGEFILTEREX)GetProcAddress(hM, "ChangeWindowMessageFilterEx");
		if(ChangeWindowMessageFilterEx)	// Win7 이하에서는 해당 함수가 없으므로 패스
		{
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), WM_COPYDATA, 1, NULL);
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), WM_DROPFILES, 1, NULL);
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), 0x0049, 1, NULL);
		}
		FreeLibrary(hM);
	}

	m_pStat.SetRange(1, 100);
	m_ePassword.SetLimitText(32);

	m_lstSource.InsertColumn(0, _T("파일명"), LVCFMT_LEFT);
	m_lstSource.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_lstSource.InsertColumn(1, _T(""), LVCFMT_LEFT, 0);	// hidden : 풍선도움말용
	m_lstSource.InsertColumn(2, _T(""), LVCFMT_LEFT, 0);	// hidden2 : 작업결과용
	m_lstSource.SetExtendedStyle(m_lstSource.GetExtendedStyle()
		| LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	CImageList iSmall;
	iSmall.Create(16, 16, ILC_COLOR4 | ILC_MASK, 1, 1);
	iSmall.Add(AfxGetApp()->LoadIcon(IDI_ICON1));
	iSmall.Add(AfxGetApp()->LoadIcon(IDI_ICON2));
	iSmall.Add(AfxGetApp()->LoadIcon(IDI_ICON3));
	iSmall.Add(AfxGetApp()->LoadIcon(IDI_ICON4));
	iSmall.Add(AfxGetApp()->LoadIcon(IDI_ICON5));
	m_lstSource.SetImageList(&iSmall, LVSIL_SMALL);
	iSmall.Detach();

	// 초기화
	m_bWorking = FALSE;
	m_bHalt = FALSE;
	m_bForce = FALSE;

	SetDlgItemText(IDC_STATIC_COUNT, _T(""));
	m_eTargetPath.SetWindowText(m_strTargetInfo);
	m_bOverwrite.SetCheck(1);

	TCHAR szExt[MAX_PATH] = {0,};
	CSetDlg dlg;
	dlg.AccessRegistry(szExt, TRUE);
	_stprintf_s(m_szExt, MAX_PATH, _T(".%s"), szExt);

	// todo: 레지스트리로 부터 설정 값을 읽어오도록 할까?
#if (JEJES_READONLY == 1)
	m_brEnc.EnableWindow(0);
	m_brDec.SetCheck(TRUE);
#else	// 기본방법 2에 따른 비활성화
#endif

#if (JEJES_READONLY == 1)
	RefreshItems(gui_event::DecryptSelected);
#else
	RefreshItems(gui_event::EncryptSelected);
#endif
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CJ2J25Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CJ2J25Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CJ2J25Dlg::OnBnClickedCheck1()
{	// 원본 덮어쓰기
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	RefreshItems(gui_event::OverwriteSelected, m_bOverwrite.GetCheck());
}

void CJ2J25Dlg::OnBnClickedCheck2()
{	// 같은 폴더에 출력하기
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	RefreshItems(gui_event::SamePathSelected, m_bSamePath.GetCheck());
}

void CJ2J25Dlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData();

	if(m_lstSource.GetItemCount() < 1)
	{
		AfxMessageBox(_T("작업파일을 드랙 & 드롭으로 리스트에 추가해 주세요."));
		return;
	}

	m_bWrite = m_bOverwrite.GetCheck();
	BOOL bSamePath = m_bSamePath.GetCheck();
	if((m_sTargetPath.GetLength() == 0) || (m_sTargetPath == m_strTargetInfo))
	{
		if((m_nWorkType != 2) && !m_bWrite && !bSamePath)
		{
			AfxMessageBox(_T("출력 경로를 설정하거나 출력옵션을 선택해주세요."));
			return;
		}
	}

	// 비밀번호 확인
	m_ePassword.GetWindowText(m_szPassword, 64);

	m_bWorking = TRUE;
	AfxBeginThread(myThread, this, THREAD_PRIORITY_LOWEST);

//	OnOK();
}

UINT CJ2J25Dlg::myThread(LPVOID lp)
{
	CJ2J25Dlg *pst = (CJ2J25Dlg *)lp;

	// 초기 설정
	int nWorkType = pst->m_nWorkType;
//	pst->m_bWorking = TRUE; OnBnClickOK에서 설정해줄 것...
	pst->RefreshItems(gui_event::JobStarting);
	ENCDEC_GLOBAL *global = new ENCDEC_GLOBAL;
	::ZeroMemory(global, sizeof(ENCDEC_GLOBAL));

	global->bSamePath = pst->m_bSamePath.GetCheck();
	global->bWrite = pst->m_bWrite;
	global->nMethod = 3;
	global->nWorkType = pst->m_nWorkType;
	_tcscpy_s(global->szExt, MAX_PATH, pst->m_szExt);
	if(nWorkType == 2)	// 검사이면 덮어쓰기는 FALSE, 같은 폴더는 TRUE
	{
		global->bWrite = FALSE;
		global->bSamePath = TRUE;
	}
	_tcscpy_s(global->szPassword, _countof(global->szPassword), pst->m_szPassword);
	if(pst->m_sTargetPath == pst->m_strTargetInfo)
		global->szTargetFolder[0] = _T('\0');
	else
		_tcscpy_s(global->szTargetFolder, _countof(global->szTargetFolder), pst->m_sTargetPath);
	// 복원하기, 검사하기면 일단 분할파일 합치는 것으로 설정
	if(nWorkType != 0)
		global->bJoin = TRUE;
	else
		global->bJoin = FALSE;
//	global->pStat = &pst->m_pStat;
	ENCDEC_CTX *ctx = new ENCDEC_CTX;
	::ZeroMemory(ctx, sizeof(ENCDEC_CTX));

	CString strTarget, strCount;
	int nItem;
	CString strBuf;
	int nCount, nCount2;
	if(pst->m_bForce)
	{
		global->bForce = TRUE;	// 강제 복원
		// 단일 파일만 대상으로 한다.
		nCount2 = 0;
		nCount = pst->m_lstSource.GetSelectedCount();
		int nIndex = -1;
		if((nIndex = pst->m_lstSource.GetNextItem(-1, LVNI_SELECTED)) = -1)
		{
			POSITION pos = pst->m_lstSource.GetFirstSelectedItemPosition();
			for(int i = 0; i < nCount; i++)
			{
				nItem = pst->m_lstSource.GetNextSelectedItem(pos);
				strBuf = pst->m_lstSource.GetItemText(nItem, 2);
				if(strBuf == _T("-2"))
					nCount2++;
			}
		}
		if(nCount2 == 0)
			nCount = 0;
	}
	else
	{
		global->bForce = FALSE;
		nCount = pst->m_lstSource.GetItemCount();
	}
	for(int i = 0; i < nCount; i++)
	{
		if(pst->m_bHalt)
			break;

		pst->m_pStat.SetPos(0);
		if(pst->m_bForce)
			strCount.Format(_T("%d / %d"), i + 1, nCount2);
		else
			strCount.Format(_T("%d / %d"), i + 1, nCount);
		pst->SetDlgItemText(IDC_STATIC_COUNT, strCount);

		if(pst->m_bForce)
		{
			// 위에서 조사된 nItem에 대해서 작업한다.
		}
		else
		{
			nItem = i;
		}
		// 매번 해주어야하는 초기화들...
		global->nWorkType = nWorkType;
		::ZeroMemory(ctx, sizeof(ENCDEC_CTX));
		ctx->Global = global;
		strTarget = pst->m_lstSource.GetItemText(nItem, 0);
		_tcscpy_s(ctx->File.szFileName, MAX_PATH, strTarget);

		CEncDec *myT = new CEncDec(ctx, &pst->m_pStat);
		BOOL bR = myT->SimpleDo(ctx);
		// 리스트에 결과 저장
		if(!bR)
		{
			pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);
			if((nWorkType == 2) && (_tcscmp(ctx->szError, _T("변조된 파일이 아니거나 손상된 파일입니다.")) == 0))
			{
				// 검사인데 변조파일인지 확인할 수 없을 때
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Unknown);
				pst->m_lstSource.ChangeItem(nItem, 2, _T("-1"));
			}
			else
			{
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Error);
				pst->m_lstSource.ChangeItem(nItem, 2, _T("-2"));
			}
		}
		else
		{
			if(nWorkType == 2)	// 검사이면
			{
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Encrypted);
				pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);
			}
			else if(pst->m_bWrite)	// 덮어쓰기면 해당 파일 정보 수정
			{
				if(ctx->Core.nMethod == 3)	// 방법3이면 확장자 처리
				{
					TCHAR szNew[MAX_PATH];
					_stprintf_s(szNew, MAX_PATH, _T("%s%s"), ctx->File.szNewName, ctx->File.szNewExt);
					CString strName = pst->m_lstSource.GetItemText(nItem, 0);
					if(strName != szNew)	// 변경되었으면
					{
						pst->m_lstSource.ChangeItem(nItem, 0, szNew);
					}
				}
				if((nWorkType == 0) || (nWorkType == 2))	// 변조 또는 검사
					pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Encrypted);
				else	// 복원
					pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Success);
				pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);	// 성공한 경우는 파일 정보를 저장한다.
			}
			else	// 덮어쓰기가 아니고 검사가 아니면
			{
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Success);
				CString strResult = _T("작업 성공. ") + pst->m_lstSource.GetItemText(nItem, 1);
				pst->m_lstSource.ChangeItem(nItem, 1, (LPTSTR)strResult.GetString());
			}
			pst->m_lstSource.ChangeItem(nItem, 2, _T("1"));
		}
		delete myT;
	}

	pst->m_pStat.SetPos(0);
	delete ctx;
	delete global;
	pst->m_bWorking = FALSE;
	pst->m_bHalt = FALSE;
	pst->m_bForce = FALSE;
	pst->RefreshItems(gui_event::JobFinished);
	return 1;
}

void CJ2J25Dlg::OnBnClickedRadio1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	RefreshItems(gui_event::EncryptSelected);
}

void CJ2J25Dlg::OnBnClickedRadio2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	RefreshItems(gui_event::DecryptSelected);
}

void CJ2J25Dlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnDropFiles(hDropInfo);
}

void CJ2J25Dlg::RefreshItems(int nEvent, BOOL bCheck)
{
	switch(nEvent)
	{
	case gui_event::EncryptSelected:	// 변조하기
	case gui_event::DecryptSelected:	// 복원하기
	case gui_event::CheckSelected:	// 검사하기
		// 별 할 일이 없네...
		break;
	case gui_event::OverwriteSelected:	// 덮어쓰기 선택됨
	case gui_event::SamePathSelected:	// 같은 폴더에 출력하기
		m_eTargetPath.SetWindowText(m_strTargetInfo);
		break;
	case gui_event::JobStarting:	// 작업 시작
		m_ePassword.EnableWindow(0);
		m_bOK.EnableWindow(0);
		m_bCleanListSuccess.EnableWindow(0);
		m_bCleanListAll.EnableWindow(0);
		m_bOverwrite.EnableWindow(0);
		m_bSamePath.EnableWindow(0);
		m_bCancel.SetWindowText(_T("중지"));
		m_bAddFile.EnableWindow(0);
		m_bAddPath.EnableWindow(0);
		m_bSet.EnableWindow(0);
		break;
	case gui_event::JobFinished:	// 작업 종료
		m_ePassword.EnableWindow();
		m_bOK.EnableWindow();
		m_bCleanListSuccess.EnableWindow();
		m_bCleanListAll.EnableWindow();
		m_bOverwrite.EnableWindow();
		m_bSamePath.EnableWindow();
		m_bCancel.SetWindowText(_T("닫기"));
		m_bAddFile.EnableWindow();
		m_bAddPath.EnableWindow();
		m_bSet.EnableWindow();
		break;
	case gui_event::TargetAdded:	// 대상폴더 선택됨
		m_bOverwrite.SetCheck(0);
		m_bSamePath.SetCheck(0);
		UpdateData();
		break;
	}
}

void CJ2J25Dlg::OnBnClickedRadio3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	RefreshItems(gui_event::CheckSelected);
}

void CJ2J25Dlg::OnBnClickedHelp()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CHelpDlg dlg;
	dlg.DoModal();
}

void CJ2J25Dlg::OnBnClickedClearlistsuccess()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int nCount = m_lstSource.GetItemCount();
	CString strResult;
	for(int i = nCount -1; i >= 0; i--)
	{
		strResult = m_lstSource.GetItemText(i, 2);
		if(strResult == _T("1"))
			m_lstSource.DeleteItem(i);
	}
}

void CJ2J25Dlg::OnBnClickedClearlistall()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_lstSource.DeleteAllItems();
}

void CJ2J25Dlg::OnBnClickedCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if(m_bWorking)
	{
		m_bHalt = TRUE;
	}
	else
	{
		OnCancel();
	}
}

void CJ2J25Dlg::OnNMRclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 작업중이면 우측마우스 안됨
	if(m_bWorking)
		return;
	CMenu menu, *pSubMenu;
	CPoint pos;
	NOTIFYICONDATA data;
	data.uID = IDR_MENU1;
	data.hWnd = m_hWnd;
	menu.LoadMenu(data.uID);
	pSubMenu = menu.GetSubMenu(0);
	CString strBuf;
	BOOL bEnable = FALSE;
	int nItem;
	int nIndex = -1;
	if((nIndex = m_lstSource.GetNextItem(-1, LVNI_SELECTED)) != -1)
	{
		POSITION pos = m_lstSource.GetFirstSelectedItemPosition();
		int nCount = m_lstSource.GetSelectedCount();
		for(int i = 0; i < nCount; i++)
		{
			nItem = m_lstSource.GetNextSelectedItem(pos);
			strBuf = m_lstSource.GetItemText(nItem, 2);
			if(strBuf == _T("-2"))
			{
				bEnable = TRUE;
				break;
			}
		}
	}
	if(!bEnable)
	{
		pSubMenu->EnableMenuItem(ID_MENU_FORCE, MF_BYCOMMAND | MF_GRAYED);
	}

	GetCursorPos(&pos);
	::SetForegroundWindow(data.hWnd);
	TrackPopupMenu(pSubMenu->m_hMenu, 0, pos.x, pos.y, 0, data.hWnd, NULL);
	menu.DestroyMenu();
	*pResult = 0;
}

void CJ2J25Dlg::OnMenuRemove()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	int nIndex = -1;
	if((nIndex = m_lstSource.GetNextItem(-1, LVNI_SELECTED)) == -1)
		return;

	POSITION pos = m_lstSource.GetFirstSelectedItemPosition();
	int nCount = m_lstSource.GetSelectedCount();
	int* nItem = new int[nCount];
	for(int i = 0; i < nCount; i++)
	{
		nItem[i] = m_lstSource.GetNextSelectedItem(pos);
	}
	for(int i = nCount - 1; i >= 0; i--)
		m_lstSource.DeleteItem(nItem[i]);
	delete [] nItem;
}

LRESULT CJ2J25Dlg::OnMessageEditUpdated(WPARAM wParam, LPARAM lParam)
{
	TCHAR* szTarget = (TCHAR*)wParam;
	m_eTargetPath.SetWindowText(szTarget);
	m_sTargetPath = szTarget;
	RefreshItems(gui_event::TargetAdded);
	return 1;
}
BOOL CJ2J25Dlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	// ESC 종료 방지
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
		return TRUE;
	return CDialog::PreTranslateMessage(pMsg);
}

void CJ2J25Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if((nID & 0xFFF0) == SC_CLOSE)
	{
		// 작업도중이면 종료하지 않도록
		if(m_bWorking)
			return;
		else
			EndDialog(IDCANCEL);
		return;
	}
	CDialog::OnSysCommand(nID, lParam);
}

void CJ2J25Dlg::OnMenuForce()
{
	// TODO: 여기에 명령 처리기 코드를 추가합니다.
	int nCount = m_lstSource.GetSelectedCount();
	if(nCount > 1)
	{
		AfxMessageBox(_T("오류무시하고 복원은 오류가 난 파일 하나만을 선택후 실행하여야 합니다."));
		return;
	}
	if(MessageBox(_T("오류무시하고 복원시 덮어쓰지 않고 사본을 생성합니다. 계속하시겠습니까?"), _T("중요 작업 확인"), MB_YESNO) != IDYES)
	{
		return;
	}
	m_bForce = TRUE;
	m_bOverwrite.SetCheck(0);
	if(m_sTargetPath == m_strTargetInfo)	// 출력경로가 없으면
	{
		m_bSamePath.SetCheck(1);
	}
	else
		m_bSamePath.SetCheck(0);
	m_brEnc.SetCheck(0);
	m_brCheck.SetCheck(0);
	m_brDec.SetCheck(1);
	OnBnClickedOk();
}

void CJ2J25Dlg::OnBnClickedButtonAddTarget()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CFileDialog dlg(TRUE, NULL, NULL,OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, _T("모든 파일(*.*)|*.*||"));
	TCHAR szBuffer[MAX_PATH * 250];
	szBuffer[0] = _T('\0');
	dlg.m_ofn.lpstrFile = szBuffer;
	dlg.m_ofn.nMaxFile = _countof(szBuffer);
	CString strBuf;
	if(dlg.DoModal() == IDOK)
	{
		for(POSITION pos = dlg.GetStartPosition(); pos != NULL;)
		{
			strBuf = dlg.GetNextPathName(pos);
			m_lstSource.AddNewItem((LPTSTR)(LPCTSTR)strBuf, 0);
		}
	}
}

void CJ2J25Dlg::OnBnClickedButtonPath()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	LPITEMIDLIST pidl;
	BROWSEINFO bi;
	TCHAR szBuf[MAX_PATH] = {0,};

	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuf;
	bi.lpszTitle = _T("대상 폴더를 선택하세요.");
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	pidl = SHBrowseForFolder(&bi);
	if(pidl != NULL)
	{
		SHGetPathFromIDList(pidl, szBuf);
		SendMessage(WM_USER_EDIT_UPDATED, (WPARAM)szBuf);
	}
}

void CJ2J25Dlg::OnBnClickedButtonSet()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CSetDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		_stprintf_s(m_szExt, MAX_PATH, _T(".%s"), dlg.GetExt());
	}
}
