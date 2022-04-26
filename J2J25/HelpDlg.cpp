// HelpDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "J2J25.h"
#include "HelpDlg.h"


// CHelpDlg ��ȭ �����Դϴ�.

IMPLEMENT_DYNAMIC(CHelpDlg, CDialog)

CHelpDlg::CHelpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHelpDlg::IDD, pParent)
{

}

CHelpDlg::~CHelpDlg()
{
}

void CHelpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_eHelp);
}


BEGIN_MESSAGE_MAP(CHelpDlg, CDialog)
END_MESSAGE_MAP()


// CHelpDlg �޽��� ó�����Դϴ�.

BOOL CHelpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	m_sText = _T("J2J25 v1.0.1.4 ����\n\n\n");
	m_sText += _T("1. J2J25��\n\n");
	m_sText += _T(" J2J25�� ���� ���� ���ȼ��� �ſ� ���� �ӵ��� ����(���� ������� ���)�ϴ� ����/���� ���α׷��Դϴ�.\n");
	m_sText += _T(" ���� ������ ���� �Ϻκ��� �����Ͽ� �Ϲ� ���α׷������� ������ ������ ������ �� ������ ���ݴϴ�.\n");
	m_sText += _T("\n2. ���̼���\n\n");
	m_sText += _T(" J2J25�� ���������Դϴ�. ���� �� ����� �����Դϴ�.\n �� ���α׷����� ���Ͽ� �߻��ϴ� ������ ���Ͽ� ������/�����ڴ� ���� å���� ���� ������ ����ڴ� ��� �������Ե� å���� ���� �� �����ϴ�.\n");
	m_sText += _T(" �� ���α׷��� ����Ͻô� ���� �� ���̼����� �����ϴ� ������ ���ֵ˴ϴ�.\n");
	m_sText += _T(" ���α׷��� ���װ� �߰ߵǸ� 0cd0dvd@gmail.com���� �����ֽø� �����ϵ��� �ϰڽ��ϴ�.\n");
	m_sText += _T("\n3. �۾�����\n\n");
	m_sText += _T(" �۾��� \"�����ϱ�\", \"�����ϱ�\", \"�˻��ϱ�\"�� 3������ ������ �۾� ������ ���� �Ϻ� �ɼ��� ���õ˴ϴ�.\n");
	m_sText += _T(" ��, \"�˻��ϱ�\"�� ��� \"���� �����\"�� \"���� ������ ����ϱ�\" �ɼ��� ���õ˴ϴ�.\n");
	m_sText += _T("\n 3.1 �����ϱ�\n\n");
	m_sText += _T(" ������ �����ϱ� ���� �۾��Դϴ�.\n");
	m_sText += _T(" �����ϱ�� ���������� �����۾� �Ŀ� �˻��ϱ⸦ �����ϱ� ������ ������ �˻��۾��� �ʿ����� �ʽ��ϴ�.\n");
	m_sText += _T(" �����ϱ⿡ ���ð����� �ɼ��� \"���� �����\", \"���� ������ ����ϱ�\"�Դϴ�.\n");
	m_sText += _T(" ���� ����� ���3���� �����մϴ�.\n");
	m_sText += _T("\n 3.2 �����ϱ�\n\n");
	m_sText += _T(" ������ ������ �����ϱ� ���� �۾��Դϴ�.\n");
	m_sText += _T(" �����ϱ�� ������ �κ��� ������ ������ ���Ϻκ��� �ջ�� ��츦 �������� �ʽ��ϴ�.\n");
	m_sText += _T(" �����ϱ�� ���������� �˻��ϱ⸦ ������ �Ŀ� �����ϱ� ������ ������ �˻��۾��� �ʿ����� �ʽ��ϴ�.\n");
	m_sText += _T(" �����ϱ⿡ ���ð����� �ɼ��� \"���� �����\", \"���� ������ ����ϱ�\"�Դϴ�.\n");
	m_sText += _T(" ������������ ������ ���� ���� ������ �����մϴ�.\n");
	m_sText += _T("\n 3.3 �˻��ϱ�\n\n");
	m_sText += _T(" �˻��ϱ�� ������ ������ �´����� ��й�ȣ�� ������� �˻��ϴ� ����Դϴ�.\n");
	m_sText += _T(" �˻��ϱ⿡���� ���� ������ ����ǰų� �߰����� �ʽ��ϴ�.\n");
	m_sText += _T(" �˻��ϱ⿡�� \"���� �����\"�� \"���� ������ ����ϱ�\" �ɼ��� ���õ˴ϴ�.\n");
	m_sText += _T("\n4. ��¿ɼ�\n\n");
	m_sText += _T(" ��¿ɼ��� \"���� �����\", \"���� ������ ����ϱ�\"�� ������ �Ϻ� �ɼǿ� ���� �ٸ� �ɼ� �� \"��� ���\"�� ��Ȱ��ȭ�˴ϴ�.\n");
	m_sText += _T("\n 4.1 ���� �����\n\n");
	m_sText += _T(" ���� �Ǵ� ������ ���纻�� ������ �ʰ� ������ ����� ������ �����ϴ� �ɼ��Դϴ�.\n");
	m_sText += _T(" �� �ɼ��� ���ý� �۾��ӵ��� ���� �����ϴ�.\n");
	m_sText += _T("\n 4.2 ���� ������ ����ϱ�\n\n");
	m_sText += _T(" �۾������� �ִ� ������ ��������� �����ϰ� �˴ϴ�.\n");
	m_sText += _T(" �� ��� \"��� ���\"�� ������ ���� ���ð� �Ǹ� �����Ǵ� ������ ������ ����ڰ� ������ Ȯ����(�⺻���� J2J)�� �߰��Ǹ� ������ ���� ������ ����ڰ� ������ Ȯ������ ��� �����˴ϴ�.\n");
	m_sText += _T("\n5. ��й�ȣ\n\n");
	m_sText += _T(" �����ÿ� ������ ��й�ȣ�� ������ ��й�ȣ�θ� �����̳� �˻簡 �����մϴ�.\n 1.0.0.2 ������ 1.0.0.3 ������ �������� ��й�ȣ�� ������ ��� ȣȯ���� �ʽ��ϴ�.\n ��й�ȣ���� ����θ� �⺻������ ������ ������ ����ǰ� �˴ϴ�.\n");
	m_sText += _T("\n6. �۾� ����\n\n");
	m_sText += _T(" ����, ���� �Ǵ� �˻��ϱ� ���� �۾����ϵ��� �����ϴ� ������ ���콺 �巢&������� �ϳ� �̻� �߰��� �� �ֽ��ϴ�.\n");
	m_sText += _T(" ����Ʈ�� �߰��� ���� ���� ���콺�� ���� ��� ���� ����/���¸� �˷��ִ� ǳ�������� �������ϴ�.\n");
	m_sText += _T("\n7. ��� ���\n\n");
	m_sText += _T(" ���� �Ǵ� ������ ������ �����ϱ� ���� ��� ��θ� �����ϴ� ������ ��µǱ� ���ϴ� ������ ���콺 �巢&������� ������ �� �ֽ��ϴ�.\n");
	m_sText += _T("\n8. Ž���� ����\n\n");
	m_sText += _T(" ���� �������� �־��� Ž���� ���� ����� ���ŵǾ����ϴ�.\n");
	m_sText += _T("\n9. ȯ�漳��\n\n");
	m_sText += _T(" ������ �߰��� Ȯ���ڸ� ������ �� �ֽ��ϴ�. �⺻�� J2J�̸� ���ϴ� Ȯ���ڷ� ������ �����մϴ�..\n");
	m_sText += _T(" ������ ������ Ȯ���ڿ� ������ Ȯ���ڰ� ������ �ش� Ȯ���ڸ� ���������� �׷��� ������ Ȯ���� ������ ���� �ʽ��ϴ�.\n");
	m_sText += _T("\n10. ���������ϰ� ����\n\n");
	m_sText += _T(" ���� ���������� ������ ���� ��� ������ �ߴܵǾ����� ������ �����ϰ� ������ �õ��� �� �ִ� ����� �߰��Ͽ����ϴ�.\n");
	m_sText += _T(" ���� �Ǵ� �˻簡 ������ ���Ͽ��� ���콺 ������ư�� ���� ���������ϰ� ������ �����ϸ� �˴ϴ�.\n");
	m_sText += _T(" ��, �� ��� ���� ������ �������� �ʰ� ������ �纻�� �����ϰ� �˴ϴ�.\n");
	m_sText.Replace(_T("\n"), _T("\r\n"));
	m_eHelp.SetWindowText(m_sText);
	return TRUE;  // return TRUE unless you set the focus to a control
	// ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}