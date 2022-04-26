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

BOOL CEncDec::CheckValid(ENCDEC_CTX* ctx)	// 파일 유효성 검사
{
	// 파일이 존재하는지 확인
	DWORD dwAtt = GetFileAttributes(ctx->File.szFileName);
	if((dwAtt == INVALID_FILE_ATTRIBUTES) || (dwAtt & FILE_ATTRIBUTE_DIRECTORY))
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("작업파일이 없거나 파일이 아닙니다.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}

	if(!CheckFileSize(ctx))
		return FALSE;

	ctx->Core.bSamePath = ctx->Global->bSamePath;
	ctx->Core.bWrite = ctx->Global->bWrite;
	ctx->Core.bJoin = ctx->Global->bJoin;
	if(ctx->Core.bJoin == TRUE)	// 분할 합치기면
	{
		if(!CheckJoinFile(ctx))
			return FALSE;
	}

	if(ctx->Core.bJoin == FALSE)
	{
		if(ctx->Global->llSplitSize == 0LL)	// 분할않음이면
			ctx->Core.nFiles = 1;
		else
		{
			if(ctx->Core.llTotalSize < ctx->Global->llSplitSize)
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("분할크기가 너무 큽니다. 분할크기는 원본크기보다 클 수 없습니다."));
				ctx->nResult = -1;
				return FALSE;
			}
			ctx->Core.nFiles = ctx->Core.llTotalSize / ctx->Global->llSplitSize + 1;
			if(ctx->Core.nFiles > 999)
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("분할크기가 너무 작습니다. 분할파일은 1~999까지입니다."));
				ctx->nResult = -1;
				return FALSE;
			}
		}
	}

	// 버전 확인
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
			if(ctx->Core.nFiles != 1)	// 방법2,3은 분할을 지원하지 않으므로
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("손상된 파일입니다."));
				ctx->nResult = -1;
				return FALSE;
			}
			break;
		case 9:
			_tcscpy_s(ctx->szError, MAX_PATH, _T("지원하지 않는 버전으로 변조된 파일입니다. 상위버전으로 복원해보세요."));
			ctx->nResult = -1;
			return FALSE;
			break;
		default:
			_tcscpy_s(ctx->szError, MAX_PATH, _T("변조된 파일이 아니거나 손상된 파일입니다."));
			ctx->nResult = -1;
			return FALSE;
			break;
		}
	}
	else
		ctx->Core.nMethod = ctx->Global->nMethod;

	// 출력경로 및 파일 확인
	if(!CheckTargetFile(ctx))
		return FALSE;

	return TRUE;
}

BOOL CEncDec::CheckFileSize(ENCDEC_CTX *ctx)
{
	// 원본파일크기 확인 및 변조시 파일최소크기 확인
	HANDLE hFind;
	WIN32_FIND_DATA fFile;
	hFind = FindFirstFile(ctx->File.szFileName, &fFile);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("작업파일 정보를 확인할 수 없습니다.(%d)"), m_dwErr);
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
		_tcscpy_s(ctx->szError, MAX_PATH, _T("작업파일이 너무 작습니다."));
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
	// 동일한 크기인지, 파일들이 다 있는지 확인
	int nErr = 0;
	int nNo = 1;	// 확장자명용
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
			if(nErr <= 1)	// 정상
			{
				File.Format(_T("%s.%03d"), FileName, nNo + 1);
				DWORD dwAtt = GetFileAttributes(File);
				if(dwAtt == INVALID_FILE_ATTRIBUTES)
				{
					ctx->Core.nFiles = nNo -1;
					if(ctx->Core.nFiles < 1)
					{
						ctx->Core.bJoin = FALSE;	// 파일 하나이면 Join이 아님
						ctx->Core.nFiles = 1;
					}
					if(ctx->Core.nFiles > 1)
					{
						if(ctx->Core.bWrite)
						{
							ctx->Core.bWrite = FALSE;	// 합치기면 덮어쓰기일 수 없음
							ctx->Core.bSamePath = TRUE;	// 합치기면 일단 같은 폴더로 간주
						}
						else
						{	// 덮어쓰기가 아니면 같은 폴더 또는 폴더가 지정되어 있다.
						}
						ctx->Core.llTotalSize = llTotalSize;	// 합치기면 토탈크기 다시 계산
						_tcscpy_s(ctx->File.szNewName, MAX_PATH, FileName);
					}
					else
						_tcscpy_s(ctx->File.szNewName, MAX_PATH, ctx->File.szFileName);
					return TRUE;
				}
				else
				{
					_stprintf_s(ctx->szError, MAX_PATH, _T("누락된 파일이 존재합니다.(%03d)"), nNo);
					ctx->nResult = -1;
					return FALSE;
				}
			}
			else
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("분할된 파일의 크기가 틀립니다."));
				ctx->nResult = -1;
				return FALSE;
			}
		}
		FindClose(hFind);
		ULONGLONG fSize = (ULONGLONG)fFile.nFileSizeHigh << (sizeof(int) * 8);
		fSize += fFile.nFileSizeLow;
		llTotalSize += fSize;
		if(ctx->Global->llSplitSize == 0LL)	// 아직 정해지지 않았으면 업데이트
			ctx->Global->llSplitSize = fSize;
		else
		{
			if(nErr > 0)	// 이미 오류가 발생했으면 다음 파일이 있으면 안된다.
			{
				_tcscpy_s(ctx->szError, MAX_PATH, _T("분할된 파일의 크기가 틀립니다."));
				ctx->nResult = -1;
				return FALSE;
			}
			if(fSize != ctx->Global->llSplitSize)
				nErr++;
		}
		nNo++;
	}
	ctx->Core.llTotalSize = llTotalSize;	// 합치기면 토탈크기 다시 계산
	_tcscpy_s(ctx->File.szNewName, MAX_PATH, FileName);
	return TRUE;
}

