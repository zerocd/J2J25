// MiniDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "J2J25.h"
#include "MiniDlg.h"
#include "EncDec.h"


// CMiniDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CMiniDlg, CDialog)

CMiniDlg::CMiniDlg(CString strSource, int nWorkType, CWnd* pParent /*=NULL*/)
	: CDialog(CMiniDlg::IDD, pParent)
{
	m_strSource = strSource;
	m_nWorkType = nWorkType;
}

CMiniDlg::~CMiniDlg()
{
}

void CMiniDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_pStat);
}


BEGIN_MESSAGE_MAP(CMiniDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CMiniDlg 메시지 처리기입니다.

BOOL CMiniDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	m_pStat.SetRange(1, 100);

	AfxBeginThread(myThread, this, THREAD_PRIORITY_LOWEST);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

UINT CMiniDlg::myThread(LPVOID lp)
{
	CMiniDlg *pst = (CMiniDlg*)lp;

	// 초기 설정
	ENCDEC_GLOBAL *global = new ENCDEC_GLOBAL;
	::ZeroMemory(global, sizeof(ENCDEC_GLOBAL));

	global->bWrite = TRUE;	// 무조건 덮어쓰기
	global->nMethod = 3;
	global->nWorkType = pst->m_nWorkType;

	ENCDEC_CTX *ctx = new ENCDEC_CTX;
	::ZeroMemory(ctx, sizeof(ENCDEC_CTX));
	ctx->Global = global;
	_tcscpy_s(ctx->File.szFileName, MAX_PATH, pst->m_strSource);
	// 복원하기, 검사하기면 분할파일 합치는 것으로 설정
	global->bJoin = TRUE;

	CEncDec *myT = new CEncDec(ctx, &pst->m_pStat);
	BOOL bR = myT->SimpleDo(ctx);
	if(!bR)
	{
		AfxMessageBox(ctx->szError);
	}
	delete myT;
	delete ctx;
	delete global;

	pst->SetTimer(0, 1000, NULL);
//	pst->OnOK();	//>EndDialog(0);
	return 1;
}
BOOL CMiniDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	// ESC키 종료 방지
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
		return TRUE;
	return CDialog::PreTranslateMessage(pMsg);
}

void CMiniDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	OnOK();
//	CDialog::OnTimer(nIDEvent);
}
