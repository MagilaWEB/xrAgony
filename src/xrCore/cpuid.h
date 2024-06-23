#pragma once
#include <bitset>
#include <winternl.h>
#include <memory>

struct _processor_info final
{
	XRCORE_API _processor_info();

	string32 vendor; // vendor name
	string64 brand; // Name of model eg. Intel_Pentium_Pro
	int family; // family of the processor, eg. Intel_Pentium_Pro is family 6 processor
	int model; // model of processor, eg. Intel_Pentium_Pro is model 1 of family 6 processor
	int stepping; // Processor revision number
	u16 threadCount; // number of available threads, both physical and logical
	u16 coresCount; // number of physical cores

	XRCORE_API void clearFeatures() { m_f1_ECX = m_f1_EDX = m_f7_EBX = m_f7_ECX = m_f81_ECX = m_f81_EDX = 0; }
	XRCORE_API bool hasMMX() const { return m_f1_EDX[23]; }
	XRCORE_API bool has3DNOWExt() const { return m_f81_EDX[30]; }
	XRCORE_API bool has3DNOW() const { return m_f81_EDX[31]; }
	XRCORE_API bool hasSSE() const { return m_f1_EDX[25]; }
	XRCORE_API bool hasSSE2() const { return m_f1_EDX[26]; }
	XRCORE_API bool hasSSE3() const { return m_f1_ECX[0]; }
	XRCORE_API bool hasMWAIT() const { return m_f1_ECX[3]; }
	XRCORE_API bool hasSSSE3() const { return m_f1_ECX[9]; }
	XRCORE_API bool hasSSE41() const { return m_f1_ECX[19]; }
	XRCORE_API bool hasSSE42() const { return m_f1_ECX[20]; }
	XRCORE_API bool hasSSE4a() const { return m_f81_ECX[6]; }
	XRCORE_API bool hasAVX() const { return m_f1_ECX[28]; }
	XRCORE_API bool hasAVX2() const { return m_f7_EBX[5]; }
	XRCORE_API bool hasAVX512F() const { return m_f7_EBX[16]; }
	XRCORE_API bool hasAVX512PF() const { return m_f7_EBX[26]; }
	XRCORE_API bool hasAVX512ER() const { return m_f7_EBX[27]; }
	XRCORE_API bool hasAVX512CD() const { return m_f7_EBX[28]; }

	DWORD m_dwNumberOfProcessors;

private:
	std::bitset<32> m_f1_ECX;
	std::bitset<32> m_f1_EDX;
	std::bitset<32> m_f7_EBX;
	std::bitset<32> m_f7_ECX;
	std::bitset<32> m_f81_ECX;
	std::bitset<32> m_f81_EDX;
};