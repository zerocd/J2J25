#pragma once
#include "stdafx.h"
#include "Crc32.h"
//#include

typedef struct
{
	TCHAR szPassword[64];
	int nMethod;	// �����ȣ -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2, 3 METHOD3
	BOOL bWrite;	// �����
	BOOL bSamePath;	// ���� ���
	BOOL bJoin;	// ��ġ��
	UINT nWorkType;	// 0 ��ȣȭ, 1 ��ȣȭ, 2 �˻�
	BOOL bExistNextJob;	// ���� �۾��� �ִ���?
	CProgressCtrl * pStat;	// ���μ�����Ʈ��
	TCHAR szTargetFolder[MAX_PATH];
	ULONGLONG llSplitSize;	// ������ ũ�� �Ǵ� ������ ���ϴ� ���ҵ� ũ��
	BOOL bForce;	// ���� �߻��ص� ���� ����?
	TCHAR szExt[MAX_PATH];
} ENCDEC_GLOBAL;

typedef struct
{
	int nFiles;	// ���Ұ��� ���ϰ���(1���� ����)
	BOOL bJoin;	// ��ġ��(���Ϻ� ����)
	BOOL bWrite;	// �����
	BOOL bSamePath;	// ���� ���
	int nMethod;	// �����ȣ -1 UNDER5, 0 OVER5, 1 METHOD1, 2 METHOD2, 3 METHOD3 
	UINT nWorkType;	// 0 ��ȣȭ, 1 ��ȣȭ, 2 �˻�
	ULONGLONG llBufferSize;	// ���� ũ��
	ULONGLONG llInterval;	// ���͹�
	ULONGLONG llDefaultSkipSize;	// ��ŵ�� ũ��
	ULONGLONG llTotalSize;	// ��ü ũ��
	ULONGLONG llRemainSize;	// ���� ũ��
	ULONGLONG llProcessedSize;	// ����� ũ��
	ULONGLONG llFileProcessedSize;	// ���� ���� ����� ũ��
	ULONGLONG llRemainSkipSize;	// ���� ��ŵ�� ũ��
	int nReadSize;	// ���� ���� ũ��
	int nWrittenSize;	// �� ���� ũ��
	HANDLE hSrc;	// ���� �ڵ�
	HANDLE hDst;	// ��� �ڵ�
//	HANDLE *hTarget;	// ��������� ��� �ڵ�
	BYTE IV[BUFFER_SIZE_METHOD2];	// Ű ����
	BYTE IV_Cmp[BUFFER_SIZE_METHOD2];	// Ű ����(�񱳿�)
	CCrc32 Crcs;	// CRC
	DWORD dwCRC;	// CRC�� DWORD
	BYTE buf[BUFFER_SIZE_METHOD2];	// �б� ���⿡ ���� ����
	BYTE IV2[BUFFER_SIZE_METHOD2];	// �ι�° ����
	BOOL bIV1;	// IV[]�� ����� ���ΰ�?
	TCHAR szCrc[16];	// Tail�� ����� CRC��
	int nCrc;	// Crc �о�� ��
	int nCount;	// METHOD3���� ���Ǵ� ������ �տ��� ��� �����Ͽ�����
	TCHAR szOrgName[MAX_PATH];	// ���� �̸�
} ENCDEC_CORE;

typedef struct
{
	TCHAR szFileName[MAX_PATH];
	TCHAR szNewName[MAX_PATH];
	TCHAR szFirstTargetName[MAX_PATH];
	TCHAR szNewExt[MAX_PATH];
	DWORD dwShareRight;	// ���� ����� ��������
	HANDLE *hSrc;
	HANDLE *hDst;
} ENCDEC_FILE;

typedef struct
{
	ENCDEC_GLOBAL * Global;
	ENCDEC_CORE Core;
	ENCDEC_FILE File;
	TCHAR szError[MAX_PATH];	// ������ ���ڿ�
	int nResult;	// ���ϰ�
} ENCDEC_CTX;


class CEncDec
{
public:
	CEncDec(ENCDEC_CTX* ctx, CProgressCtrl* pStat);
	virtual ~CEncDec();

	BOOL SimpleDo(ENCDEC_CTX* ctx);	// ��ȿ�� �˻�, �⺻�� ����, �������� Ȯ�� �� ����/�����۾��� �ѹ���...

	BOOL CheckValid(ENCDEC_CTX* ctx);	// ���� ��ȿ�� �˻�
	void InitDefaultValues(ENCDEC_CTX* ctx);	// �⺻���� ����
	void InitLoopDefaults(ENCDEC_CTX* ctx);	// �� �۾��ø��� �ʱ�ȭ�� ���� �ʱ�ȭ
	BOOL DoJob(ENCDEC_CTX* ctx);	// �����۾� ����
	BOOL CheckEncryptedFile(ENCDEC_CTX* ctx);	// �������� Ȯ��
	BOOL EncryptFile(ENCDEC_CTX* ctx);	// ���� �۾� ����
	BOOL DecryptFile(ENCDEC_CTX* ctx);
	TCHAR* GetFirstTargetFile(ENCDEC_CTX* ctx);
private:
	ENCDEC_CTX* m_CTX;
	CProgressCtrl* m_pStat;
	BOOL CheckFileSize(ENCDEC_CTX* ctx);	// ���� ���� ũ��Ȯ�� �� �����ּ�ũ�� Ȯ��
	BOOL CheckJoinFile(ENCDEC_CTX* ctx);	// ��ġ��� ������ ũ������ ���ϵ��� �� �ִ��� Ȯ��
	BOOL CheckTargetFile(ENCDEC_CTX* ctx);	// ��°�� Ȯ��
	int CheckValidVersion(ENCDEC_CTX* ctx);	// ���� Ȯ��
	BOOL FileLoop(ENCDEC_CTX* ctx);
	BOOL PostEnc(ENCDEC_CTX* ctx);
#if 0
	int EncFile(ENCDEC_CTX* ctx);
	int EncFile2(ENCDEC_CTX* ctx);	// ���2��
#endif
	int EncFile3(ENCDEC_CTX* ctx);	// ���3��
	int DecFile(ENCDEC_CTX* ctx, BOOL bCheck);
	int DecFile2(ENCDEC_CTX* ctx, BOOL bCheck);	// ���2��
	int DecFile3(ENCDEC_CTX* ctx, BOOL bCheck);	// ���3��
	BOOL Encrypt2(ENCDEC_CTX* ctx);	// ������ġ�κ��� METHOD2 BUFFERũ�⸸ŭ�� ����
	BOOL Decrypt2(ENCDEC_CTX* ctx, BOOL bCheck);	// ������ġ�κ��� METHOD2 BUFFERũ�⸸ŭ�� ����
	int DecFileOld(ENCDEC_CTX* ctx, BOOL bCheck);
	void InitHandle(ENCDEC_CTX* ctx, BOOL bClose, BOOL bDelete);
	int GetMethod3Multiple(ENCDEC_CTX* ctx, BOOL bEncode);
	int AccessTail(ENCDEC_CTX* ctx, BOOL bGetLength, BOOL bWrite, BOOL bUpdate);
	void UpdateResultString(ENCDEC_CTX* ctx, int nWorkType);	// ������ڿ� ������Ʈ
	DWORD m_dwErr;
	CString m_strDbg;	// ����� �޽�����
};