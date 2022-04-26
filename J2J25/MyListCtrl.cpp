// MyListCtrl.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "J2J25.h"
#include "MyListCtrl.h"


// CMyListCtrl

IMPLEMENT_DYNAMIC(CMyListCtrl, CListCtrl)

CMyListCtrl::CMyListCtrl()
{
	m_bAscending = FALSE;
}

CMyListCtrl::~CMyListCtrl()
{
}


BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
	ON_WM_DROPFILES()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, &CMyListCtrl::OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, &CMyListCtrl::OnHdnItemclick)
END_MESSAGE_MAP()



// CMyListCtrl �޽��� ó�����Դϴ�.



void CMyListCtrl::OnDropFiles(HDROP hDropInfo)
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
	INT_PTR qRet = -1;
	UINT nFile = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	CString filePathName;
	for(UINT i = 0; i < nFile; i++)
	{
		UINT nLength = DragQueryFile(hDropInfo, i, NULL, 0);
		TCHAR* szFile = new TCHAR[nLength + 2];
		DragQueryFile(hDropInfo, i, szFile, nLength + 2);
		AddNewItem(szFile, 0);
		delete [] szFile;
	}

	CListCtrl::OnDropFiles(hDropInfo);
}

void CMyListCtrl::AddNewItem(TCHAR* szName, int nImage)
{
	if(szName[0] == _T('\\'))
		return;	// ��Ʈ��ũ ����̺�� ����
	if(szName[3] == _T('\0'))
		return;	// ��ũ�� ����
	DWORD dwAttr = GetFileAttributes(szName);
	if((dwAttr == INVALID_FILE_ATTRIBUTES) || (dwAttr & FILE_ATTRIBUTE_DIRECTORY))
		return;	// ������ ���ų� ���丮�� ����
	int nIndex = GetItemCount();
	CString strBuf;
	for(int i = 0; i < nIndex; i++)
	{
		strBuf = GetItemText(i, 0);
		if(strBuf == szName)	// �ߺ��߰��� �ȵȴ�.
			return;
	}
	InsertItem(nIndex, szName, nImage);
	SetItem(nIndex, 1, LVIF_TEXT, _T(""), 0, 0, 0, 0);
	SetItem(nIndex, 2, LVIF_TEXT, _T("0"), 0, 0, 0, 0);
}

void CMyListCtrl::ChangeItem(int nItem, int nSubItem, TCHAR* szName, int nImage)
{
	int nCount = GetItemCount();
	if(nItem >= nCount)
		return;

	LVITEM lvItem;
	memset(&lvItem, 0, sizeof(LVITEM));
	lvItem.iItem = nItem;
	lvItem.iSubItem = nSubItem;

	if(szName != NULL)	// ������ �����Ѵ�.
	{
		lvItem.mask |= LVIF_TEXT;
		lvItem.pszText = szName;
		lvItem.cchTextMax = _tcslen(szName) * sizeof(TCHAR);
	}

	if(nItem >= 0)	// �̹����� �����Ѵ�.
	{
		lvItem.mask |= LVIF_IMAGE;
		lvItem.iImage = nImage;
	}

	SetItem(&lvItem);
}
void CMyListCtrl::PreSubclassWindow()
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.

	CListCtrl::PreSubclassWindow();
	EnableToolTips();
}

INT_PTR CMyListCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	// TODO: ���⿡ Ư��ȭ�� �ڵ带 �߰� ��/�Ǵ� �⺻ Ŭ������ ȣ���մϴ�.
	int nRow, nCol;
	RECT cellRect;
	nRow = CellRectFromPoint(point, &cellRect, &nCol);
	if(nRow == -1)
		return -1;

	pTI->hwnd = m_hWnd;
	pTI->uId = (UINT)((nRow << 10) + (nCol & 0x3ff) + 1);
	pTI->lpszText = LPSTR_TEXTCALLBACK;
	pTI->rect = cellRect;
	return pTI->uId;
//	return CListCtrl::OnToolHitTest(point, pTI);
}

