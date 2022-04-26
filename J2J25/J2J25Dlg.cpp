// J2J25Dlg.cpp : ���� ����
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


// CJ2J25Dlg ��ȭ ����




CJ2J25Dlg::CJ2J25Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJ2J25Dlg::IDD, pParent)
	, m_nWorkType(0)
	, m_sTargetPath(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strTargetInfo = _T("����� ������ �巢 & ����ϼ���");
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


// CJ2J25Dlg �޽��� ó����

BOOL CJ2J25Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.

	// ���� ����
	typedef BOOL (WINAPI *CHANGEWINDOWMESSAGEFILTEREX)(HWND hWNd, UINT message, DWORD action, LPVOID lp);

	// ���� �������κ��͵� �޽����� �����ϱ� ����
	CHANGEWINDOWMESSAGEFILTEREX ChangeWindowMessageFilterEx = NULL;
	HINSTANCE hM;
	hM = LoadLibrary(_T("USER32.DLL"));
	if(hM)
	{
		ChangeWindowMessageFilterEx = (CHANGEWINDOWMESSAGEFILTEREX)GetProcAddress(hM, "ChangeWindowMessageFilterEx");
		if(ChangeWindowMessageFilterEx)	// Win7 ���Ͽ����� �ش� �Լ��� �����Ƿ� �н�
		{
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), WM_COPYDATA, 1, NULL);
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), WM_DROPFILES, 1, NULL);
			ChangeWindowMessageFilterEx(this->GetSafeHwnd(), 0x0049, 1, NULL);
		}
		FreeLibrary(hM);
	}

	m_pStat.SetRange(1, 100);
	m_ePassword.SetLimitText(32);

	m_lstSource.InsertColumn(0, _T("���ϸ�"), LVCFMT_LEFT);
	m_lstSource.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_lstSource.InsertColumn(1, _T(""), LVCFMT_LEFT, 0);	// hidden : ǳ�����򸻿�
	m_lstSource.InsertColumn(2, _T(""), LVCFMT_LEFT, 0);	// hidden2 : �۾������
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

	// �ʱ�ȭ
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

	// todo: ������Ʈ���� ���� ���� ���� �о������ �ұ�?
#if (JEJES_READONLY == 1)
	m_brEnc.EnableWindow(0);
	m_brDec.SetCheck(TRUE);
#else	// �⺻��� 2�� ���� ��Ȱ��ȭ
#endif

#if (JEJES_READONLY == 1)
	RefreshItems(gui_event::DecryptSelected);
#else
	RefreshItems(gui_event::EncryptSelected);
#endif
	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CJ2J25Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CJ2J25Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CJ2J25Dlg::OnBnClickedCheck1()
{	// ���� �����
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	RefreshItems(gui_event::OverwriteSelected, m_bOverwrite.GetCheck());
}

void CJ2J25Dlg::OnBnClickedCheck2()
{	// ���� ������ ����ϱ�
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	RefreshItems(gui_event::SamePathSelected, m_bSamePath.GetCheck());
}

void CJ2J25Dlg::OnBnClickedOk()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	UpdateData();

	if(m_lstSource.GetItemCount() < 1)
	{
		AfxMessageBox(_T("�۾������� �巢 & ������� ����Ʈ�� �߰��� �ּ���."));
		return;
	}

	m_bWrite = m_bOverwrite.GetCheck();
	BOOL bSamePath = m_bSamePath.GetCheck();
	if((m_sTargetPath.GetLength() == 0) || (m_sTargetPath == m_strTargetInfo))
	{
		if((m_nWorkType != 2) && !m_bWrite && !bSamePath)
		{
			AfxMessageBox(_T("��� ��θ� �����ϰų� ��¿ɼ��� �������ּ���."));
			return;
		}
	}

	// ��й�ȣ Ȯ��
	m_ePassword.GetWindowText(m_szPassword, 64);

	m_bWorking = TRUE;
	AfxBeginThread(myThread, this, THREAD_PRIORITY_LOWEST);

//	OnOK();
}

