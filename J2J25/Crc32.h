#pragma once
class CCrc32
{
public:
	CCrc32();
	virtual ~CCrc32();

	void Init();
	void Free();
	int MemCrc32(BYTE *Buf, int nSize, DWORD *dwCrc32);

protected:
	inline void CalcCrc32(BYTE Buf, DWORD *dwCrc32);
	DWORD *m_Crc32Table;
};