BOOL CEncDec::CheckTargetFile(ENCDEC_CTX *ctx)
{
	DWORD dwAtt;
	TCHAR szDrv[MAX_PATH], szPath[MAX_PATH], szName[MAX_PATH], szExt[MAX_PATH];
	_tsplitpath_s(ctx->File.szFileName, szDrv, MAX_PATH, szPath, MAX_PATH, szName, MAX_PATH, szExt, MAX_PATH);

	// 경로명 업데이트
	if((ctx->Global->nWorkType == 2) || ctx->Core.bWrite || ctx->Core.bSamePath)
		// 검사, 덮어쓰기, 같은 경로인 경우에는 경로명이 모두 일치
	{
		_stprintf_s(ctx->File.szNewName, MAX_PATH, _T("%s%s%s"), szDrv, szPath, szName);
	}
	else	// 다른 경로
	{
		dwAtt = GetFileAttributes(ctx->Global->szTargetFolder);
		if((dwAtt == INVALID_FILE_ATTRIBUTES) || !(dwAtt & FILE_ATTRIBUTE_DIRECTORY))
		{
			m_dwErr = GetLastError();
			CString warn;
			warn.Format(_T("출력 경로가 잘못되었습니다.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		_tcscpy_s(ctx->File.szNewName, MAX_PATH, ctx->Global->szTargetFolder);
		if(ctx->File.szNewName[_tcslen(ctx->File.szNewName) - 1] != _T('\\'))
			_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("\\"));
		_tcscat_s(ctx->File.szNewName, MAX_PATH, szName);
	}

	// 확장자 등 처리
	if(ctx->Global->nWorkType == 2)	// 검사이면
	{
		if(!ctx->Global->bJoin)	// 합치기가 아니면 확장자 추가
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
	}
	else if(ctx->Core.nMethod == 3)
	{
		if(ctx->Global->nWorkType == 0)	// 암호화면 j2j 추가
		{
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
			_tcscat_s(ctx->File.szNewName, MAX_PATH, ctx->Global->szExt);
		}
		else
		{	// 있던 .j2j가 없어졌으니 OK
		}
	}
	else
	{
		if((ctx->Core.nMethod < 2) && (ctx->Core.bJoin == TRUE))	// 하위버전에서 파일 합치기면
		{	// 있던 .001이 없어졌으니 OK
		}
		else if(ctx->Core.bWrite)	// 덮어쓰기
		{
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
		}
		else if(ctx->Core.bSamePath)
		{
			if(ctx->Global->nWorkType == 0)	// 암호화
				_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("_enc"));
			else
				_tcscat_s(ctx->File.szNewName, MAX_PATH, _T("_dec"));
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
		}
		else
			_tcscat_s(ctx->File.szNewName, MAX_PATH, szExt);
	}

	// Method3이고 (검사가 아니거나 추가작업이 있으면) 출력파일 확인
	if((ctx->Core.nMethod == 3) && ((ctx->Global->nWorkType != 2) || ctx->Global->bExistNextJob))
	{
		dwAtt = GetFileAttributes(ctx->File.szNewName);
		if(dwAtt != INVALID_FILE_ATTRIBUTES)
		{
			_tcscpy_s(ctx->szError, MAX_PATH, _T("출력 경로에 같은 이름의 파일이 이미 존재합니다."));
			ctx->nResult = -1;
			return FALSE;
		}
	}
	// 출력파일 확인
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
			_tcscpy_s(ctx->szError, MAX_PATH, _T("출력 경로에 같은 이름의 파일이 이미 존재합니다."));
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
		return -2;	// 파일 열 수 없음
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
	if(nReaded < 16)	// 덜 읽어왔으면 그전 파일 열어보자
	{
		if(ctx->Core.nFiles < 1)
			return -2;	// 잘못된 파일
		File.Format(_T("%s.%03d"), ctx->File.szFileName, ctx->Core.nFiles - 1);
		hF = CreateFile(File, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if(hF == INVALID_HANDLE_VALUE)
			return -2;	// 파일 열 수 없음
		dwOrder = 16 - nReaded;
		llToMove.QuadPart = dwOrder;
		llToMove.QuadPart *= -1;
		SetFilePointerEx(hF, llToMove, 0, FILE_END);
		ReadFile(hF, Buf, dwOrder, &dwRead, NULL);
		CloseHandle(hF);
	}

	// 첵섬확인
	int nNull = 0;
	for(int i = 1; i < 16; i += 2)
	{
		if(Buf[i] == 0)
			nNull++;
	}
	if(nNull == 0)	// 새버전인지 체크
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

		if(nVer > 9)	// 상위버전?
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
	else if(nNull == 8)	// 구버전인지 체크
	{
		nNull = 0;
		for(int i = 0; i < 16; i += 2)
		{
			if((Buf[i] >= '0') && (Buf[i] <= '9'))
				nNull++;
			else if((Buf[i] >= 'A') && (Buf[i] <= 'F'))
				nNull++;
		}
		if(nNull == 8)	// 구버전
			return -1;
		else	// 오류
			return -3;
	}
	else	// 오류
		return -3;
}

void CEncDec::InitDefaultValues(ENCDEC_CTX* ctx)	// 기본값들 설정
{
	switch(ctx->Core.nMethod)
	{
	case -1:	// 구버전
		ctx->Core.llBufferSize = BUFFER_SIZE_UNDER5;
		ctx->Core.llInterval = INTERVAL_COUNT_UNDER5;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_UNDER5;
		break;
	case 0:	// 5버전
		ctx->Core.llBufferSize = BUFFER_SIZE_OVER5;
		ctx->Core.llInterval = INTERVAL_COUNT_OVER5;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_OVER5;
		break;
	case 1:	// 방법1
		ctx->Core.llBufferSize = BUFFER_SIZE_METHOD1;
		ctx->Core.llInterval = INTERVAL_COUNT_METHOD1;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_METHOD1;
		break;
	case 2:	// 방법2
	case 3:	// 방법3
		ctx->Core.llBufferSize = BUFFER_SIZE_METHOD2;
		ctx->Core.llInterval = INTERVAL_COUNT_METHOD2;
		ctx->Core.llDefaultSkipSize = SKIP_SIZE_METHOD2;
		break;
	}

	// 핸들 생성 검사과정도 포함되므로 파일 갯수만큼 hSrc, hDst를 생성
	ctx->File.hSrc = new HANDLE[ctx->Core.nFiles];
	ctx->File.hDst = new HANDLE[ctx->Core.nFiles];
	InitHandle(ctx, FALSE, FALSE);
/*	if(ctx->Global->nWorkType == 0)	// 암호화면
	{
		ctx->File.hSrc = new HANDLE[1];	// 소스는 하나
		*(ctx->File.hSrc) = NULL;
		if(ctx->Core.bWrite)	// 덮어쓰기면
			ctx->File.hDst = ctx->File.hSrc;
		else
		{
			ctx->File.hDst = new HANDLE[ctx->Core.nFiles];	// 대상이 여러개
			for(int i = 0; i < ctx->Core.nFiles; i++)
				ctx->File.hDst[i] = NULL;
		}
	}
	else
	{
		ctx->File.hSrc = new HANDLE[ctx->Core.nFiles];
		for(int i = 0; i < ctx->Core.nFiles; i++)
			ctx->File.hSrc[i] = NULL;
		if(ctx->Core.bWrite)	// 덮어쓰기면
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
	ctx->File.dwShareRight = NULL;	// 기본은 독점
	if(ctx->Global->nWorkType == 0)	// 암호화
	{
		ctx->Global->bExistNextJob = TRUE;
		nNowWork = 0;
		nNextWork = 2;
	}
	else if(ctx->Global->nWorkType == 1)	// 복호화
	{
		ctx->Global->bExistNextJob = TRUE;
		nNowWork = 2;
		nNextWork = 1;
	}
	else	// 검사
	{
		nNowWork = 2;
		bNeedTwice = FALSE;	// 1회로 끝
		ctx->File.dwShareRight = FILE_SHARE_READ;	// 읽을 수 있도록
	}

	ctx->Global->nWorkType = nNowWork;

	// 1차 작업 시작
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

	if(!bNeedTwice)	// 1회로 끝이면
	{
		if(bReturn)	// 수행완료면 파일 정보 업데이트
		{
			UpdateResultString(ctx, nNowWork);
		}
#ifdef DEEP_DEBUG
		AfxMessageBox(m_strDbg);
#endif
		return bReturn;
	}

	if(nNowWork == 0)	// 변조였으면 싱크를 위해 3초 대기
		Sleep(3000);

	// 추가작업은 없다.
	ctx->Global->bExistNextJob = FALSE;
	if((ctx->Core.nMethod == 3) && (nNowWork == 0))	// 덮어쓰기거나 변조했으면
	{
		// 파일명 업데이트
		_tcscpy_s(ctx->File.szFileName, MAX_PATH, GetFirstTargetFile(ctx));
	}
	ctx->Global->nWorkType = nNextWork;

	// 2차 작업 시작
	bReturn = CheckValid(ctx);
	if(!bReturn)
		return bReturn;
	InitDefaultValues(ctx);
	bReturn = DoJob(ctx);

	InitHandle(ctx, TRUE, TRUE);
	if(bReturn)	// 수행완료면 파일정보 업데이트
	{
		if(nNextWork == 2)	// 여기에서 검사이면 암호화란 의미
			UpdateResultString(ctx, 0);
		else	// 복원
			UpdateResultString(ctx, 1);
	}
#ifdef DEEP_DEBUG
	AfxMessageBox(m_strDbg);
#endif
	return bReturn;
}

void CEncDec::InitLoopDefaults(ENCDEC_CTX *ctx)
{
	// 최초키 생성
	int nLen = _tcslen(ctx->Global->szPassword);
	BYTE* szPass = (BYTE*)ctx->Global->szPassword;
	int nBufSize;
	nBufSize = ctx->Core.llBufferSize;

	if(nLen == 0)	// 기본키
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
			// 소스파일 
			CString SrcFile;
			HANDLE* lpH;
			if((ctx->Core.nWorkType != 0) && ctx->Core.bJoin)	// 분할 합치기
			{
				SrcFile.Format(_T("%s.%03d"), ctx->File.szNewName, i + 1);
				if(ctx->Global->nWorkType == 0)	// 암호화시 사후 검사할 시점이면
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
				while(nReopen > 0)	// 안열려도 3회 반복 시도
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
				warn.Format(_T("작업 파일을 열 수 없습니다.(%d)"), m_dwErr);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
			if(ctx->File.szFirstTargetName[0] == _T('\0'))
			{
				_tcscpy_s(ctx->File.szFirstTargetName, MAX_PATH, SrcFile);
			}
		}

		if((ctx->Core.nWorkType != 2) && (ctx->Core.hDst == NULL))	// && (!bAfterCheck || bChecked))	// 체크가 아니고 핸들이 NULL이며 복호화전 체크한 후이거나 암호화중이면
		{
			// 대상파일
			CString DstFile;
			HANDLE* lpH;
			if((ctx->Global->nWorkType == 0) && (ctx->Global->llSplitSize != 0LL))	// 분할하기
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
				while(nReopen > 0)	// 안열려도 3회 반복 시도
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
					warn.Format(_T("출력 파일을 생성할 수 없습니다.(%d)"), m_dwErr);
					_tcscpy_s(ctx->szError, MAX_PATH, warn);
					ctx->nResult = -1;
					return FALSE;
				}
			}
			ctx->Core.hDst = *lpH;
			if(ctx->Core.bWrite)	// 덮어쓰기면 소스와 같은 핸들...
				ctx->Core.hDst = ctx->Core.hSrc;
			if(i == 0)	// 첫번째이면
			{
				_tcscpy_s(ctx->File.szFirstTargetName, MAX_PATH, DstFile);
				if((ctx->Core.nMethod == 3) && ctx->Core.bWrite)	// Method3이면서 덮어쓰기이면
				{
					// 엄밀하게 말하면 변조시에만 유효한 값임
					_tcscat_s(ctx->File.szFirstTargetName, MAX_PATH, ctx->File.szNewExt);
				}
			}
		}
		if(ctx->Core.nWorkType == 0)	// 암호화면
		{
			int nR;
#if 0	// 옛날 변조는 지원않는다.
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
				warn.Format(_T("변조가 실패하였습니다.(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}
		else if(ctx->Core.nWorkType == 1)	// 복호화면
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
				warn.Format(_T("복원이 실패하였습니다.(손상된 파일이거나 틀린 비밀번호)(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}
		else	// 검사이면
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

			if(!ctx->Global->bForce && (nR != 1))	// 강제 복원이 아니고 실패한 경우는 중지
			{
				ctx->Core.Crcs.Free();
				CString warn;
				warn.Format(_T("검사가 실패하였습니다.(손상된 파일이거나 틀린 비밀번호)(%d)"), nR);
				_tcscpy_s(ctx->szError, MAX_PATH, warn);
				ctx->nResult = -1;
				return FALSE;
			}
		}

		// 핸들 정리
		if((ctx->Core.nWorkType == 0) && !ctx->Core.bJoin)	// 암호화면
		{
			if(ctx->Core.nFiles != i - 1)
			{
				llToMove.QuadPart = 0LL;
				SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_BEGIN);
				ctx->Core.hDst = NULL;
			}
		}
		else if((ctx->Core.nWorkType != 0) && ctx->Core.bJoin)	// 복호화면
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
	if(ctx->Global->llSplitSize == 0LL)	// 분할하지 않을 경우
		dstRemainSize = remainSize;
	else
		dstRemainSize = ctx->Global->llSplitSize;
	ctx->Core.llFileProcessedSize = 0LL;

	int nPercent = 0;

//	int nInterval;	// 인터벌...
	BYTE* wBuf;	// 쓰기 작업할 때 사용할 버퍼

	DWORD dwOrder, dwRead, dwWritten, dwBufSize, dwUnit;
	while((remainSize > 0) && (dstRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// 이전 단계에서 BUFFER 크기에 미달한 크기를 처리했다면
		dwOrder = ctx->Core.nReadSize - ctx->Core.nWrittenSize;
		if(dwOrder)	// 이전 단계에서 읽은 크기만큼 쓰지 못했다면
		{
			ctx->Core.nReadSize = dwOrder;
			dwBufSize = dwOrder;	// 미처리한 크기
			wBuf = ctx->Core.buf + ctx->Core.nWrittenSize;
			goto Write;
		}
		else
		{
			ctx->Core.nReadSize = 0;
			ctx->Core.nWrittenSize = 0;
			dwBufSize = ctx->Core.llBufferSize;
		}

		// 원본 크기에 따라 읽어온다.(dstRemainSize는 다음 턴에서 고려)
		if(remainSize < dwBufSize)
			dwOrder = remainSize;
		else
			dwOrder = dwBufSize;

		// 현재 위치가 변조/복원할 위치인가? dwUnit == 1이면 변조/복원 필요
		dwUnit = ((ctx->Core.llProcessedSize + dwOrder)/ ctx->Core.llBufferSize) % ctx->Core.llInterval;
		if(ctx->Core.bWrite && (dwUnit != 1))	// 덮어쓰기이고 주기가 아니면 굳이 읽을 필요가 없다.
		{
			DWORD dwSkip = ctx->Core.llBufferSize * (ctx->Core.llInterval - dwUnit + 1);	// 시작이 1이므로
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
			warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(dwUnit == 1)	// 위변조가 필요하다.
		{
			if(ctx->Core.bWrite)	// 덮어쓰기이면 읽은 만큼 되돌리기...
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
		// 읽은 크기가 분할 남은 크기보다 작을 경우
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
			warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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
	if(!ctx->Core.bWrite)	// 덮어쓰기가 아니면 복사 시작
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
				warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
				warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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

	// 맨처음 위치 변조 시작
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

	int nInterval = -1;	// 인터벌...
//	BYTE* wBuf;	// 쓰기 작업할 때 사용할 버퍼
	BYTE* rBuf;	// 읽기 작업할 때 사용할 버퍼

//	BOOL bFirst = FALSE;	// 최초에는 org를 사용해야 하므로
	DWORD dwOrder, dwRead, dwWritten, dwBufSize;	//, dwUnit;

	// 포인터 앞으로
	while((remainSize > 0) && (srcRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// 이전 단계에서 BUFFER 크기 보다 작은 크기를 읽어왔다면
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

		// 현재 위치가 복원할 위치인가? dwUnit == 1이면 복원 필요
		if((ctx->Core.llRemainSkipSize != ctx->Core.llDefaultSkipSize) && (ctx->Core.bWrite || bCheck))	// 덮어쓰기거나 검사하기면서 주기가 아니면 굳이 읽을 필요가 없다.
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
			warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(ctx->Core.bIV1)	// 다음에 키로 사용할 버퍼에 읽은 값을 복사
			memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
		else
			memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);

		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(ctx->Core.llRemainSkipSize == ctx->Core.llDefaultSkipSize)	// 복원이 필요하다.
		{
			if(lDistance % ctx->Core.llBufferSize != 0)	// 버퍼크기만큼 되지 않으면 나중에
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
			if(!bCheck && ctx->Core.bWrite)	// 덮어쓰기이면 읽은 만큼 되돌리기...
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
					warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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
			dwOrder = remainSize;	// 남은 소스 파일크기
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
			warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(!bCheck && ctx->Core.bWrite)	// 덮어쓰기이면 파일 종료처리
		{
			llToMove.QuadPart = dwOrder;
			llToMove.QuadPart *= -1;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
			SetEndOfFile(ctx->Core.hSrc);
		}
		ctx->Core.nCrc += dwOrder;

		if(ctx->Core.nCrc < 16)	// 덜 읽어온 경우
		{
			if(ctx->Global->llSplitSize != 0)	// 분할파일이면 다음 파일로 진행
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

		// 다 읽었으면 비교 시작
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

			// 변조 방법 확인
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

//	BYTE* wBuf;	// 쓰기 작업할 때 사용할 버퍼
//	BYTE* rBuf;	// 읽기 작업할 때 사용할 버퍼

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize;	//, dwUnit;
	if(!bCheck && !ctx->Core.bWrite)	// 덮어쓰기가 아니고 체크가 아니면 파일복사 시작
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
				warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
				warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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

	// 맨처음 위치 처리
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
		warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		return 0;
	}

	if(!bCheck)	// 체크가 아니면 파일 종료처리
	{
		SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
		SetEndOfFile(ctx->Core.hDst);
	}

	// 다 읽었으면 비교 시작
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

		// 변조 방법 확인
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

	int nInterval = -1;	// 인터벌...
//	BYTE* wBuf;	// 쓰기 작업할 때 사용할 버퍼
	BYTE* rBuf;	// 읽기 작업할 때 사용할 버퍼

//	BOOL bFirst = FALSE;	// 최초에는 org를 사용해야 하므로
	DWORD dwOrder, dwRead, dwWritten, dwBufSize;	//, dwUnit;

	while((remainSize > 0) && (srcRemainSize > 0))
	{
		nPercent = ctx->Core.llProcessedSize * 100 / ctx->Core.llTotalSize;
		m_pStat->SetPos(nPercent);

		// 이전 단계에서 BUFFER 크기 보다 작은 크기를 읽어왔다면
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

		// 현재 위치가 복원할 위치인가? dwUnit == 1이면 복원 필요
		if((ctx->Core.llRemainSkipSize != ctx->Core.llDefaultSkipSize) && (ctx->Core.bWrite || bCheck))	// 덮어쓰기거나 검사하기면서 주기가 아니면 굳이 읽을 필요가 없다.
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
			AfxMessageBox(_T("작업 파일을 읽어오는 데 실패하였습니다."));
			return 0;
		}
		if(ctx->Core.bIV1)	// 다음에 키로 사용할 버퍼에 읽은 값을 복사
			memcpy_s(ctx->Core.IV2, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);
		else
			memcpy_s(ctx->Core.IV, ctx->Core.llBufferSize, ctx->Core.buf, ctx->Core.llBufferSize);

		ctx->Core.nReadSize += dwOrder;
		int lDistance = ctx->Core.nReadSize;
		if(ctx->Core.llRemainSkipSize == ctx->Core.llDefaultSkipSize)	// 복원이 필요하다.
		{
			if(lDistance % ctx->Core.llBufferSize != 0)	// 버퍼크기만큼 되지 않으면 나중에
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
			if(!bCheck && ctx->Core.bWrite)	// 덮어쓰기이면 읽은 만큼 되돌리기...
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
					warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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
			dwOrder = remainSize;	// 남은 소스 파일크기
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
			warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
			_tcscpy_s(ctx->szError, MAX_PATH, warn);
			ctx->nResult = -1;
			return FALSE;
		}
		if(!bCheck && ctx->Core.bWrite)	// 덮어쓰기이면 파일 종료처리
		{
			llToMove.QuadPart = dwOrder;
			llToMove.QuadPart *= -1;
			SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_CURRENT);
			SetEndOfFile(ctx->Core.hSrc);
		}
		ctx->Core.nCrc += dwOrder;

		if(ctx->Core.nCrc < 16)	// 덜 읽어온 경우
		{
			if(ctx->Global->llSplitSize != 0)	// 분할파일이면 다음 파일로 진행
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

		// 다 읽었으면 비교 시작
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
		warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
	if(memcmp(ctx->Core.IV, ctx->Core.IV_Cmp, dwOrder) != 0)	// 두 값이 일치하지 않으면 메모리 오류
	{
		_tcscpy_s(ctx->szError, MAX_PATH, _T("변조가 실패하였습니다.(다른 값)"));
		ctx->nResult = -1;
		return FALSE;
	}

	WriteFile(ctx->Core.hDst, ctx->Core.IV, dwOrder, &dwWritten, NULL);
	if(dwOrder != dwWritten)
	{
		m_dwErr = GetLastError();
		CString warn;
		warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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
		warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
		_tcscpy_s(ctx->szError, MAX_PATH, warn);
		ctx->nResult = -1;
		return FALSE;
	}
	if(ctx->Core.bIV1)	// 다음에 키로 사용할 버퍼에 읽은 값을 복사
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
		warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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

	// ctx 업데이트
	ctx->Core.nCount = GetMethod3Multiple(ctx, TRUE);
	
	ULONGLONG llTotal;
	if(ctx->Core.bWrite)
		llTotal = ctx->Core.nCount * 4;
	else
		llTotal = ctx->Core.llTotalSize + (ctx->Core.nCount * ctx->Core.llBufferSize * 4);	// 맨앞 맨뒤 읽고 쓰고;

	int nPercent = 0;

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize, dwUnit;
	if(!ctx->Core.bWrite)	// 덮어쓰기가 아니면 복사 시작
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
				warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
				warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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

	// 맨처음 위치 변조 시작(파일크기의 5%만큼을 변조(BUFFER_SIZE_METHOD2의 정배수)
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
		if(ctx->Core.bWrite)	// 덮어쓰기이면
			nPercent = (i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// 맨마지막 위치 변조 시작(파일크기의 5%만큼을 변조(BUFFER_SIZE_METHOD2의 정배수)
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
		if(ctx->Core.bWrite)	// 덮어쓰기이면
			nPercent = (ctx->Core.nCount + i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (ctx->Core.nCount + i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// 파일명 추가, 추가파일명 길이, CRC 및 버전 추가
	if(AccessTail(ctx, FALSE, TRUE, FALSE) <= 0)
		return 0;
	// 덮어쓰기면 파일명을 변경, 아니면 대상파일명 설정시에 미리 적용
	if(ctx->Core.bWrite)	// 덮어쓰기면 파일명 변경
	{
		TCHAR szNewName[MAX_PATH];
		_tcscpy_s(szNewName, MAX_PATH, ctx->File.szNewName);
		_tcscat_s(szNewName, MAX_PATH, ctx->File.szNewExt);
		FlushFileBuffers(ctx->File.hSrc[0]);
		CloseHandle(ctx->File.hSrc[0]);	// 분할이 없으므로 1개 밖에 없다.
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

	// ctx 업데이트
	ctx->Core.nCount = GetMethod3Multiple(ctx, FALSE);

	ULONGLONG llTotal;
	if(bCheck || ctx->Core.bWrite)	// 체크거나 덮어쓰기면
		llTotal = ctx->Core.nCount * 4;
	else
		llTotal = ctx->Core.llTotalSize + (ctx->Core.nCount * ctx->Core.llBufferSize * 4);	// 맨앞 맨뒤 읽고 쓰고

	int nPercent = 0;

//	BYTE* wBuf;	// 쓰기 작업할 때 사용할 버퍼
//	BYTE* rBuf;	// 읽기 작업할 때 사용할 버퍼

	DWORD dwOrder, dwRead, dwWritten;	//, dwBufSize;	//, dwUnit;
	if(!bCheck && !ctx->Core.bWrite)	// 덮어쓰기가 아니고 체크가 아니면 파일복사 시작
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
				warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
				warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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

	// 맨처음 위치 처리
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
		if(bCheck || ctx->Core.bWrite)	// 체크거나 덮어쓰기면
			nPercent = (i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// 맨마지막 부분 처리
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
		if(bCheck || ctx->Core.bWrite)	// 체크거나 덮어쓰기면
			nPercent = (ctx->Core.nCount + i + 1) * 100 * 2 / llTotal;
		else
			nPercent = (ctx->Core.llTotalSize + (ctx->Core.llBufferSize * (ctx->Core.nCount + i + 1) * 2)) * 100 / llTotal;
		m_pStat->SetPos(nPercent);
	}

	// 체크면
	if(bCheck)
	{
		if(AccessTail(ctx, FALSE, FALSE, FALSE) <= 0)
			return 0;
		else
			return 1;
	}

	// 체크가 아니면 파일 종료처리
//	llToMove.QuadPart = 0;
//	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	SetEndOfFile(ctx->Core.hDst);

	// 덮어쓰기면 파일명을 변경, 아니면 대상파일명 설정시에 미리 적용
	if(ctx->Core.bWrite)
	{
		TCHAR szNewName[MAX_PATH];
		_tcscpy_s(szNewName, MAX_PATH, ctx->File.szFileName);
		if(_tcsicmp((szNewName + _tcslen(szNewName) - _tcslen(ctx->Global->szExt)), ctx->Global->szExt) == 0)
			szNewName[_tcslen(szNewName) - _tcslen(ctx->Global->szExt)] = _T('\0');
		FlushFileBuffers(ctx->File.hSrc[0]);
		CloseHandle(ctx->File.hSrc[0]);	// 분할이 없으므로 1개 밖에 없다.
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
	// 암호화
	InitLoopDefaults(ctx);
	BOOL bRet = FileLoop(ctx);
//	InitHandle(ctx, TRUE, FALSE);
	return bRet;
}

BOOL CEncDec::DecryptFile(ENCDEC_CTX *ctx)
{
	// 복호화
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
 * Method3에서 몇번의 BUFFER_SIZE_METHOD2만큼 변조/복원해야 하는지를 리턴(5%?)
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
		nCount = 1;	// 최소 1회...
	return nCount;
}

/*
 * Tail부분을 쓰거나 읽는다. bGetLength가 참이면 뒤의 인자들은 무시되며 Tail의 길이만 리턴된다.
 * 읽는 경우(!bWrite) bUpdate가 참이면 ctx에 업데이트 거짓이면 비교 결과를 리턴한다.
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

	if(bGetLength || !bWrite)	// 읽어온다.
	{
		llToMove.QuadPart = -32;
		SetFilePointerEx(ctx->Core.hSrc, llToMove, NULL, FILE_END);
		ReadFile(ctx->Core.hSrc, buf, sizeof(buf), &dwResult, NULL);
		if(sizeof(buf) != dwResult)
			return -2;	// 파일 액세스 실패
		if(buf[16 + 8] != 'L')
			return -1;	// 잘못된 파일
		if(buf[0] != 0)	// 파일명 변조가 되었으면
		{
			nNameLen = (short)(buf + 1);
			if((nNameLen <= 0) || (nNameLen > MAX_PATH * 2))
				return -1;	// 잘못된 파일
		}
		if(bGetLength)
			return 32 + nNameLen;
		nCount = buf[8];
		if(nCount <= 0)	// 반드시 1 이상
			return -1;	// 잘못된 파일
		if(!bUpdate)
		{
			if(ctx->Core.nCount != nCount)
				return 0;	// 일치 않음
		}
		else
			ctx->Core.nCount = nCount;

		// CRC값 비교/업데이트
		::ZeroMemory(szBuf, sizeof(szBuf));
		memcpy_s(szBuf, sizeof(szBuf), (buf + 16), 8);
		if(!bUpdate)	// 비교이면
		{
			sprintf_s(szCrcA, MAX_PATH, "%08X", ctx->Core.dwCRC);
			if(strncmp(szBuf, szCrcA, 8) != 0)
				return 0;	// 일치 않음
		}
		else
		{
			strncpy_s((char*)ctx->Core.szCrc, sizeof(ctx->Core.szCrc), szBuf, 8);
		}

		// Method 업데이트
		char szMethod[2] = {0,};
		szMethod[0] = buf[16 + 9];
		int nMethod = atoi(szMethod);
		if(!bUpdate)	// 비교이면
		{
			if(ctx->Core.nMethod != nMethod)
				return 0;	// 일치않음
		}
		else
			ctx->Core.nMethod = nMethod;

		// 파일명 비교(현재는 파일명 변조를 않으므로...)
		if(nNameLen > 0)
		{
			if(!bUpdate)
				return 0;
			else
			{
//				읽어와서 복사하는 것을 나중에 구현하자...
//				_tcscpy_s(ctx->Core.szOrgName, sizeof(ctx->Core.szOrgName), ...
			}
		}

		// 여기까지 왔으면 비교가 성공했거나 업데이트가 완료된 경우다.
		return 1;
	}

	// 여기부터는 쓰기...
	// 파일명 변조할 경우 파일명 복사하는 것은 나중에 구현한다...
	::ZeroMemory(buf, sizeof(buf));
	::ZeroMemory(szCrcA, sizeof(szCrcA));
	buf[0] = 0;	// 파일명 변조 않음
	buf[8] = ctx->Core.nCount;
	sprintf_s(szCrcA, MAX_PATH, "%08XL%d", ctx->Core.dwCRC, 3);
	strcat_s(szCrcA, MAX_PATH, "000009");
	memcpy_s(buf + 16, 16, szCrcA, 16);
	llToMove.QuadPart = 0;
	SetFilePointerEx(ctx->Core.hDst, llToMove, NULL, FILE_END);
	WriteFile(ctx->Core.hDst, buf, 32, &dwResult, NULL);
	if(dwResult != 32)
		return -2;	// 파일 액세스 실패
	return 32;	// 쓴 바이트 수
}

void CEncDec::UpdateResultString(ENCDEC_CTX *ctx, int nWorkType)
{
	// 성공적으로 수행된 경우 결과문자열 업데이트
	TCHAR szBuf[MAX_PATH];
	if(nWorkType == 1)	// 복원이면
	{
		_tcscpy_s(ctx->szError, MAX_PATH, _T("복원 완료"));
		return;
	}

	if(nWorkType == 0)
		_tcscpy_s(szBuf, MAX_PATH, _T("변조 완료 (변조방법 : "));
	else
		_tcscpy_s(szBuf, MAX_PATH, _T("검사 완료 (변조방법 : "));

	switch(ctx->Core.nMethod)
	{
	case -1:	// UNDER5
		_tcscat_s(szBuf, MAX_PATH, _T("1.0.0.4이하)"));
		break;
	case 0:	// OVER5
		_tcscat_s(szBuf, MAX_PATH, _T("1.0.0.5)"));
		break;
	case 1:	// METHOD1
		_tcscat_s(szBuf, MAX_PATH, _T("방법1)"));
		break;
	case 2:	// METHOD2
		_tcscat_s(szBuf, MAX_PATH, _T("방법2)"));
		break;
	case 3:	// METHOD3
		_tcscat_s(szBuf, MAX_PATH, _T("방법3)"));
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
//		warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
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
//		warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
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
//		warn.Format(_T("작업 파일을 읽어오는 데 실패하였습니다.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//	if(ctx->Core.bIV1)	// 다음에 키로 사용할 버퍼에 읽은 값을 복사
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
//		warn.Format(_T("대상 파일을 쓰는 데 실패하였습니다.(%d)"), m_dwErr);
//		_tcscpy_s(ctx->szError, MAX_PATH, warn);
//		ctx->nResult = -1;
//		return FALSE;
//	}
//	return 1;
//}