int CMyListCtrl::CellRectFromPoint(CPoint &point, RECT *cellrect, int *col) const
{
	int colnum;

    // ����Ʈ�䰡 LVS_REPORT ��忡�ִ��� Ȯ��
    if( (GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK) != LVS_REPORT ) 
         return -1;

    // ���� ȭ�鿡 ���̴�ó���� �� Row �� �˾Ƴ���
    int row = GetTopIndex();
    int bottom = row + GetCountPerPage();
    if( bottom > GetItemCount() )  bottom = GetItemCount();

    // �÷����� �˾Ƴ���
    CHeaderCtrl* pHeader = (CHeaderCtrl*)GetDlgItem(0);
    int nColumnCount = pHeader->GetItemCount();

    // ���纸�̴� Row �鰣�� ���� ����
    for( ;row <=bottom;row++)       
	{
        // �������� ��� �簢���� ��������, ���� ���ԵǴ��� üũ        
        CRect rect;
        GetItemRect( row, &rect, LVIR_BOUNDS );
        if( rect.PtInRect(point) )       
		{
            // �÷�ã��
            for( colnum = 0; colnum < nColumnCount; colnum++ )
			{
                int colwidth = GetColumnWidth(colnum);                
                if( point.x >= rect.left && point.x <= (rect.left + colwidth) ) 
				{
                    RECT rectClient;
                    GetClientRect( &rectClient );
                    if( col ) *col = colnum;
                    rect.right = rect.left + colwidth;

                    // ������ ���� Ŭ���̾�Ʈ ������ ����� �ʵ��� Ȯ��
                    if( rect.right > rectClient.right )             
                             rect.right = rectClient.right;
                    *cellrect = rect;

                    return row;
                }
                rect.left += colwidth;
            }
		}
    }
    return -1;
}

BOOL CMyListCtrl::OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT * pResult)
{
	// ANSI�� UNICODE �޽��� ó��
//	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strText;
	UINT nID = pNMHDR->idFrom;

	if(nID == 0)	// NT�� �ڵ����� ���� ������ �н�
		return FALSE;

	int nRow = ((nID - 1) >> 10) & 0x3fffff;
	int nCol = (nID - 1) & 0x3ff;
//	int nSize;

	m_strTip = GetItemText(nRow, 1);	// hidden1�� ������

	if(pNMHDR->code != TTN_NEEDTEXTA)	// �ƽ�Ű�� �ƴ� ��쿡��...����;
	{
		pTTTW->lpszText = (LPTSTR)m_strTip.GetString();
	}
	return TRUE;
}

int CALLBACK CMyListCtrl::ColumnCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CString *pStr1 = (CString*)lParam1;
	CString *pStr2 = (CString*)lParam2;
	BOOL bSort = (BOOL)lParamSort;

	int nResult = _tcscmp(*pStr1, *pStr2);
	if(bSort)
		return nResult;
	else
		return -nResult;
}

void CMyListCtrl::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	CHeaderCtrl* pHeader = GetHeaderCtrl();
	int nCount = pHeader->GetItemCount();

	for(int i = 0; i < nCount; i++)
	{
		if(phdr->iItem == i)
		{
			m_bAscending = !m_bAscending;
			UpdateArrow(i);
			ListSort(phdr->iItem, m_bAscending);
		}
	}
	*pResult = 0;
}

void CMyListCtrl::UpdateArrow(int nCol)
{
	CHeaderCtrl* pHeader = GetHeaderCtrl();
	int nCount = pHeader->GetItemCount();
	UINT bitmapID;

	for(int i = 0; i < nCount; i++)
	{
		HDITEM hditem = {0};
		hditem.mask = HDI_FORMAT;
		pHeader->GetItem(i, &hditem);
		hditem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		if(i == nCol)
		{
			if(m_bAscending)
				hditem.fmt |= HDF_SORTDOWN;
			else
				hditem.fmt |= HDF_SORTUP;
		}
		pHeader->SetItem(i, &hditem);
	}

	if(m_bAscending)
		bitmapID = IDB_SORTDOWN;
	else
		bitmapID = IDB_SORTUP;

	for(int i = 0; i < nCount; i++)
	{
		HDITEM hditem = {0};
		hditem.mask = HDI_BITMAP | HDI_FORMAT;
		pHeader->GetItem(i, &hditem);
		if(hditem.fmt & HDF_BITMAP && hditem.fmt & HDF_BITMAP_ON_RIGHT)
		{
			if(hditem.hbm)
			{
				DeleteObject(hditem.hbm);
				hditem.hbm = NULL;
			}
			hditem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
			pHeader->SetItem(i, &hditem);
		}
		if(i == nCol)
		{
			hditem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
			hditem.hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(bitmapID), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS); 
			pHeader->SetItem(i, &hditem);
		}
	}
}

BOOL CMyListCtrl::ListSort(int nCol, BOOL bSort)
{
	int nItem = GetItemCount();
	CString **arStr = new CString*[nItem];

	// Callback Item Setting
	for(int i = 0; i < nItem; i++)
	{
		arStr[i] = new CString(GetItemText(i, nCol));
		SetItemData(i, (LPARAM)arStr[i]);
	}
	SortItems(ColumnCompare, (LPARAM)bSort);

	for(int i = 0; i < nItem; i++)
		delete arStr[i];
	delete [] arStr;
	return TRUE;
}