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
	BOOL m_bAscending;	// 정렬오름내림
public:
	afx_msg void OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult);
};


// 컬럼0은 파일명, 컬럼1은 풍선도움말에 보여줄 내용
// 컬럼2는 작업 결과 0은 작업않음, -1은 실패, 1은 작업성공