UINT CJ2J25Dlg::myThread(LPVOID lp)
{
	CJ2J25Dlg *pst = (CJ2J25Dlg *)lp;

	// �ʱ� ����
	int nWorkType = pst->m_nWorkType;
//	pst->m_bWorking = TRUE; OnBnClickOK���� �������� ��...
	pst->RefreshItems(gui_event::JobStarting);
	ENCDEC_GLOBAL *global = new ENCDEC_GLOBAL;
	::ZeroMemory(global, sizeof(ENCDEC_GLOBAL));

	global->bSamePath = pst->m_bSamePath.GetCheck();
	global->bWrite = pst->m_bWrite;
	global->nMethod = 3;
	global->nWorkType = pst->m_nWorkType;
	_tcscpy_s(global->szExt, MAX_PATH, pst->m_szExt);
	if(nWorkType == 2)	// �˻��̸� ������ FALSE, ���� ������ TRUE
	{
		global->bWrite = FALSE;
		global->bSamePath = TRUE;
	}
	_tcscpy_s(global->szPassword, _countof(global->szPassword), pst->m_szPassword);
	if(pst->m_sTargetPath == pst->m_strTargetInfo)
		global->szTargetFolder[0] = _T('\0');
	else
		_tcscpy_s(global->szTargetFolder, _countof(global->szTargetFolder), pst->m_sTargetPath);
	// �����ϱ�, �˻��ϱ�� �ϴ� �������� ��ġ�� ������ ����
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
		global->bForce = TRUE;	// ���� ����
		// ���� ���ϸ� ������� �Ѵ�.
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
			// ������ ����� nItem�� ���ؼ� �۾��Ѵ�.
		}
		else
		{
			nItem = i;
		}
		// �Ź� ���־���ϴ� �ʱ�ȭ��...
		global->nWorkType = nWorkType;
		::ZeroMemory(ctx, sizeof(ENCDEC_CTX));
		ctx->Global = global;
		strTarget = pst->m_lstSource.GetItemText(nItem, 0);
		_tcscpy_s(ctx->File.szFileName, MAX_PATH, strTarget);

		CEncDec *myT = new CEncDec(ctx, &pst->m_pStat);
		BOOL bR = myT->SimpleDo(ctx);
		// ����Ʈ�� ��� ����
		if(!bR)
		{
			pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);
			if((nWorkType == 2) && (_tcscmp(ctx->szError, _T("������ ������ �ƴϰų� �ջ�� �����Դϴ�.")) == 0))
			{
				// �˻��ε� ������������ Ȯ���� �� ���� ��
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
			if(nWorkType == 2)	// �˻��̸�
			{
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Encrypted);
				pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);
			}
			else if(pst->m_bWrite)	// ������ �ش� ���� ���� ����
			{
				if(ctx->Core.nMethod == 3)	// ���3�̸� Ȯ���� ó��
				{
					TCHAR szNew[MAX_PATH];
					_stprintf_s(szNew, MAX_PATH, _T("%s%s"), ctx->File.szNewName, ctx->File.szNewExt);
					CString strName = pst->m_lstSource.GetItemText(nItem, 0);
					if(strName != szNew)	// ����Ǿ�����
					{
						pst->m_lstSource.ChangeItem(nItem, 0, szNew);
					}
				}
				if((nWorkType == 0) || (nWorkType == 2))	// ���� �Ǵ� �˻�
					pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Encrypted);
				else	// ����
					pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Success);
				pst->m_lstSource.ChangeItem(nItem, 1, ctx->szError);	// ������ ���� ���� ������ �����Ѵ�.
			}
			else	// ����Ⱑ �ƴϰ� �˻簡 �ƴϸ�
			{
				pst->m_lstSource.ChangeItem(nItem, 0, NULL, list_icon::Success);
				CString strResult = _T("�۾� ����. ") + pst->m_lstSource.GetItemText(nItem, 1);
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	RefreshItems(gui_event::EncryptSelected);
}

void CJ2J25Dlg::OnBnClickedRadio2()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	RefreshItems(gui_event::DecryptSelected);
}

