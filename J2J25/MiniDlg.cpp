// MiniDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "J2J25.h"
#include "MiniDlg.h"
#include "EncDec.h"


// CMiniDlg ��ȭ �����Դϴ�.

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


// CMiniDlg �޽��� ó�����Դϴ�.

BOOL CMiniDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	m_pStat.SetRange(1, 100);

	AfxBeginThread(myThread, this, THREAD_PRIORITY_LOWEST);
	return TRUE;  // return TRUE unless you set the focus to a control
	// ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}

UINT CMiniDlg::myThread(LPVOID lp)
{
	CMiniDlg *pst = (CMiniDlg*)lp;

	// �ʱ� ����
	ENCDEC_GLOBAL *global = new ENCDEC_GLOBAL;
	::ZeroMemory(global, sizeof(ENCDEC_GLOBAL));

	global->bWrite = TRUE;	// ������ �����
	global->nMethod = 3;
	global->nWorkType = pst->m_nWorkType;

	ENCDEC_CTX *ctx = new ENCDEC_CTX;
	::ZeroMemory(ctx, sizeof(ENCDEC_CTX));
	ctx->Global = global;
	_tcscpy_s(ctx->File.szFileName, MAX_PATH, pst->m_strSource);
	// �����ϱ�, �˻��ϱ�� �������� ��ġ�� ������ ����
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
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	// ESCŰ ���� ����
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
		return TRUE;
	return CDialog::PreTranslateMessage(pMsg);
}

void CMiniDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	OnOK();
//	CDialog::OnTimer(nIDEvent);
}
