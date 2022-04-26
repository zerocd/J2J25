#pragma once


// CMyListCtrl

class CMyListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CMyListCtrl)

public:
	CMyListCtrl();
	virtual ~CMyListCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
	void AddNewItem(TCHAR* szName, int nImage);
	void ChangeItem(int nItem, int nSubItem = 0, TCHAR* szName = NULL, int nImage = -1);
private:
	CToolTipCtrl m_ToolTip;
protected:
	virtual void PreSubclassWindow();
public:
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	static int CALLBACK ColumnCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
private:
	int CellRectFromPoint(CPoint &point, RECT *cellrect, int *col) const;
	BOOL OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT * pResult);
	CString m_strTip;
	void UpdateArrow(int nCol);
	BOOL ListSort(int nCol, BOOL bSort);
	BOOL m_bAscending;	// ���Ŀ�������
public:
	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
};


// �÷�0�� ���ϸ�, �÷�1�� ǳ�����򸻿� ������ ����
// �÷�2�� �۾� ��� 0�� �۾�����, -1�� ����, 1�� �۾�����