void CJ2J25Dlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.

	CDialog::OnDropFiles(hDropInfo);
}

void CJ2J25Dlg::RefreshItems(int nEvent, BOOL bCheck)
{
	switch(nEvent)
	{
	case gui_event::EncryptSelected:	// �����ϱ�
	case gui_event::DecryptSelected:	// �����ϱ�
	case gui_event::CheckSelected:	// �˻��ϱ�
		// �� �� ���� ����...
		break;
	case gui_event::OverwriteSelected:	// ����� ���õ�
	case gui_event::SamePathSelected:	// ���� ������ ����ϱ�
		m_eTargetPath.SetWindowText(m_strTargetInfo);
		break;
	case gui_event::JobStarting:	// �۾� ����
		m_ePassword.EnableWindow(0);
		m_bOK.EnableWindow(0);
		m_bCleanListSuccess.EnableWindow(0);
		m_bCleanListAll.EnableWindow(0);
		m_bOverwrite.EnableWindow(0);
		m_bSamePath.EnableWindow(0);
		m_bCancel.SetWindowText(_T("����"));
		m_bAddFile.EnableWindow(0);
		m_bAddPath.EnableWindow(0);
		m_bSet.EnableWindow(0);
		break;
	case gui_event::JobFinished:	// �۾� ����
		m_ePassword.EnableWindow();
		m_bOK.EnableWindow();
		m_bCleanListSuccess.EnableWindow();
		m_bCleanListAll.EnableWindow();
		m_bOverwrite.EnableWindow();
		m_bSamePath.EnableWindow();
		m_bCancel.SetWindowText(_T("�ݱ�"));
		m_bAddFile.EnableWindow();
		m_bAddPath.EnableWindow();
		m_bSet.EnableWindow();
		break;
	case gui_event::TargetAdded:	// ������� ���õ�
		m_bOverwrite.SetCheck(0);
		m_bSamePath.SetCheck(0);
		UpdateData();
		break;
	}
}

void CJ2J25Dlg::OnBnClickedRadio3()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	RefreshItems(gui_event::CheckSelected);
}

void CJ2J25Dlg::OnBnClickedHelp()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	CHelpDlg dlg;
	dlg.DoModal();
}

void CJ2J25Dlg::OnBnClickedClearlistsuccess()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	m_lstSource.DeleteAllItems();
}

void CJ2J25Dlg::OnBnClickedCancel()
{
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	// �۾����̸� �������콺 �ȵ�
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
	// TODO: ���⿡ ��� ó���� �ڵ带 �߰��մϴ�.
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
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	// ESC ���� ����
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
		return TRUE;
	return CDialog::PreTranslateMessage(pMsg);
}

void CJ2J25Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	if((nID & 0xFFF0) == SC_CLOSE)
	{
		// �۾������̸� �������� �ʵ���
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
	// TODO: ���⿡ ��� ó���� �ڵ带 �߰��մϴ�.
	int nCount = m_lstSource.GetSelectedCount();
	if(nCount > 1)
	{
		AfxMessageBox(_T("���������ϰ� ������ ������ �� ���� �ϳ����� ������ �����Ͽ��� �մϴ�."));
		return;
	}
	if(MessageBox(_T("���������ϰ� ������ ����� �ʰ� �纻�� �����մϴ�. ����Ͻðڽ��ϱ�?"), _T("�߿� �۾� Ȯ��"), MB_YESNO) != IDYES)
	{
		return;
	}
	m_bForce = TRUE;
	m_bOverwrite.SetCheck(0);
	if(m_sTargetPath == m_strTargetInfo)	// ��°�ΰ� ������
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	CFileDialog dlg(TRUE, NULL, NULL,OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, _T("��� ����(*.*)|*.*||"));
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	LPITEMIDLIST pidl;
	BROWSEINFO bi;
	TCHAR szBuf[MAX_PATH] = {0,};

	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuf;
	bi.lpszTitle = _T("��� ������ �����ϼ���.");
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
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	CSetDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		_stprintf_s(m_szExt, MAX_PATH, _T(".%s"), dlg.GetExt());
	}
}
