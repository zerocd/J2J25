#include "stdafx.h"
#include "Crc32.h"

CCrc32::CCrc32() : m_Crc32Table(NULL)
{
}

CCrc32::~CCrc32()
{
	Free();
}

void CCrc32::Init()
{
	DWORD dwPolynomial = 0xEDB88320;
	int i, j;

	Free();
	m_Crc32Table = new DWORD[256];

	DWORD dwCrc;
	for(i = 0; i < 256; i++)
	{
		dwCrc = i;
		for(j = 8; j > 0; j--)
		{
			if(dwCrc & 1)
				dwCrc = (dwCrc >> 1) ^ dwPolynomial;
			else
				dwCrc >>= 1;
		}
		m_Crc32Table[i] = dwCrc;
	}
}

void CCrc32::Free()
{
	if(m_Crc32Table)
	{
		delete [] m_Crc32Table;
		m_Crc32Table = NULL;
	}
}

inline void CCrc32::CalcCrc32(BYTE Buf, DWORD *dwCrc32)
{
	*dwCrc32 = ((*dwCrc32) >> 8) ^ m_Crc32Table[(Buf) ^ ((*dwCrc32) & 0x000000FF)];
}

// 최초 호출시 *dwCrc32 = 0xFFFFFFFF로 초기화해야 한다.
int CCrc32::MemCrc32(BYTE *Buf, int nSize, DWORD *dwCrc32)
{
	if(m_Crc32Table == NULL)
		return 0;	// 초기화가 안되어 있음

//	*dwCrc32 = 0xFFFFFFFF;
	for(int i = 0; i < nSize; i++)
	{
		CalcCrc32(Buf[i], dwCrc32);
	}

	*dwCrc32 = ~(*dwCrc32);
	return 1;
}