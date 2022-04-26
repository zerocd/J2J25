#include "stdafx.h"
#include "EncDec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CEncDec::CEncDec(ENCDEC_CTX* ctx, CProgressCtrl* pStat)
{
	m_CTX = ctx;
	m_pStat = pStat;
}

CEncDec::~CEncDec()
{
	InitHandle(m_CTX, TRUE, TRUE);
}

BOOL CEncDec::CheckValid(ENCDEC_CTX* ctx)	// ���� ��ȿ�� �˻�
{
	// ������ �����ϴ��� Ȯ��
	DWORD dwAtt = GetFileAttributes(ctx->File.szFileName);
	if((dwAtt == INVALID_FILE_ATTRIBUTES) || (dwAtt & FILE_ATTRIBUTE_DIRECTORY))
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("�۾������� ���ų� ������ �ƴմϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}

	if(!CheckFileSize(ctx))
		return FALSE;

	ctx->Core.bSamePath = ctx->Global->bSamePath;
	ctx->Core.bWrite = ctx->Global->bWrite;
	ctx->Core.bJoin = ctx->Global->bJoin;
	if(ctx->Core.bJoin == TRUE)	// ���� ��ġ���
	{
		if(!CheckJoinFile(ctx))
			return FALSE;
	}

	if(ctx->Core.bJoin == FALSE)
	{
		if(ctx->Global->llSplitSize == 0LL)	// ���Ҿ����̸�
			ctx->Core.nFiles = 1;
		else
		{
			if(ctx->Core.llTotalSize < ctx->Global->llSplitSize)
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("����ũ�Ⱑ �ʹ� Ů�ϴ�. ����ũ��� ����ũ�⺸�� Ŭ �� �����ϴ�."));
				ctx->nResult = -1;
				return FALSE;
			}
			ctx->Core.nFiles = ctx->Core.llTotalSize / ctx->Global->llSplitSize + 1;
			if(ctx->Core.nFiles > 999)
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("����ũ�Ⱑ �ʹ� �۽��ϴ�. ���������� 1~999�����Դϴ�."));
				ctx->nResult = -1;
				return FALSE;
			}
		}
	}

	// ���� Ȯ��
	if(ctx->Global->nWorkType != 0)
	{
		int nVersion = CheckValidVersion(ctx);
		switch(nVersion)
		{
		case -1:
		case 0:
		case 1:
			ctx->Core.nMethod = nVersion;
			break;
		case 2:
		case 3:
			ctx->Core.nMethod = nVersion;
			if(ctx->Core.nFiles != 1)	// ���2,3�� ������ �������� �����Ƿ�
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("�ջ�� �����Դϴ�."));
				ctx->nResult = -1;
				return FALSE;
			}
			break;
		case 9:
			_tcscpy_s(ctx->szError, MAX_PATH, _T("�������� �ʴ� �������� ������ �����Դϴ�. ������������ �����غ�����."));
			ctx->nResult = -1;
			return FALSE;
			break;
		default:
			_tcscpy_s(ctx->szError, MAX_PATH, _T("������ ������ �ƴϰų� �ջ�� �����Դϴ�."));
			ctx->nResult = -1;
			return FALSE;
			break;
		}
	}
	else
		ctx->Core.nMethod = ctx->Global->nMethod;

	// ��°�� �� ���� Ȯ��
	if(!CheckTargetFile(ctx))
		return FALSE;

	return TRUE;
}

BOOL CEncDec::CheckFileSize(ENCDEC_CTX *ctx)
{
	// ��������ũ�� Ȯ�� �� ������ �����ּ�ũ�� Ȯ��
	HANDLE hFind;
	WIN32_FIND_DATA fFile;
	hFind = FindFirstFile(ctx->File.szFileName, &fFile);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("�۾����� ������ Ȯ���� �� �����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
	FindClose(hFind);
	ctx->Core.llTotalSize = (ULONGLONG)fFile.nFileSizeHigh << (sizeof(int) * 8);
	ctx->Core.llTotalSize += fFile.nFileSizeLow;
	if((ctx->Global->nWorkType == 0)	// && (ctx->Global->nMethod < 2)
		&& (ctx->Core.llTotalSize < BUFFER_SIZE_METHOD2 * INTERVAL_COUNT_METHOD2))
	{
		_tcscpy_s(ctx->szError, MAX_PATH, _T("�۾������� �ʹ� �۽��ϴ�."));
		ctx->nResult = -1;
		return FALSE;
	}
	return TRUE;
}

BOOL CEncDec::CheckJoinFile(ENCDEC_CTX *ctx)
{
	CString strExt = _T("001");
	CString FileName;
	CString SrcFile = ctx->File.szFileName;
	ctx->Global->llSplitSize = 0LL;
	ULONGLONG llTotalSize = 0LL;
	// ������ ũ������, ���ϵ��� �� �ִ��� Ȯ��
	int nErr = 0;
	int nNo = 1;	// Ȯ���ڸ��
	FileName = SrcFile.Left(SrcFile.ReverseFind(_T('.')));
	HANDLE hFind;
	WIN32_FIND_DATA fFile;
	while(1)
	{
		CString File;
		File.Format(_T("%s.%03d"), FileName, nNo);
		hFind = FindFirstFile(File, &fFile);
		if(hFind == INVALID_HANDLE_VALUE)
		{
			if(nErr <= 1)	// ����
			{
				File.Format(_T("%s.%03d"), FileName, nNo + 1);
				DWORD dwAtt = GetFileAttributes(File);
				if(dwAtt == INVALID_FILE_ATTRIBUTES)
				{
					ctx->Core.nFiles = nNo -1;
					if(ctx->Core.nFiles < 1)
					{
						ctx->Core.bJoin = FALSE;	// ���� �ϳ��̸� Join�� �ƴ�
						ctx->Core.nFiles = 1;
					}
					if(ctx->Core.nFiles > 1)
					{
						if(ctx->Core.bWrite)
						{
							ctx->Core.bWrite = FALSE;	// ��ġ��� ������� �� ����
							ctx->Core.bSamePath = TRUE;	// ��ġ��� �ϴ� ���� ������ ����
						}
						else
						{	// ����Ⱑ �ƴϸ� ���� ���� �Ǵ� ������ �����Ǿ� �ִ�.
						}
						ctx->Core.llTotalSize = llTotalSize;	// ��ġ��� ��Żũ�� �ٽ� ���
						_tcscpy_s(ctx->File.szNewName, MAX_PATH, FileName);
					}
					else
						_tcscpy_s(ctx->File.szNewName, MAX_PATH, ctx->File.szFileName);
					return TRUE;
				}
				else
				{
					_stprintf_s(ctx->szError, MAX_PATH, _T("������ ������ �����մϴ�.(%03d)"), nNo);
					ctx->nResult = -1;
					return FALSE;
				}
			}
			else
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("���ҵ� ������ ũ�Ⱑ Ʋ���ϴ�."));
				ctx->nResult = -1;
				return FALSE;
			}
		}
		FindClose(hFind);
		ULONGLONG fSize = (ULONGLONG)fFile.nFileSizeHigh << (sizeof(int) * 8);
		fSize += fFile.nFileSizeLow;
		llTotalSize += fSize;
		if(ctx->Global->llSplitSize == 0LL)	// ���� �������� �ʾ����� ������Ʈ
			ctx->Global->llSplitSize = fSize;
		else
		{
			if(nErr > 0)	// �̹� ������ �߻������� ���� ������ ������ �ȵȴ�.
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("���ҵ� ������ ũ�Ⱑ Ʋ���ϴ�."));
				ctx->nResult = -1;
				return FALSE;
			}
			if(fSize != ctx->Global->llSplitSize)
				nErr++;
		}
		nNo++;
	}
	ctx->Core.llTotalSize = llTotalSize;	// ��ġ��� ��Żũ�� �ٽ� ���
	_tcscpy_s(ctx->File.szNewName, MAX_PATH, FileName);
	return TRUE;
}

BOOL CEncDec::CheckTargetFile(ENCDEC_CTX *ctx)
{
	DWORD dwAtt;
	TCHAR szDrv[MAX_PATH], szPath[MAX_PATH], szName[MAX_PATH], szExt[MAX_PATH];
	_tsplitpath_s(ctx->File.szFileName, szDrv, MAX_PATH, szPath, MAX_PATH, szName, MAX_PATH, szExt, MAX_PATH);

	// ��θ� ������Ʈ
	if((ctx->Global->nWorkType == 2) || ctx->Core.bWrite || ctx->Core.bSamePath)
		// �˻�, �����, ���� ����� ��쿡�� ��θ��� ��� ��ġ
	{
		_stprintf_s(ctx->File.szNewName, MAX_PATH, _T("%s%s%s"), szDrv, szPath, szName);
	}
	else	// �ٸ� ���
	{
		dwAtt = GetFileAttributes(ctx->Global->szTargetFolder);
		if((dwAtt == INVALID_FILE_ATTRIBUTES) || !(dwAtt & FILE_ATTRIBUTE_DIRECTORY))
		{
			m_dwErr = GetLastError();
			CString warn;
			warn.Format(_T("��� ��ΰ� �߸��Ǿ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		_tcscpy_s(ctx->File.szNewName, MAX_PATH, ctx->Global->szTargetFolder);
		if(ctx->File.szNewName[_tcslen(ctx->File.szNewName) - 1] != _T('\\'))
			_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("\\"));
		_tcscat_s(ctx->File.szNewName, MAX_PATH, szName);
	}

	// Ȯ���� �� ó��
	if(ctx->Global->nWorkType == 2)	// �˻��̸�
	{
		if(!ctx->Global->bJoin)	// ��ġ�Ⱑ �ƴϸ� Ȯ���� �߰�
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
	}
	else if(ctx->Core.nMethod == 3)
	{
		if(ctx->Global->nWorkType == 0)	// ��ȣȭ�� j2j �߰�
		{
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
			_tcscat_s(ctx->File.szNewName, MAX_PATH, ctx->Global->szExt);
		}
		else
		{	// �ִ� .j2j�� ���������� OK
		}
	}
	else
	{
		if((ctx->Core.nMethod < 2) && (ctx->Core.bJoin == TRUE))	// ������������ ���� ��ġ���
		{	// �ִ� .001�� ���������� OK
		}
		else if(ctx->Core.bWrite)	// �����
		{
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
		}
		else if(ctx->Core.bSamePath)
		{
			if(ctx->Global->nWorkType == 0)	// ��ȣȭ
				_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("_enc"));
			else
				_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("_dec"));
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
		}
		else
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
	}

	// Method3�̰� (�˻簡 �ƴϰų� �߰��۾��� ������) ������� Ȯ��
	if((ctx->Core.nMethod == 3) && ((ctx->Global->nWorkType != 2) || ctx->Global->bExistNextJob))
	{
		dwAtt = GetFileAttributes(ctx->File.szNewName);
		if(dwAtt != INVALID_FILE_ATTRIBUTES)
		{
			_tcscpy_s(ctx->szError, MAX_PATH, _T("��� ��ο� ���� �̸��� ������ �̹� �����մϴ�."));
			ctx->nResult = -1;
			return FALSE;
		}
	}
	// ������� Ȯ��
	else if(!ctx->Core.bWrite && (ctx->Global->nWorkType != 2))
	{
		if((ctx->Global->nWorkType != 0) || (ctx->Global->llSplitSize == 0LL))
			dwAtt = GetFileAttributes(ctx->File.szNewName);
		else
		{
			CString tmp = ctx->File.szNewName;
			tmp += _T(".001");
			dwAtt = GetFileAttributes(tmp);
		}
		if(dwAtt != INVALID_FILE_ATTRIBUTES)
		{
			_tcscpy_s(ctx->szError, MAX_PATH, _T("��� ��ο� ���� �̸��� ������ �̹� �����մϴ�."));
			ctx->nResult = -1;
			return FALSE;
		}
	}
	return TRUE;
}

