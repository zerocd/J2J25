#pragma once
#include "stdafx.h"
#include "Crc32.h"
//#include

typedef struct
{
	TCHAR szPassword[64];
	int nMethod;	// 방법번호 -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2, 3 METHOD3
	BOOL bWrite;	// 덮어쓰기
	BOOL bSamePath;	// 같은 경로
	BOOL bJoin;	// 합치기
	UINT nWorkType;	// 0 암호화, 1 복호화, 2 검사
	BOOL bExistNextJob;	// 추후 작업이 있는지?
	CProgressCtrl * pStat;	// 프로세스컨트롤
	TCHAR szTargetFolder[MAX_PATH];
	ULONGLONG llSplitSize;	// 분할할 크기 또는 복원시 파일당 분할된 크기
	BOOL bForce;	// 오류 발생해도 강제 복원?
	TCHAR szExt[MAX_PATH];
} ENCDEC_GLOBAL;

typedef struct
{
	int nFiles;	// 분할관련 파일갯수(1부터 시작)
	BOOL bJoin;	// 합치기(파일별 설정)
	BOOL bWrite;	// 덮어쓰기
	BOOL bSamePath;	// 같은 경로
	int nMethod;	// 방법번호 -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2, 3 METHOD3 
	UINT nWorkType;	// 0 암호화, 1 복호화, 2 검사
	ULONGLONG llBufferSize;	// 버퍼 크기
	ULONGLONG llInterval;	// 인터벌
	ULONGLONG llDefaultSkipSize;	// 스킵할 크기
	ULONGLONG llTotalSize;	// 전체 크기
	ULONGLONG llRemainSize;	// 남은 크기
	ULONGLONG llProcessedSize;	// 진행된 크기
	ULONGLONG llFileProcessedSize;	// 단일 파일 진행된 크기
	ULONGLONG llRemainSkipSize;	// 남은 스킵할 크기
	int nReadSize;	// 읽은 단위 크기
	int nWrittenSize;	// 쓴 단위 크기
	HANDLE hSrc;	// 원본 핸들
	HANDLE hDst;	// 대상 핸들
//	HANDLE *hTarget;	// 현재시점의 대상 핸들
	BYTE IV[BUFFER_SIZE_METHOD2];	// 키 버퍼
	BYTE IV_Cmp[BUFFER_SIZE_METHOD2];	// 키 버퍼(비교용)
	CCrc32 Crcs;	// CRC
	DWORD dwCRC;	// CRC용 DWORD
	BYTE buf[BUFFER_SIZE_METHOD2];	// 읽기 쓰기에 사용될 버퍼
	BYTE IV2[BUFFER_SIZE_METHOD2];	// 두번째 버퍼
	BOOL bIV1;	// IV[]를 사용할 것인가?
	TCHAR szCrc[16];	// Tail에 저장된 CRC값
	int nCrc;	// Crc 읽어온 값
	int nCount;	// METHOD3부터 사용되는 값으로 앞에서 몇번 변조하였는지
	TCHAR szOrgName[MAX_PATH];	// 원래 이름
} ENCDEC_CORE;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
	TCHAR szNewName[MAX_PATH];
	TCHAR szFirstTargetName[MAX_PATH];
	TCHAR szNewExt[MAX_PATH];
	DWORD dwShareRight;	// 파일 열기시 공유권한
	HANDLE *hSrc;
	HANDLE *hDst;
} ENCDEC_FILE;

typedef struct
{
	ENCDEC_GLOBAL * Global;
	ENCDEC_CORE Core;
	ENCDEC_FILE File;
	TCHAR szError[MAX_PATH];	// 오류시 문자열
	int nResult;	// 리턴값
} ENCDEC_CTX;


class CEncDec
{
public:
	CEncDec(ENCDEC_CTX* ctx, CProgressCtrl* pStat);
	virtual ~CEncDec();

	BOOL SimpleDo(ENCDEC_CTX* ctx);	// 유효성 검사, 기본값 설정, 변조여부 확인 및 변조/복원작업을 한번에...

	BOOL CheckValid(ENCDEC_CTX* ctx);	// 파일 유효성 검사
	void InitDefaultValues(ENCDEC_CTX* ctx);	// 기본값들 설정
	void InitLoopDefaults(ENCDEC_CTX* ctx);	// 각 작업시마다 초기화할 값들 초기화
	BOOL DoJob(ENCDEC_CTX* ctx);	// 단일작업 수행
	BOOL CheckEncryptedFile(ENCDEC_CTX* ctx);	// 변조여부 확인
	BOOL EncryptFile(ENCDEC_CTX* ctx);	// 변조 작업 수행
	BOOL DecryptFile(ENCDEC_CTX* ctx);
	TCHAR* GetFirstTargetFile(ENCDEC_CTX* ctx);
private:
	ENCDEC_CTX* m_CTX;
	CProgressCtrl* m_pStat;
	BOOL CheckFileSize(ENCDEC_CTX* ctx);	// 원본 파일 크기확인 및 파일최소크기 확인
	BOOL CheckJoinFile(ENCDEC_CTX* ctx);	// 합치기시 동일한 크기인지 파일들이 다 있는지 확인
	BOOL CheckTargetFile(ENCDEC_CTX* ctx);	// 출력경로 확인
	int CheckValidVersion(ENCDEC_CTX* ctx);	// 버전 확인
	BOOL FileLoop(ENCDEC_CTX* ctx);
	BOOL PostEnc(ENCDEC_CTX* ctx);
#if 0
	int EncFile(ENCDEC_CTX* ctx);
	int EncFile2(ENCDEC_CTX* ctx);	// 방법2용
#endif
	int EncFile3(ENCDEC_CTX* ctx);	// 방법3용
	int DecFile(ENCDEC_CTX* ctx, BOOL bCheck);
	int DecFile2(ENCDEC_CTX* ctx, BOOL bCheck);	// 방법2용
	int DecFile3(ENCDEC_CTX* ctx, BOOL bCheck);	// 방법3용
	BOOL Encrypt2(ENCDEC_CTX* ctx);	// 현재위치로부터 METHOD2 BUFFER크기만큼을 변조
	BOOL Decrypt2(ENCDEC_CTX* ctx, BOOL bCheck);	// 현재위치로부터 METHOD2 BUFFER크기만큼을 복원
	int DecFileOld(ENCDEC_CTX* ctx, BOOL bCheck);
	void InitHandle(ENCDEC_CTX* ctx, BOOL bClose, BOOL bDelete);
	int GetMethod3Multiple(ENCDEC_CTX* ctx, BOOL bEncode);
	int AccessTail(ENCDEC_CTX* ctx, BOOL bGetLength, BOOL bWrite, BOOL bUpdate);
	void UpdateResultString(ENCDEC_CTX* ctx, int nWorkType);	// 결과문자열 업데이트
	DWORD m_dwErr;
	CString m_strDbg;	// 디버그 메시지용
};