int CEncDec::CheckValidVersion(ENCDEC_CTX *ctx)
{
	LARGE_INTEGER llToMove;
	BYTE Buf[17] = {0, };
	int nReaded = 0;
	DWORD dwOrder, dwRead;
	CString File;
	if(ctx->Core.nFiles > 1)
		File.Format(_T("%s.%03d"), ctx->File.szNewName, ctx->Core.nFiles);
	else
		File = ctx->File.szFileName;
	HANDLE hF = CreateFile(File, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(hF == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		return -2;	// ���� �� �� ����
	}
	LARGE_INTEGER FileSize;
	GetFileSizeEx(hF, &FileSize);
	if(FileSize.QuadPart >= 16)
		dwOrder = 16;
	else
		dwOrder = FileSize.LowPart;

	llToMove.QuadPart = dwOrder;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(hF, llToMove, NULL, FILE_END);
	ReadFile(hF, Buf + 16 - dwOrder, dwOrder, &dwRead, NULL);
	CloseHandle(hF);
	nReaded = dwOrder;
	if(nReaded < 16)	// �� �о������ ���� ���� �����
	{
		if(ctx->Core.nFiles < 1)
			return -2;	// �߸��� ����
		File.Format(_T("%s.%03d"), ctx->File.szFileName, ctx->Core.nFiles - 1);
		hF = CreateFile(File, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if(hF == INVALID_HANDLE_VALUE)
			return -2;	// ���� �� �� ����
		dwOrder = 16 - nReaded;
		llToMove.QuadPart = dwOrder;
		llToMove.QuadPart *= -1;
		SetFilePointerEx(hF, llToMove, 0, FILE_END);
		ReadFile(hF, Buf, dwOrder, &dwRead, NULL);
		CloseHandle(hF);
	}

	// ý��Ȯ��
	int nNull = 0;
	for(int i = 1; i < 16; i += 2)
	{
		if(Buf[i] == 0)
			nNull++;
	}
	if(nNull == 0)	// ���������� üũ
	{
		for(int i = 0; i < 8; i++)
		{
			if((Buf[i] >= '0') && (Buf[i] <= '9'))
				nNull++;
			else if((Buf[i] >= 'A') && (Buf[i] <= 'F'))
				nNull++;
		}
		if(nNull != 8)
			return -3;
		if(Buf[8] != 'L')
			return -3;
		nNull = 0;
		for(int i = 9; i < 16; i++)
		{
			if((Buf[i] >= '0') && (Buf[i] <= '9'))
				nNull++;
		}
		if(nNull != 7)
			return -3;
		char* szSN = (char*)(Buf + 10);
		int nVer = atoi(szSN);

		if(nVer > 9)	// ��������?
			return 9;

		if(Buf[9] == '0')	// OVER5
			return 0;
		else if(Buf[9] == '1')
			return 1;
		else if(Buf[9] == '2')
			return 2;
		else if(Buf[9] == '3')
			return 3;
		else
			return -3;
	}
	else if(nNull == 8)	// ���������� üũ
	{
		nNull = 0;
		for(int i = 0; i < 16; i += 2)
		{
			if((Buf[i] >= '0') && (Buf[i] <= '9'))
				nNull++;
			else if((Buf[i] >= 'A') && (Buf[i] <= 'F'))
				nNull++;
		}
		if(nNull == 8)	// ������
			return -1;
		else	// ����
			return -3;
	}
	else	// ����
		return -3;
}

void CEncDec::InitDefaultValues(ENCDEC_CTX* ctx)	// �⺻���� ����
{
	switch(ctx->Core.nMethod)
	{
	case -1:	// ������
		ctx->Core.llBufferSize = BUFFER_SIZE_UNDER5;
		ctx->Core.llInterval = INTERVAL_COUNT_UNDER5;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_UNDER5;
		break;
	case 0:	// 5����
		ctx->Core.llBufferSize = BUFFER_SIZE_OVER5;
		ctx->Core.llInterval = INTERVAL_COUNT_OVER5;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_OVER5;
		break;
	case 1:	// ���1
		ctx->Core.llBufferSize = BUFFER_SIZE_METHOD1;
		ctx->Core.llInterval = INTERVAL_COUNT_METHOD1;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_METHOD1;
		break;
	case 2:	// ���2
	case 3:	// ���3
		ctx->Core.llBufferSize = BUFFER_SIZE_METHOD2;
		ctx->Core.llInterval = INTERVAL_COUNT_METHOD2;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_METHOD2;
		break;
	}

	// �ڵ� ���� �˻������ ���ԵǹǷ� ���� ������ŭ hSrc, hDst�� ����
	ctx->File.hSrc = new HANDLE[ctx->Core.nFiles];
	ctx->File.hDst = new HANDLE[ctx->Core.nFiles];
	InitHandle(ctx, FALSE, FALSE);
/*	if(ctx->Global->nWorkType == 0)	// ��ȣȭ��
	{
		ctx->File.hSrc = new HANDLE[1];	// �ҽ��� �ϳ�
		*(ctx->File.hSrc) = NULL;
		if(ctx->Core.bWrite)	// ������
			ctx->File.hDst = ctx->File.hSrc;
		else
		{
			ctx->File.hDst = new HANDLE[ctx->Core.nFiles];	// ����� ������
			for(int i = 0; i < ctx->Core.nFiles; i++)
				ctx->File.hDst[i] = NULL;
		}
	}
	else
	{
		ctx->File.hSrc = new HANDLE[ctx->Core.nFiles];
		for(int i = 0; i < ctx->Core.nFiles; i++)
			ctx->File.hSrc[i] = NULL;
		if(ctx->Core.bWrite)	// ������
			ctx->File.hDst = ctx->File.hSrc;
		else
		{
			ctx->File.hDst = new HANDLE[1];
			*(ctx->File.hDst) = NULL;
		}
	}
	*/
}

BOOL CEncDec::SimpleDo(ENCDEC_CTX *ctx)
{
#ifdef DEEP_DEBUG
	m_strDbg.Empty();
#endif
	int nNowWork, nNextWork;
	BOOL bNeedTwice = TRUE;
	ctx->File.dwShareRight = NULL;	// �⺻�� ����
	if(ctx->Global->nWorkType == 0)	// ��ȣȭ
	{
		ctx->Global->bExistNextJob = TRUE;
		nNowWork = 0;
		nNextWork = 2;
	}
	else if(ctx->Global->nWorkType == 1)	// ��ȣȭ
	{
		ctx->Global->bExistNextJob = TRUE;
		nNowWork = 2;
		nNextWork = 1;
	}
	else	// �˻�
	{
		nNowWork = 2;
		bNeedTwice = FALSE;	// 1ȸ�� ��
		ctx->File.dwShareRight = FILE_SHARE_READ;	// ���� �� �ֵ���
	}

	ctx->Global->nWorkType = nNowWork;

	// 1�� �۾� ����
	BOOL bReturn = CheckValid(ctx);

	if(!bReturn)
		return bReturn;
	InitDefaultValues(ctx);
	bReturn = DoJob(ctx);
	if(!bReturn)
	{
#ifdef DEEP_DEBUG
		AfxMessageBox(m_strDbg);
#endif
		return bReturn;
	}

	InitHandle(ctx, TRUE, TRUE);

	if(!bNeedTwice)	// 1ȸ�� ���̸�
	{
		if(bReturn)	// ����Ϸ�� ���� ���� ������Ʈ
		{
			UpdateResultString(ctx, nNowWork);
		}
#ifdef DEEP_DEBUG
		AfxMessageBox(m_strDbg);
#endif
		return bReturn;
	}

	if(nNowWork == 0)	// ���������� ��ũ�� ���� 3�� ���
		Sleep(3000);

	// �߰��۾��� ����.
	ctx->Global->bExistNextJob = FALSE;
	if((ctx->Core.nMethod == 3) && (nNowWork == 0))	// �����ų� ����������
	{
		// ���ϸ� ������Ʈ
		_tcscpy_s(ctx->File.szFileName, MAX_PATH, GetFirstTargetFile(ctx));
	}
	ctx->Global->nWorkType = nNextWork;

	// 2�� �۾� ����
	bReturn = CheckValid(ctx);
	if(!bReturn)
		return bReturn;
	InitDefaultValues(ctx);
	bReturn = DoJob(ctx);

	InitHandle(ctx, TRUE, TRUE);
	if(bReturn)	// ����Ϸ�� �������� ������Ʈ
	{
		if(nNextWork == 2)	// ���⿡�� �˻��̸� ��ȣȭ�� �ǹ�
			UpdateResultString(ctx, 0);
		else	// ����
			UpdateResultString(ctx, 1);
	}
#ifdef DEEP_DEBUG
	AfxMessageBox(m_strDbg);
#endif
	return bReturn;
}

void CEncDec::InitLoopDefaults(ENCDEC_CTX *ctx)
{
	// ����Ű ����
	int nLen = _tcslen(ctx->Global->szPassword);
	BYTE* szPass = (BYTE*)ctx->Global->szPassword;
	int nBufSize;
	nBufSize = ctx->Core.llBufferSize;

	if(nLen == 0)	// �⺻Ű
	{
		for(int i = 0; i < nBufSize; i++)
		{
			ctx->Core.IV[i] = (BYTE)i;
		}
	}
	else
	{
		nLen *= 2;
		for(int i = 0, j = 0; i < nBufSize; i++)
		{
			ctx->Core.IV[i] = szPass[j++] + ((ctx->Core.nMethod != -1) ? i : 0);
			if(j > nLen)
				j = 0;
		}
	}
	memcpy_s(ctx->Core.IV_Cmp, nBufSize, ctx->Core.IV, nBufSize);

	ctx->Core.Crcs.Init();
	ctx->Core.dwCRC = 0xFFFFFFFF;
	ctx->Core.hSrc = NULL;
	ctx->Core.hDst = NULL;
	ctx->Core.llProcessedSize = 0LL;
	ctx->Core.llRemainSize = ctx->Core.llTotalSize;
	ctx->Core.llFileProcessedSize = 0LL;
	ctx->Core.nReadSize = 0;
	ctx->Core.nWrittenSize = 0;
	ctx->Core.nCrc = 0;
	ctx->Core.bIV1 = TRUE;
	ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
	ctx->Core.nWorkType = ctx->Global->nWorkType;
}

BOOL CEncDec::FileLoop(ENCDEC_CTX *ctx)
{
	LARGE_INTEGER llToMove;
	int nReopen = 3;
	for(int i = 0; i < ctx->Core.nFiles; i++)
	{
		if(ctx->Core.hSrc == NULL)
		{
			// �ҽ����� 
			CString SrcFile;
			HANDLE* lpH;
			if((ctx->Core.nWorkType != 0) && ctx->Core.bJoin)	// ���� ��ġ��
			{
				SrcFile.Format(_T("%s.%03d"), ctx->File.szNewName, i + 1);
				if(ctx->Global->nWorkType == 0)	// ��ȣȭ�� ���� �˻��� �����̸�
					lpH = ctx->File.hDst + i;
				else
					lpH = ctx->File.hSrc + i;
			}
			else
			{
				SrcFile = ctx->File.szFileName;
				lpH = ctx->File.hSrc;
			}
			if(*lpH == NULL)
			{
				nReopen = 3;
				while(nReopen > 0)	// �ȿ����� 3ȸ �ݺ� �õ�
				{
					if(ctx->Core.bWrite)
						*lpH = CreateFile(SrcFile, GENERIC_READ | GENERIC_WRITE, ctx->File.dwShareRight, NULL, OPEN_EXISTING, NULL, NULL);
					else
						*lpH = CreateFile(SrcFile, GENERIC_READ,  ctx->File.dwShareRight, NULL, OPEN_EXISTING, NULL, NULL);
					if(*lpH == INVALID_HANDLE_VALUE)
					{
						nReopen--;
						Sleep(100);
					}
					else
						break;
				}
			}
			ctx->Core.hSrc = *lpH;
			if(ctx->Core.hSrc == INVALID_HANDLE_VALUE)
			{
				m_dwErr = GetLastError();
				ctx->Core.Crcs.Free();
				CString warn;
				warn.Format(_T("�۾� ������ �� �� �����ϴ�.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			if(ctx->File.szFirstTargetName[0] == _T('\0'))
			{
				_tcscpy_s(ctx->File.szFirstTargetName, MAX_PATH, SrcFile);
			}
		}

		if((ctx->Core.nWorkType != 2) && (ctx->Core.hDst == NULL))	// && (!bAfterCheck || bChecked))	// üũ�� �ƴϰ� �ڵ��� NULL�̸� ��ȣȭ�� üũ�� ���̰ų� ��ȣȭ���̸�
		{
			// �������
			CString DstFile;
			HANDLE* lpH;
			if((ctx->Global->nWorkType == 0) && (ctx->Global->llSplitSize != 0LL))	// �����ϱ�
			{
				DstFile.Format(_T("%s.%03d"), ctx->File.szNewName, i + 1);
				lpH = ctx->File.hDst + i;
			}
			else
			{
				DstFile = ctx->File.szNewName;
				lpH = ctx->File.hDst;
			}
			if(!ctx->Core.bWrite && (*lpH == NULL))
			{
				nReopen = 3;
				while(nReopen > 0)	// �ȿ����� 3ȸ �ݺ� �õ�
				{
					*lpH = CreateFile(DstFile, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if(*lpH == INVALID_HANDLE_VALUE)
					{
						nReopen--;
						Sleep(100);
					}
					else
						break;
				}
				if(*lpH == INVALID_HANDLE_VALUE)
				{
					m_dwErr = GetLastError();
					ctx->Core.Crcs.Free();
					CString warn;
					warn.Format(_T("��� ������ ������ �� �����ϴ�.(%d)"), m_dwErr);
					_tcscpy_s(ctx->szError, MAX_PATH, warn);
					ctx->nResult = -1;
					return FALSE;
				}
			}
			ctx->Core.hDst = *lpH;
			if(ctx->Core.bWrite)	// ������ �ҽ��� ���� �ڵ�...
				ctx->Core.hDst = ctx->Core.hSrc;
			if(i == 0)	// ù��°�̸�
			{
				_tcscpy_s(ctx->File.szFirstTargetName, MAX_PATH, DstFile);
				if((ctx->Core.nMethod == 3) && ctx->Core.bWrite)	// Method3�̸鼭 ������̸�
				{
					// �����ϰ� ���ϸ� �����ÿ��� ��ȿ�� ����
					_tcscat_s(ctx->File.szFirstTargetName, MAX_PATH, ctx->File.szNewExt);
				}
			}
		}
		if(ctx->Core.nWorkType == 0)	// ��ȣȭ��
		{
			int nR;
#if 0	// ���� ������ �����ʴ´�.
			if(ctx->Core.nMethod == 1)
				nR = EncFile(ctx);
			else if(ctx->Core.nMethod == 2)
				nR = EncFile2(ctx);
			else
#endif
				nR = EncFile3(ctx);
			if(nR != 1)
			{
				ctx->Core.Crcs.Free();
				CString warn;
				warn.Format(_T("������ �����Ͽ����ϴ�.(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}
		else if(ctx->Core.nWorkType == 1)	// ��ȣȭ��
		{
			int nR;
			if(ctx->Core.nMethod == 3)
				nR = DecFile3(ctx, FALSE);
			else if(ctx->Core.nMethod == 2)
				nR = DecFile2(ctx, FALSE);
			else if(ctx->Core.nMethod != -1)
				nR = DecFile(ctx, FALSE);
			else
				nR = DecFileOld(ctx, FALSE);

			if(nR != 1)
			{
				ctx->Core.Crcs.Free();
				CString warn;
				warn.Format(_T("������ �����Ͽ����ϴ�.(�ջ�� �����̰ų� Ʋ�� ��й�ȣ)(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}
		else	// �˻��̸�
		{
			int nR;
			if(ctx->Core.nMethod == 3)
				nR = DecFile3(ctx, TRUE);
			else if(ctx->Core.nMethod == 2)
				nR = DecFile2(ctx, TRUE);
			else if(ctx->Core.nMethod != -1)
				nR = DecFile(ctx, TRUE);
			else
				nR = DecFileOld(ctx, TRUE);

			if(!ctx->Global->bForce && (nR != 1))	// ���� ������ �ƴϰ� ������ ���� ����
			{
				ctx->Core.Crcs.Free();
				CString warn;
				warn.Format(_T("�˻簡 �����Ͽ����ϴ�.(�ջ�� �����̰ų� Ʋ�� ��й�ȣ)(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}

		// �ڵ� ����
		if((ctx->Core.nWorkType == 0) && !ctx->Core.bJoin)	// ��ȣȭ��
		{
			if(ctx->Core.nFiles != i - 1)
			{
				llToMove.QuadPart = 0LL;
				SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
				ctx->Core.hDst = NULL;
			}
		}
		else if((ctx->Core.nWorkType != 0) && ctx->Core.bJoin)	// ��ȣȭ��
		{
			llToMove.QuadPart = 0LL;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
			ctx->Core.hSrc = NULL;
		}
		else if(ctx->Core.nWorkType != 0)
		{
//			CloseHandle(ctx->hSrc);
			llToMove.QuadPart = 0LL;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
			ctx->Core.hSrc = NULL;
		}
	}
	return TRUE;
}

#if 0
int CEncDec::EncFile(ENCDEC_CTX* ctx)
{
#if (JEJES_READONLY != 1)
	LARGE_INTEGER llToMove;
	LONGLONG remainSize = ctx->Core.llRemainSize;
	LONGLONG dstRemainSize;
	if(ctx->Global->llSplitSize == 0LL)	// �������� ���� ���
		dstRemainSize = remainSize;
	else
		dstRemainSize = ctx->Global->llSplitSize;
	ctx->Core.llFileProcessedSize = 0LL;

	int nPercent = 0;

//	int nInterval;	// ���͹�...
	BYTE* wBuf;	// ���� �۾��� �� ����� ����

	DWORD dwOrder, dwRead, dwWritten, dwBufSize, dwUnit;
	while((remainSize > 0) && (dstRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// ���� �ܰ迡�� BUFFER ũ�⿡ �̴��� ũ�⸦ ó���ߴٸ�
		dwOrder = ctx->Core.nReadSize - ctx->Core.nWrittenSize;
		if(dwOrder)	// ���� �ܰ迡�� ���� ũ�⸸ŭ ���� ���ߴٸ�
		{
			ctx->Core.nReadSize = dwOrder;
			dwBufSize = dwOrder;	// ��ó���� ũ��
			wBuf = ctx->Core.buf + ctx->Core.nWrittenSize;
			goto Write;
		}
		else
		{
			ctx->Core.nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			dwBufSize = ctx->Core.llBufferSize;
		}

		// ���� ũ�⿡ ���� �о�´�.(dstRemainSize�� ���� �Ͽ��� ���)
		if(remainSize < dwBufSize)
			dwOrder = remainSize;
		else
			dwOrder = dwBufSize;

		// ���� ��ġ�� ����/������ ��ġ�ΰ�? dwUnit == 1�̸� ����/���� �ʿ�
		dwUnit = ((ctx->Core.llProcessedSize + dwOrder)/ ctx->Core.llBufferSize) % ctx->Core.llInterval;
		if(ctx->Core.bWrite && (dwUnit != 1))	// ������̰� �ֱⰡ �ƴϸ� ���� ���� �ʿ䰡 ����.
		{
			DWORD dwSkip = ctx->Core.llBufferSize * (ctx->Core.llInterval - dwUnit + 1);	// ������ 1�̹Ƿ�
			if(dwSkip > remainSize)
				dwSkip = remainSize;

			llToMove.QuadPart = dwSkip;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
//			passedSize += dwSkip;
			remainSize -= dwSkip;
			ctx->Core.llProcessedSize += dwSkip;
			ctx->Core.llRemainSize -= dwSkip;
			ctx->Core.nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			dstRemainSize -= dwSkip;
			ctx->Core.llFileProcessedSize += dwSkip;
			continue;
		}
		ReadFile(ctx->Core.hSrc, ctx->Core.buf, dwOrder, &dwRead, NULL);
		if(dwOrder != dwRead)
		{
//			CloseHandle(ctx->hSrc);
			m_dwErr = GetLastError();
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			CString warn;
			warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(dwUnit == 1)	// �������� �ʿ��ϴ�.
		{
			if(ctx->Core.bWrite)	// ������̸� ���� ��ŭ �ǵ�����...
			{
				llToMove.QuadPart = lDistance;
				llToMove.QuadPart *= -1;
				SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
			}
			ctx->Core.Crcs.MemCrc32(ctx->Core.buf, lDistance, &ctx->Core.dwCRC);
#ifdef _DEBUG
			TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
			for(int i = 0; i < lDistance; i++)
				ctx->Core.IV[i] ^= ctx->Core.buf[i];
			wBuf = ctx->Core.IV;
		}
		else
			wBuf = ctx->Core.buf;
		// ���� ũ�Ⱑ ���� ���� ũ�⺸�� ���� ���
		if(ctx->Global->llSplitSize != 0LL)
		{
			if(ctx->Global->llSplitSize - ctx->Core.llFileProcessedSize < dwOrder)
				dwOrder = ctx->Global->llSplitSize - ctx->Core.llFileProcessedSize;
		}

Write:
		WriteFile(ctx->Core.hDst, wBuf, dwOrder, &dwWritten, NULL);
		if(dwOrder != dwWritten)
		{
//			CloseHandle(ctx->hSrc);
			m_dwErr = GetLastError();
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			CString warn;
			warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
//		passedSize += dwOrder;
		remainSize -= dwOrder;
		ctx->Core.llProcessedSize += dwOrder;
		ctx->Core.llRemainSize -= dwOrder;
		ctx->Core.nWrittenSize = dwOrder;
		ctx->Core.llFileProcessedSize += dwOrder;
		dstRemainSize -= dwOrder;
	}
#endif
	return 1;
}

int CEncDec::EncFile2(ENCDEC_CTX* ctx)
{
#if (JEJES_READONLY != 1)
	LONGLONG remainSize = ctx->Core.llRemainSize;
	ctx->Core.llFileProcessedSize = 0LL;
	LARGE_INTEGER llToMove;

	int nPercent = 0;

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize, dwUnit;
	if(!ctx->Core.bWrite)	// ����Ⱑ �ƴϸ� ���� ����
	{
		llToMove.QuadPart = 0LL;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
		while(remainSize > 0)
		{
			nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
			m_pStat->SetPos(nPercent);

			if(remainSize < ctx->Core.llBufferSize)
				dwOrder = remainSize;
			else
				dwOrder = ctx->Core.llBufferSize;
			ReadFile(ctx->Core.hSrc, ctx->Core.buf, dwOrder, &dwRead, NULL);
			if(dwOrder != dwRead)
			{
				m_dwErr = GetLastError();
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				CString warn;
				warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
			if(dwOrder != dwWritten)
			{
				m_dwErr = GetLastError();
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				CString warn;
				warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			remainSize -= dwOrder;
			ctx->Core.llProcessedSize += dwOrder;
			ctx->Core.llRemainSize -= dwOrder;
			ctx->Core.nWrittenSize = dwOrder;
			ctx->Core.llFileProcessedSize += dwOrder;
		}
	}

	// ��ó�� ��ġ ���� ����
	llToMove.QuadPart = 0LL;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
	if(Encrypt2(ctx) == FALSE)
	{
		ctx->Core.hSrc = NULL;
		ctx->Core.hDst = NULL;
		return 0;
	}
#ifdef _DEBUG
	TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif

	llToMove.QuadPart = ctx->Core.llBufferSize;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	if(Encrypt2(ctx) == FALSE)
	{
		ctx->Core.hSrc = NULL;
		ctx->Core.hDst = NULL;
	}
#ifdef _DEBUG
	TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif

#endif
	return 1;
}
#endif

int CEncDec::DecFile(ENCDEC_CTX* ctx, BOOL bCheck)
{
	LARGE_INTEGER FileSize;
	LARGE_INTEGER llToMove;
	GetFileSizeEx(ctx->Core.hSrc, &FileSize);
	LONGLONG remainSize = FileSize.QuadPart;
	LONGLONG srcRemainSize = (ctx->Core.llRemainSize - remainSize > 16) ? remainSize : ctx->Core.llRemainSize - 16;

	int nPercent = 0;

	int nInterval = -1;	// ���͹�...
//	BYTE* wBuf;	// ���� �۾��� �� ����� ����
	BYTE* rBuf;	// �б� �۾��� �� ����� ����

//	BOOL bFirst = FALSE;	// ���ʿ��� org�� ����ؾ� �ϹǷ�
	DWORD dwOrder, dwRead, dwWritten, dwBufSize;	//, dwUnit;

	// ������ ������
	while((remainSize > 0) && (srcRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// ���� �ܰ迡�� BUFFER ũ�� ���� ���� ũ�⸦ �о�Դٸ�
		dwOrder = ctx->Core.llBufferSize - ctx->Core.nReadSize;
		if(dwOrder)
		{
			if(srcRemainSize < dwOrder)
			{
				dwBufSize = srcRemainSize;
			}
			else
				dwBufSize = dwOrder;
			rBuf = ctx->Core.buf + ctx->Core.nReadSize;
		}
		else
		{
			ctx->Core.nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			dwBufSize = ctx->Core.llBufferSize;
			rBuf = ctx->Core.buf;
		}

		if(srcRemainSize < dwBufSize)
			dwOrder = srcRemainSize;
		else
			dwOrder = dwBufSize;

		// ���� ��ġ�� ������ ��ġ�ΰ�? dwUnit == 1�̸� ���� �ʿ�
		if((ctx->Core.llRemainSkipSize != ctx->Core.llDefaultSkipSize) && (ctx->Core.bWrite || bCheck))	// �����ų� �˻��ϱ�鼭 �ֱⰡ �ƴϸ� ���� ���� �ʿ䰡 ����.
		{
			DWORD dwSkip = ctx->Core.llRemainSkipSize;
			if(dwSkip > srcRemainSize)
				dwSkip = srcRemainSize;

			llToMove.QuadPart = dwSkip;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
//			passedSize += dwSkip;
			remainSize -= dwSkip;
			ctx->Core.llProcessedSize += dwSkip;
			ctx->Core.llRemainSize -= dwSkip;
//			ctx->nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			ctx->Core.llRemainSkipSize -= dwSkip;
			if(ctx->Core.llRemainSkipSize == 0LL)
				ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
			srcRemainSize -= dwSkip;
			ctx->Core.llFileProcessedSize += dwSkip;
			continue;
		}

		ReadFile(ctx->Core.hSrc, rBuf, dwOrder, &dwRead, NULL);
		if(dwOrder != dwRead)
		{
			m_dwErr = GetLastError();
#ifdef _DEBUG
			TRACE(_T("DecFile Read Error : %d"), m_dwErr);
#endif
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			CString warn;
			warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(ctx->Core.bIV1)	// ������ Ű�� ����� ���ۿ� ���� ���� ����
			memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
		else
			memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);

		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(ctx->Core.llRemainSkipSize == ctx->Core.llDefaultSkipSize)	// ������ �ʿ��ϴ�.
		{
			if(lDistance % ctx->Core.llBufferSize != 0)	// ����ũ�⸸ŭ ���� ������ ���߿�
			{
				remainSize -= dwRead;
				ctx->Core.llProcessedSize += dwRead;
				ctx->Core.llRemainSize -= dwRead;
				ctx->Core.nWrittenSize = dwRead;
				ctx->Core.llFileProcessedSize += dwRead;
				ctx->Core.llRemainSkipSize -= dwRead;
				if(ctx->Core.llRemainSkipSize == 0LL)
					ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
				srcRemainSize -= dwRead;
				continue;
			}
			if(!bCheck && ctx->Core.bWrite)	// ������̸� ���� ��ŭ �ǵ�����...
			{
				llToMove.QuadPart = lDistance;
				llToMove.QuadPart *= -1;
				SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
			}

			if(ctx->Core.bIV1)
				rBuf = ctx->Core.IV;
			else
				rBuf = ctx->Core.IV2;
			for(int i = 0; i < lDistance; i++)
				ctx->Core.buf[i] = ctx->Core.buf[i] ^ rBuf[i];
			ctx->Core.Crcs.MemCrc32(ctx->Core.buf, lDistance, &ctx->Core.dwCRC);
#ifdef _DEBUG
			TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
			ctx->Core.bIV1 = !ctx->Core.bIV1;
		}

		dwOrder = ctx->Core.nReadSize;
		if(!bCheck)
		{
			if((ctx->Core.nReadSize == ctx->Core.llBufferSize) || (ctx->Core.llRemainSize - 16 == dwRead))
			{
				WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
				if(dwOrder != dwWritten)
				{
					m_dwErr = GetLastError();
//					CloseHandle(ctx->hSrc);
					ctx->Core.hSrc = NULL;
					if(!ctx->Core.bWrite)
					{
//						CloseHandle(ctx->hDst);
						ctx->Core.hDst = NULL;
					}
					CString warn;
					warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
					_tcscpy_s(ctx->szError, MAX_PATH, warn);
					ctx->nResult = -1;
					return FALSE;
				}
			}
		}
//		passedSize += dwOrder;
		remainSize -= dwRead;
		ctx->Core.llProcessedSize += dwRead;
		ctx->Core.llRemainSize -= dwRead;
		ctx->Core.nWrittenSize = dwRead;
		ctx->Core.llFileProcessedSize += dwRead;
		ctx->Core.llRemainSkipSize -= dwRead;
		if(ctx->Core.llRemainSkipSize == 0LL)
			ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
		srcRemainSize -= dwRead;
	}

	if(ctx->Core.llRemainSize <= 16LL)
	{
		if(remainSize < 16LL)
			dwOrder = remainSize;	// ���� �ҽ� ����ũ��
		else
			dwOrder = 16;
		char* bufs = (char*)ctx->Core.szCrc;
		bufs += ctx->Core.nCrc;
		ReadFile(ctx->Core.hSrc, bufs, dwOrder, &dwRead, NULL);
		if(dwOrder != dwRead)
		{
			m_dwErr = GetLastError();
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			CString warn;
			warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(!bCheck && ctx->Core.bWrite)	// ������̸� ���� ����ó��
		{
			llToMove.QuadPart = dwOrder;
			llToMove.QuadPart *= -1;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
			SetEndOfFile(ctx->Core.hSrc);
		}
		ctx->Core.nCrc += dwOrder;

		if(ctx->Core.nCrc < 16)	// �� �о�� ���
		{
			if(ctx->Global->llSplitSize != 0)	// ���������̸� ���� ���Ϸ� ����
				return 1;
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			return 0;
		}

		// �� �о����� �� ����
		TCHAR szCrc[MAX_PATH];
		char szCrcA[MAX_PATH];
		_stprintf_s(szCrc, MAX_PATH, _T("%08X"), ctx->Core.dwCRC);
		ctx->Core.Crcs.Free();
		bufs = (char*)ctx->Core.szCrc;
		WideCharToMultiByte(CP_ACP, 0, szCrc, -1, szCrcA, MAX_PATH, NULL, NULL);
		if(strncmp(szCrcA, bufs, 8) == 0)
		{
			if(bufs[8] != 'L')
				return 0;

			// ���� ��� Ȯ��
			char szMethod[2] = {0,};
			szMethod[0] = bufs[9];
			int nMethod = atoi(szMethod);
			if(nMethod == ctx->Core.nMethod)
				return 1;
			else
				return 0;
		}
		else
			return 0;
	}
	return 1;
}

int CEncDec::DecFile2(ENCDEC_CTX* ctx, BOOL bCheck)
{
	LARGE_INTEGER FileSize;
	LARGE_INTEGER llToMove;
	GetFileSizeEx(ctx->Core.hSrc, &FileSize);
//	LONGLONG passedSize = 0LL;
	LONGLONG remainSize = FileSize.QuadPart;
	LONGLONG srcRemainSize = (ctx->Core.llRemainSize - remainSize > 16) ? remainSize : ctx->Core.llRemainSize - 16;

	int nPercent = 0;

//	BYTE* wBuf;	// ���� �۾��� �� ����� ����
//	BYTE* rBuf;	// �б� �۾��� �� ����� ����

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize;	//, dwUnit;
	if(!bCheck && !ctx->Core.bWrite)	// ����Ⱑ �ƴϰ� üũ�� �ƴϸ� ���Ϻ��� ����
	{
		llToMove.QuadPart = 0LL;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
		while(remainSize > 0)
		{
			nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
			m_pStat->SetPos(nPercent);

			if(remainSize < ctx->Core.llBufferSize)
				dwOrder = remainSize;
			else
				dwOrder = ctx->Core.llBufferSize;
			ReadFile(ctx->Core.hSrc, ctx->Core.buf, dwOrder, &dwRead, NULL);
			if(dwOrder != dwRead)
			{
				m_dwErr = GetLastError();
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				CString warn;
				warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
			if(dwOrder != dwWritten)
			{
				m_dwErr = GetLastError();
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				CString warn;
				warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			remainSize -= dwOrder;
			srcRemainSize -= dwOrder;
			ctx->Core.llProcessedSize += dwOrder;
			ctx->Core.llRemainSize -= dwOrder;
			ctx->Core.nWrittenSize = dwOrder;
			ctx->Core.llFileProcessedSize += dwOrder;
		}
	}
	else
		ctx->Core.hDst = ctx->Core.hSrc;

	// ��ó�� ��ġ ó��
	llToMove.QuadPart = 0LL;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
	if(Decrypt2(ctx, bCheck) == FALSE)
	{
		ctx->Core.hSrc = NULL;
		ctx->Core.hDst = NULL;
		return 0;
	}
#ifdef _DEBUG
	TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif

	llToMove.QuadPart = ctx->Core.llBufferSize + 16;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	if(Decrypt2(ctx, bCheck) == FALSE)
	{
		ctx->Core.hSrc = NULL;
		ctx->Core.hDst = NULL;
		return 0;
	}
#ifdef _DEBUG
	TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif

	llToMove.QuadPart = -16;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	char* bufs = (char*)ctx->Core.szCrc;
	ReadFile(ctx->Core.hDst, bufs, 16, &dwRead, NULL);
	if(dwRead != 16)
	{
		m_dwErr = GetLastError();
		ctx->Core.hSrc = NULL;
		ctx->Core.hDst = NULL;
		CString warn;
		warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		return 0;
	}

	if(!bCheck)	// üũ�� �ƴϸ� ���� ����ó��
	{
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
		SetEndOfFile(ctx->Core.hDst);
	}

	// �� �о����� �� ����
	TCHAR szCrc[MAX_PATH];
	char szCrcA[MAX_PATH];
	_stprintf_s(szCrc, MAX_PATH, _T("%08X"), ctx->Core.dwCRC);
	ctx->Core.Crcs.Free();
	bufs = (char*)ctx->Core.szCrc;
	WideCharToMultiByte(CP_ACP, 0, szCrc, -1, szCrcA, MAX_PATH, NULL, NULL);
	if(strncmp(szCrcA, bufs, 8) == 0)
	{
		if(bufs[8] != 'L')
			return 0;

		// ���� ��� Ȯ��
		char szMethod[2] = {0,};
		szMethod[0] = bufs[9];
		int nMethod = atoi(szMethod);
		if(nMethod == ctx->Core.nMethod)
			return 1;
		else
			return 0;
	}
	else
		return 0;
}

int CEncDec::DecFileOld(ENCDEC_CTX* ctx, BOOL bCheck)
{
	LARGE_INTEGER FileSize;
	LARGE_INTEGER llToMove;
	GetFileSizeEx(ctx->Core.hSrc, &FileSize);
	LONGLONG remainSize = FileSize.QuadPart;
	LONGLONG srcRemainSize = (ctx->Core.llRemainSize - remainSize > 16) ? remainSize : ctx->Core.llRemainSize - 16;

	int nPercent = 0;

	int nInterval = -1;	// ���͹�...
//	BYTE* wBuf;	// ���� �۾��� �� ����� ����
	BYTE* rBuf;	// �б� �۾��� �� ����� ����

//	BOOL bFirst = FALSE;	// ���ʿ��� org�� ����ؾ� �ϹǷ�
	DWORD dwOrder, dwRead, dwWritten, dwBufSize;	//, dwUnit;

	while((remainSize > 0) && (srcRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// ���� �ܰ迡�� BUFFER ũ�� ���� ���� ũ�⸦ �о�Դٸ�
		dwOrder = ctx->Core.llBufferSize - ctx->Core.nReadSize;
		if(dwOrder)
		{
			if(srcRemainSize < dwOrder)
			{
				dwBufSize = srcRemainSize;
			}
			else
				dwBufSize = dwOrder;
			rBuf = ctx->Core.buf + ctx->Core.nReadSize;
		}
		else
		{
			ctx->Core.nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			dwBufSize = ctx->Core.llBufferSize;
			rBuf = ctx->Core.buf;
		}

		if(srcRemainSize < dwBufSize)
			dwOrder = srcRemainSize;
		else
			dwOrder = dwBufSize;

		// ���� ��ġ�� ������ ��ġ�ΰ�? dwUnit == 1�̸� ���� �ʿ�
		if((ctx->Core.llRemainSkipSize != ctx->Core.llDefaultSkipSize) && (ctx->Core.bWrite || bCheck))	// �����ų� �˻��ϱ�鼭 �ֱⰡ �ƴϸ� ���� ���� �ʿ䰡 ����.
		{
			DWORD dwSkip = ctx->Core.llRemainSkipSize;
			if(dwSkip > srcRemainSize)
				dwSkip = srcRemainSize;

			llToMove.QuadPart = dwSkip;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
//			passedSize += dwSkip;
			remainSize -= dwSkip;
			ctx->Core.llProcessedSize += dwSkip;
			ctx->Core.llRemainSize -= dwSkip;
//			ctx->nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			ctx->Core.llRemainSkipSize -= dwSkip;
			if(ctx->Core.llRemainSkipSize == 0LL)
				ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
			srcRemainSize -= dwSkip;
			ctx->Core.llFileProcessedSize += dwSkip;
			continue;
		}

		ReadFile(ctx->Core.hSrc, rBuf, dwOrder, &dwRead, NULL);
		if(dwOrder != dwRead)
		{
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			AfxMessageBox(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�."));
			return 0;
		}
		if(ctx->Core.bIV1)	// ������ Ű�� ����� ���ۿ� ���� ���� ����
			memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
		else
			memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);

		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(ctx->Core.llRemainSkipSize == ctx->Core.llDefaultSkipSize)	// ������ �ʿ��ϴ�.
		{
			if(lDistance % ctx->Core.llBufferSize != 0)	// ����ũ�⸸ŭ ���� ������ ���߿�
			{
				remainSize -= dwRead;
				ctx->Core.llProcessedSize += dwRead;
				ctx->Core.llRemainSize -= dwRead;
				ctx->Core.nWrittenSize = dwRead;
				ctx->Core.llFileProcessedSize += dwRead;
				ctx->Core.llRemainSkipSize -= dwRead;
				if(ctx->Core.llRemainSkipSize == 0LL)
					ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
				srcRemainSize -= dwRead;
				continue;
			}
			if(!bCheck && ctx->Core.bWrite)	// ������̸� ���� ��ŭ �ǵ�����...
			{
				llToMove.QuadPart = lDistance;
				llToMove.QuadPart *= -1;
				SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
			}

			if(ctx->Core.bIV1)
				rBuf = ctx->Core.IV;
			else
				rBuf = ctx->Core.IV2;
			for(int i = 0; i < lDistance; i++)
				ctx->Core.buf[i] = ctx->Core.buf[i] ^ rBuf[i];
			ctx->Core.Crcs.MemCrc32(ctx->Core.buf, lDistance, &ctx->Core.dwCRC);
#ifdef _DEBUG
			TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
			ctx->Core.bIV1 = !ctx->Core.bIV1;
		}

		dwOrder = ctx->Core.nReadSize;
		if(!bCheck)
		{
			if((ctx->Core.nReadSize == ctx->Core.llBufferSize) || (ctx->Core.llRemainSize - 16 == dwRead))
			{
				WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
				if(dwOrder != dwWritten)
				{
					m_dwErr = GetLastError();
//					CloseHandle(ctx->hSrc);
					ctx->Core.hSrc = NULL;
					if(!ctx->Core.bWrite)
					{
//						CloseHandle(ctx->hDst);
						ctx->Core.hDst = NULL;
					}
					CString warn;
					warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
					_tcscpy_s(ctx->szError, MAX_PATH, warn);
					ctx->nResult = -1;
					return FALSE;
				}
			}
		}
//		passedSize += dwOrder;
		remainSize -= dwRead;
		ctx->Core.llProcessedSize += dwRead;
		ctx->Core.llRemainSize -= dwRead;
		ctx->Core.nWrittenSize = dwRead;
		ctx->Core.llFileProcessedSize += dwRead;
		ctx->Core.llRemainSkipSize -= dwRead;
		if(ctx->Core.llRemainSkipSize == 0LL)
			ctx->Core.llRemainSkipSize = ctx->Core.llDefaultSkipSize;
		srcRemainSize -= dwRead;
	}

	if(ctx->Core.llRemainSize <= 16LL)
	{
		if(remainSize < 16LL)
			dwOrder = remainSize;	// ���� �ҽ� ����ũ��
		else
			dwOrder = 16;
		char* bufs = (char*)ctx->Core.szCrc;
		bufs += ctx->Core.nCrc;
		ReadFile(ctx->Core.hSrc, bufs, dwOrder, &dwRead, NULL);
		if(dwOrder != dwRead)
		{
			m_dwErr = GetLastError();
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			CString warn;
			warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(!bCheck && ctx->Core.bWrite)	// ������̸� ���� ����ó��
		{
			llToMove.QuadPart = dwOrder;
			llToMove.QuadPart *= -1;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
			SetEndOfFile(ctx->Core.hSrc);
		}
		ctx->Core.nCrc += dwOrder;

		if(ctx->Core.nCrc < 16)	// �� �о�� ���
		{
			if(ctx->Global->llSplitSize != 0)	// ���������̸� ���� ���Ϸ� ����
				return 1;
//			CloseHandle(ctx->hSrc);
			ctx->Core.hSrc = NULL;
			if(!ctx->Core.bWrite)
			{
//				CloseHandle(ctx->hDst);
				ctx->Core.hDst = NULL;
			}
			return 0;
		}

		// �� �о����� �� ����
		TCHAR szCrc[MAX_PATH];
		_stprintf_s(szCrc, MAX_PATH, _T("%08X"), ctx->Core.dwCRC);
		ctx->Core.Crcs.Free();
		if(_tcsncmp(szCrc, ctx->Core.szCrc, 8) == 0)
			return 1;
		else
			return 0;
	}
	return 1;
}

BOOL CEncDec::Encrypt2(ENCDEC_CTX *ctx)
{
#if (JEJES_READONLY != 1)
	if(ctx->Core.hDst == NULL)
		return 0;
	LARGE_INTEGER llToMove;
	DWORD dwOrder, dwRead, dwWritten;
	dwOrder = BUFFER_SIZE_METHOD2;
	ReadFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwRead, NULL);
	if(dwOrder != dwRead)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
	llToMove.QuadPart = dwOrder;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
	ctx->Core.Crcs.MemCrc32(ctx->Core.buf, dwOrder, &ctx->Core.dwCRC);
	for(int i = 0; i < dwOrder; i++)
	{
		ctx->Core.IV[i] ^= ctx->Core.buf[i];
		ctx->Core.IV_Cmp[i] ^= ctx->Core.buf[i];
	}
	if(memcmp(ctx->Core.IV, ctx->Core.IV_Cmp, dwOrder) != 0)	// �� ���� ��ġ���� ������ �޸� ����
	{
		_tcscpy_s(ctx->szError, MAX_PATH, _T("������ �����Ͽ����ϴ�.(�ٸ� ��)"));
		ctx->nResult = -1;
		return FALSE;
	}

	WriteFile(ctx->Core.hDst, ctx->Core.IV, dwOrder, &dwWritten, NULL);
	if(dwOrder != dwWritten)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
#endif
	return 1;
}

BOOL CEncDec::Decrypt2(ENCDEC_CTX *ctx, BOOL bCheck)
{
	if(ctx->Core.hDst == NULL)
		return 0;
	LARGE_INTEGER llToMove;
	DWORD dwOrder, dwRead, dwWritten;
	dwOrder = BUFFER_SIZE_METHOD2;
	ReadFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwRead, NULL);
	if(dwOrder != dwRead)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
	if(ctx->Core.bIV1)	// ������ Ű�� ����� ���ۿ� ���� ���� ����
		memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
	else
		memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
	llToMove.QuadPart = dwOrder;
	llToMove.QuadPart *= -1;
	if(!bCheck)
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
	BYTE * rBuf;
	if(ctx->Core.bIV1)
		rBuf = ctx->Core.IV;
	else
		rBuf = ctx->Core.IV2;
	for(int i = 0; i < dwOrder; i++)
		ctx->Core.buf[i] = ctx->Core.buf[i] ^ rBuf[i];
	ctx->Core.Crcs.MemCrc32(ctx->Core.buf, dwOrder, &ctx->Core.dwCRC);
	ctx->Core.bIV1 = !ctx->Core.bIV1;

	if(bCheck)
		return 1;
	WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
	if(dwOrder != dwWritten)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
	return 1;
}

BOOL CEncDec::EncFile3(ENCDEC_CTX *ctx)
{
#if (JEJES_READONLY != 1)
	LONGLONG remainSize = ctx->Core.llRemainSize;
	ctx->Core.llFileProcessedSize = 0LL;
	LARGE_INTEGER llToMove;

	// ctx ������Ʈ
	ctx->Core.nCount = GetMethod3Multiple(ctx, TRUE);
	
	ULONGLONG llTotal;
	if(ctx->Core.bWrite)
		llTotal = ctx->Core.nCount * 4;
	else
		llTotal = ctx->Core.llTotalSize + (ctx->Core.nCount * ctx->Core.llBufferSize * 4);	// �Ǿ� �ǵ� �а� ����;

	int nPercent = 0;

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize, dwUnit;
	if(!ctx->Core.bWrite)	// ����Ⱑ �ƴϸ� ���� ����
	{
		llToMove.QuadPart = 0LL;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
		while(remainSize > 0)
		{
			nPercent = ctx->Core.llProcessedSize * 100 / llTotal;
			m_pStat->SetPos(nPercent);

			if(remainSize < ctx->Core.llBufferSize)
				dwOrder = remainSize;
			else
				dwOrder = ctx->Core.llBufferSize;
			ReadFile(ctx->Core.hSrc, ctx->Core.buf, dwOrder, &dwRead, NULL);
			if(dwOrder != dwRead)
			{
				m_dwErr = GetLastError();
				CString warn;
				warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
			if(dwOrder != dwWritten)
			{
				m_dwErr = GetLastError();
				CString warn;
				warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			remainSize -= dwOrder;
			ctx->Core.llProcessedSize += dwOrder;
			ctx->Core.llRemainSize -= dwOrder;
			ctx->Core.nWrittenSize = dwOrder;
			ctx->Core.llFileProcessedSize += dwOrder;
		}
	}

	// ��ó�� ��ġ ���� ����(����ũ���� 5%��ŭ�� ����(BUFFER_SIZE_METHOD2�� �����)
	llToMove.QuadPart = 0LL;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
	for(int i = 0; i < ctx->Core.nCount; i++)
	{
		if(Encrypt2(ctx) == FALSE)
		{
			ctx->Core.hSrc = NULL;
			ctx->Core.hDst = NULL;
			return 0;
		}
#ifdef _DEBUG
		TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
#ifdef DEEP_DEBUG
		{
			CString tmp;
			tmp.Format(_T("Encrypting(%X)\n"), ctx->Core.dwCRC);
			m_strDbg += tmp;
		}
#endif
		if(ctx->Core.bWrite)	// ������̸�
			nPercent = (i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// �Ǹ����� ��ġ ���� ����(����ũ���� 5%��ŭ�� ����(BUFFER_SIZE_METHOD2�� �����)
	llToMove.QuadPart = ctx->Core.llBufferSize * ctx->Core.nCount;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	for(int i = 0; i < ctx->Core.nCount; i++)
	{
		if(Encrypt2(ctx) == FALSE)
		{
			ctx->Core.hSrc = NULL;
			ctx->Core.hDst = NULL;
		}
#ifdef _DEBUG
		TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
#ifdef DEEP_DEBUG
		{
			CString tmp;
			tmp.Format(_T("Encrypting(%X)\n"), ctx->Core.dwCRC);
			m_strDbg += tmp;
		}
#endif
		if(ctx->Core.bWrite)	// ������̸�
			nPercent = (ctx->Core.nCount + i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (ctx->Core.nCount + i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// ���ϸ� �߰�, �߰����ϸ� ����, CRC �� ���� �߰�
	if(AccessTail(ctx, FALSE, TRUE, FALSE) <= 0)
		return 0;
	// ������ ���ϸ��� ����, �ƴϸ� ������ϸ� �����ÿ� �̸� ����
	if(ctx->Core.bWrite)	// ������ ���ϸ� ����
	{
		TCHAR szNewName[MAX_PATH];
		_tcscpy_s(szNewName, MAX_PATH, ctx->File.szNewName);
		_tcscat_s(szNewName, MAX_PATH, ctx->File.szNewExt);
		FlushFileBuffers(ctx->File.hSrc[0]);
		CloseHandle(ctx->File.hSrc[0]);	// ������ �����Ƿ� 1�� �ۿ� ����.
		ctx->File.hSrc[0] = NULL;
		BOOL bRet = MoveFile(ctx->File.szFileName, szNewName);
		if(!bRet)
			return 0;
	}
#endif
	return TRUE;
}

BOOL CEncDec::DecFile3(ENCDEC_CTX *ctx, BOOL bCheck)
{
	LARGE_INTEGER FileSize;
	LARGE_INTEGER llToMove;
	GetFileSizeEx(ctx->Core.hSrc, &FileSize);
//	LONGLONG passedSize = 0LL;
	LONGLONG remainSize = FileSize.QuadPart;
	int nTailLen = AccessTail(ctx, TRUE, FALSE, FALSE);
//	BOOL bReturn;

	// ctx ������Ʈ
	ctx->Core.nCount = GetMethod3Multiple(ctx, FALSE);

	ULONGLONG llTotal;
	if(bCheck || ctx->Core.bWrite)	// üũ�ų� ������
		llTotal = ctx->Core.nCount * 4;
	else
		llTotal = ctx->Core.llTotalSize + (ctx->Core.nCount * ctx->Core.llBufferSize * 4);	// �Ǿ� �ǵ� �а� ����

	int nPercent = 0;

//	BYTE* wBuf;	// ���� �۾��� �� ����� ����
//	BYTE* rBuf;	// �б� �۾��� �� ����� ����

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize;	//, dwUnit;
	if(!bCheck && !ctx->Core.bWrite)	// ����Ⱑ �ƴϰ� üũ�� �ƴϸ� ���Ϻ��� ����
	{
		llToMove.QuadPart = 0LL;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_BEGIN);
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
		while(remainSize > 0)
		{
			nPercent = ctx->Core.llProcessedSize * 100 / llTotal;
			m_pStat->SetPos(nPercent);

			if(remainSize < ctx->Core.llBufferSize)
				dwOrder = remainSize;
			else
				dwOrder = ctx->Core.llBufferSize;
			ReadFile(ctx->Core.hSrc, ctx->Core.buf, dwOrder, &dwRead, NULL);
			if(dwOrder != dwRead)
			{
				m_dwErr = GetLastError();
				CString warn;
				warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
			if(dwOrder != dwWritten)
			{
				m_dwErr = GetLastError();
				CString warn;
				warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
				ctx->Core.hSrc = NULL;
				ctx->Core.hDst = NULL;
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			remainSize -= dwOrder;
			ctx->Core.llProcessedSize += dwOrder;
			ctx->Core.llRemainSize -= dwOrder;
			ctx->Core.nWrittenSize = dwOrder;
			ctx->Core.llFileProcessedSize += dwOrder;
		}
	}
	else
		ctx->Core.hDst = ctx->Core.hSrc;

	// ��ó�� ��ġ ó��
	llToMove.QuadPart = 0LL;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
	for(int i = 0; i < ctx->Core.nCount; i++)
	{
		if(Decrypt2(ctx, bCheck) == FALSE)
		{
			ctx->Core.hSrc = NULL;
			ctx->Core.hDst = NULL;
			return 0;
		}
#ifdef _DEBUG
		TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
#ifdef DEEP_DEBUG
		{
			CString tmp;
			tmp.Format(_T("Decrypting(%X)\n"), ctx->Core.dwCRC);
			m_strDbg += tmp;
		}
#endif
		if(bCheck || ctx->Core.bWrite)	// üũ�ų� ������
			nPercent = (i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// �Ǹ����� �κ� ó��
	llToMove.QuadPart = (ctx->Core.llBufferSize * ctx->Core.nCount) + nTailLen;
	llToMove.QuadPart *= -1;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	for(int i = 0; i < ctx->Core.nCount; i++)
	{
		if(Decrypt2(ctx, bCheck) == FALSE)
		{
			ctx->Core.hSrc = NULL;
			ctx->Core.hDst = NULL;
			return 0;
		}
#ifdef _DEBUG
		TRACE(_T("%X\n"), ctx->Core.dwCRC);
#endif
#ifdef DEEP_DEBUG
		{
			CString tmp;
			tmp.Format(_T("Decrypting(%X)\n"), ctx->Core.dwCRC);
			m_strDbg += tmp;
		}
#endif
		if(bCheck || ctx->Core.bWrite)	// üũ�ų� ������
			nPercent = (ctx->Core.nCount + i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (ctx->Core.nCount + i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// üũ��
	if(bCheck)
	{
		if(AccessTail(ctx, FALSE, FALSE, FALSE) <= 0)
			return 0;
		else
			return 1;
	}

	// üũ�� �ƴϸ� ���� ����ó��
//	llToMove.QuadPart = 0;
//	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	SetEndOfFile(ctx->Core.hDst);

	// ������ ���ϸ��� ����, �ƴϸ� ������ϸ� �����ÿ� �̸� ����
	if(ctx->Core.bWrite)
	{
		TCHAR szNewName[MAX_PATH];
		_tcscpy_s(szNewName, MAX_PATH, ctx->File.szFileName);
		if(_tcsicmp((szNewName + _tcslen(szNewName) - _tcslen(ctx->Global->szExt)), ctx->Global->szExt) == 0)
			szNewName[_tcslen(szNewName) - _tcslen(ctx->Global->szExt)] = _T('\0');
		FlushFileBuffers(ctx->File.hSrc[0]);
		CloseHandle(ctx->File.hSrc[0]);	// ������ �����Ƿ� 1�� �ۿ� ����.
		ctx->File.hSrc[0] = NULL;
		BOOL bRet = MoveFile(ctx->File.szFileName, szNewName);
		if(!bRet)
			return FALSE;
		else
			return TRUE;
	}
	return TRUE;
}

BOOL CEncDec::DoJob(ENCDEC_CTX *ctx)
{
	InitLoopDefaults(ctx);
	BOOL bRet = FileLoop(ctx);
	return bRet;
}

BOOL CEncDec::CheckEncryptedFile(ENCDEC_CTX *ctx)
{
	InitLoopDefaults(ctx);
	BOOL bRet = FileLoop(ctx);
//	InitHandle(ctx, TRUE, FALSE);
	return bRet;
}

BOOL CEncDec::EncryptFile(ENCDEC_CTX *ctx)
{
	// ��ȣȭ
	InitLoopDefaults(ctx);
	BOOL bRet = FileLoop(ctx);
//	InitHandle(ctx, TRUE, FALSE);
	return bRet;
}

BOOL CEncDec::DecryptFile(ENCDEC_CTX *ctx)
{
	// ��ȣȭ
	InitLoopDefaults(ctx);
	BOOL bRet = FileLoop(ctx);
//	InitHandle(ctx, TRUE, FALSE);
	return bRet;
}

void CEncDec::InitHandle(ENCDEC_CTX *ctx, BOOL bClose, BOOL bDelete)
{
	HANDLE* hSrc = ctx->File.hSrc;
	HANDLE *hDst = ctx->File.hDst;
	if(hSrc != NULL)
	{
		for(int i = 0; i < ctx->Core.nFiles; i++)
		{
			if(bClose)
			{
				if((hSrc[i] != NULL) && (hSrc[i] != INVALID_HANDLE_VALUE))
				{
					FlushFileBuffers(hSrc[i]);
					CloseHandle(hSrc[i]);
				}
			}
			hSrc[i] = NULL;
		}
		if(bDelete)
		{
			delete [] ctx->File.hSrc;
			ctx->File.hSrc = NULL;
		}
	}
	if(hDst != NULL)
	{
		for(int i = 0; i < ctx->Core.nFiles; i++)
		{
			if(bClose)
			{
				if((hDst[i] != NULL) && (hDst[i] != INVALID_HANDLE_VALUE))
				{
					FlushFileBuffers(hDst[i]);
					CloseHandle(hDst[i]);
				}
			}
			hDst[i] = NULL;
		}
		if(bDelete)
		{
			delete [] ctx->File.hDst;
			ctx->File.hDst = NULL;
		}
	}
}

TCHAR* CEncDec::GetFirstTargetFile(ENCDEC_CTX *ctx)
{
	return ctx->File.szFirstTargetName;
}

/*
 * Method3���� ����� BUFFER_SIZE_METHOD2��ŭ ����/�����ؾ� �ϴ����� ����(5%?)
*/
int CEncDec::GetMethod3Multiple(ENCDEC_CTX* ctx, BOOL bEncode)
{
	LARGE_INTEGER llSize;
	GetFileSizeEx(ctx->Core.hSrc, &llSize);
	if(!bEncode)
	{
		int nTailSize = AccessTail(ctx, TRUE, FALSE, FALSE);
		llSize.QuadPart -= nTailSize;
	}

	int nCount = 0;
	llSize.QuadPart *= 0.01;
	if(llSize.QuadPart > BUFFER_SIZE_METHOD2)
	{
		nCount = llSize.QuadPart / BUFFER_SIZE_METHOD2;
	}
	else
		nCount = 1;	// �ּ� 1ȸ...
	return nCount;
}

/*
 * Tail�κ��� ���ų� �д´�. bGetLength�� ���̸� ���� ���ڵ��� ���õǸ� Tail�� ���̸� ���ϵȴ�.
 * �д� ���(!bWrite) bUpdate�� ���̸� ctx�� ������Ʈ �����̸� �� ����� �����Ѵ�.
*/
int CEncDec::AccessTail(ENCDEC_CTX *ctx, BOOL bGetLength, BOOL bWrite, BOOL bUpdate)
{
	BYTE buf[32];
//	BYTE *Name;
	char szBuf[16];
	char szCrcA[MAX_PATH];
	DWORD dwResult;
	LARGE_INTEGER llToMove;
	short nNameLen = 0;
	int nCount;

	if(bGetLength || !bWrite)	// �о�´�.
	{
		llToMove.QuadPart = -32;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_END);
		ReadFile(ctx->Core.hSrc, buf, sizeof(buf), &dwResult, NULL);
		if(sizeof(buf) != dwResult)
			return -2;	// ���� �׼��� ����
		if(buf[16 + 8] != 'L')
			return -1;	// �߸��� ����
		if(buf[0] != 0)	// ���ϸ� ������ �Ǿ�����
		{
			nNameLen = (short)(buf + 1);
			if((nNameLen <= 0) || (nNameLen > MAX_PATH * 2))
				return -1;	// �߸��� ����
		}
		if(bGetLength)
			return 32 + nNameLen;
		nCount = buf[8];
		if(nCount <= 0)	// �ݵ�� 1 �̻�
			return -1;	// �߸��� ����
		if(!bUpdate)
		{
			if(ctx->Core.nCount != nCount)
				return 0;	// ��ġ ����
		}
		else
			ctx->Core.nCount = nCount;

		// CRC�� ��/������Ʈ
		::ZeroMemory(szBuf, sizeof(szBuf));
		memcpy_s(szBuf, sizeof(szBuf), (buf + 16), 8);
		if(!bUpdate)	// ���̸�
		{
			sprintf_s(szCrcA, MAX_PATH, "%08X", ctx->Core.dwCRC);
			if(strncmp(szBuf, szCrcA, 8) != 0)
				return 0;	// ��ġ ����
		}
		else
		{
			strncpy_s((char*)ctx->Core.szCrc, sizeof(ctx->Core.szCrc), szBuf, 8);
		}

		// Method ������Ʈ
		char szMethod[2] = {0,};
		szMethod[0] = buf[16 + 9];
		int nMethod = atoi(szMethod);
		if(!bUpdate)	// ���̸�
		{
			if(ctx->Core.nMethod != nMethod)
				return 0;	// ��ġ����
		}
		else
			ctx->Core.nMethod = nMethod;

		// ���ϸ� ��(����� ���ϸ� ������ �����Ƿ�...)
		if(nNameLen > 0)
		{
			if(!bUpdate)
				return 0;
			else
			{
//				�о�ͼ� �����ϴ� ���� ���߿� ��������...
//				_tcscpy_s(ctx->Core.szOrgName, sizeof(ctx->Core.szOrgName), ...
			}
		}

		// ������� ������ �񱳰� �����߰ų� ������Ʈ�� �Ϸ�� ����.
		return 1;
	}

	// ������ʹ� ����...
	// ���ϸ� ������ ��� ���ϸ� �����ϴ� ���� ���߿� �����Ѵ�...
	::ZeroMemory(buf, sizeof(buf));
	::ZeroMemory(szCrcA, sizeof(szCrcA));
	buf[0] = 0;	// ���ϸ� ���� ����
	buf[8] = ctx->Core.nCount;
	sprintf_s(szCrcA, MAX_PATH, "%08XL%d", ctx->Core.dwCRC, 3);
	strcat_s(szCrcA, MAX_PATH, "000009");
	memcpy_s(buf + 16, 16, szCrcA, 16);
	llToMove.QuadPart = 0;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	WriteFile(ctx->Core.hDst, buf, 32, &dwResult, NULL);
	if(dwResult != 32)
		return -2;	// ���� �׼��� ����
	return 32;	// �� ����Ʈ ��
}

void CEncDec::UpdateResultString(ENCDEC_CTX *ctx, int nWorkType)
{
	// ���������� ����� ��� ������ڿ� ������Ʈ
	TCHAR szBuf[MAX_PATH];
	if(nWorkType == 1)	// �����̸�
	{
		_tcscpy_s(ctx->szError, MAX_PATH, _T("���� �Ϸ�"));
		return;
	}

	if(nWorkType == 0)
		_tcscpy_s(szBuf, MAX_PATH, _T("���� �Ϸ� (������� : "));
	else
		_tcscpy_s(szBuf, MAX_PATH, _T("�˻� �Ϸ� (������� : "));

	switch(ctx->Core.nMethod)
	{
	case -1:	// UNDER5
		_tcscat_s(szBuf, MAX_PATH, _T("1.0.0.4����)"));
		break;
	case 0:	// OVER5
		_tcscat_s(szBuf, MAX_PATH, _T("1.0.0.5)"));
		break;
	case 1:	// METHOD1
		_tcscat_s(szBuf, MAX_PATH, _T("���1)"));
		break;
	case 2:	// METHOD2
		_tcscat_s(szBuf, MAX_PATH, _T("���2)"));
		break;
	case 3:	// METHOD3
		_tcscat_s(szBuf, MAX_PATH, _T("���3)"));
		break;
	}
	_tcscpy_s(ctx->szError, _countof(ctx->szError), szBuf);
}

//BOOL CEncDec::Encrypt3(ENCDEC_CTX *ctx)
//{
//#if (JEJES_READONLY != 1)
//	if(ctx->Core.hDst == NULL)
//		return 0;
//	LARGE_INTEGER llToMove;
//	DWORD dwOrder, dwRead, dwWritten;
//	dwOrder = BUFFER_SIZE_METHOD2;
//	ReadFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwRead, NULL);
//	if(dwOrder != dwRead)
//	{
//		m_dwErr = GetLastError();
//		CString warn;
//		warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//	llToMove.QuadPart = dwOrder;
//	llToMove.QuadPart *= -1;
//	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
//#ifdef _DEBUG
//	{
//		CCrc32 c;
//		DWORD dwCrc = 0xFFFFFFFF;
//		c.Init();
//		c.MemCrc32(ctx->Core.buf, dwOrder, &dwCrc);
//		TRACE(_T("Block Hash is %X(%d)\n"), dwCrc, dwOrder);
//	}
//#endif
//	ctx->Core.Crcs.MemCrc32(ctx->Core.buf, dwOrder, &ctx->Core.dwCRC);
//	BYTE *rBuf, *rBuf2;
//	if(ctx->Core.bIV1)
//	{
//		rBuf = ctx->Core.IV;
//		rBuf2 = ctx->Core.IV2;
//	}
//	else
//	{
//		rBuf = ctx->Core.IV2;
//		rBuf2 = ctx->Core.IV;
//	}
//	for(int i = 0; i < dwOrder; i++)
//		rBuf2[i] = ctx->Core.buf[i] ^ rBuf[i];
//
//	ctx->Core.bIV1 = !ctx->Core.bIV1;
//
//	WriteFile(ctx->Core.hDst, rBuf2, dwOrder, &dwWritten, NULL);
//	if(dwOrder != dwWritten)
//	{
//		m_dwErr = GetLastError();
//		CString warn;
//		warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//#endif
//	return 1;
//}
//
//BOOL CEncDec::Decrypt3(ENCDEC_CTX *ctx, BOOL bCheck)
//{
//	if(ctx->Core.hDst == NULL)
//		return 0;
//	LARGE_INTEGER llToMove;
//	DWORD dwOrder, dwRead, dwWritten;
//	dwOrder = BUFFER_SIZE_METHOD2;
//	ReadFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwRead, NULL);
//	if(dwOrder != dwRead)
//	{
//		m_dwErr = GetLastError();
//		CString warn;
//		warn.Format(_T("�۾� ������ �о���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//	if(ctx->Core.bIV1)	// ������ Ű�� ����� ���ۿ� ���� ���� ����
//		memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
//	else
//		memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
//	llToMove.QuadPart = dwOrder;
//	llToMove.QuadPart *= -1;
//	if(!bCheck)
//		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_CURRENT);
//	BYTE * rBuf;
//	if(ctx->Core.bIV1)
//		rBuf = ctx->Core.IV;
//	else
//		rBuf = ctx->Core.IV2;
//	for(int i = 0; i < dwOrder; i++)
//		ctx->Core.buf[i] = ctx->Core.buf[i] ^ rBuf[i];
//#ifdef _DEBUG
//	{
//		CCrc32 c;
//		DWORD dwCrc = 0xFFFFFFFF;
//		c.Init();
//		c.MemCrc32(ctx->Core.buf, dwOrder, &dwCrc);
//		TRACE(_T("Block Hash is %X(%d)\n"), dwCrc, dwOrder);
//	}
//#endif
//	ctx->Core.Crcs.MemCrc32(ctx->Core.buf, dwOrder, &ctx->Core.dwCRC);
//	ctx->Core.bIV1 = !ctx->Core.bIV1;
//
//	if(bCheck)
//		return 1;
//	WriteFile(ctx->Core.hDst, ctx->Core.buf, dwOrder, &dwWritten, NULL);
//	if(dwOrder != dwWritten)
//	{
//		m_dwErr = GetLastError();
//		CString warn;
//		warn.Format(_T("��� ������ ���� �� �����Ͽ����ϴ�.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//	return 1;
//}