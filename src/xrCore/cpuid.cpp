#include "stdafx.h"
#include "cpuid.h"
#include <intrin.h>
#pragma comment(lib, "ntdll.lib")

_processor_info::_processor_info()
{
	int cpuInfo[4];
	// detect cpu vendor
	__cpuid(cpuInfo, 0);
	memcpy(vendor, &(cpuInfo[1]), sizeof(int));
	memcpy(vendor + sizeof(int), &(cpuInfo[3]), sizeof(int));
	memcpy(vendor + 2 * sizeof(int), &(cpuInfo[2]), sizeof(int));

	// detect cpu model
	__cpuid(cpuInfo, 0x80000002);
	memcpy(brand, cpuInfo, sizeof(cpuInfo));
	__cpuid(cpuInfo, 0x80000003);
	memcpy(brand + sizeof(cpuInfo), cpuInfo, sizeof(cpuInfo));
	__cpuid(cpuInfo, 0x80000004);
	memcpy(brand + 2 * sizeof(cpuInfo), cpuInfo, sizeof(cpuInfo));

	// detect cpu main features
	__cpuid(cpuInfo, 1);
	stepping = cpuInfo[0] & 0xf;
	model = (u8)((cpuInfo[0] >> 4) & 0xf) | ((u8)((cpuInfo[0] >> 16) & 0xf) << 4);
	family = (u8)((cpuInfo[0] >> 8) & 0xf) | ((u8)((cpuInfo[0] >> 20) & 0xff) << 4);
	m_f1_ECX = cpuInfo[2];
	m_f1_EDX = cpuInfo[3];

	__cpuid(cpuInfo, 7);
	m_f7_EBX = cpuInfo[1];
	m_f7_ECX = cpuInfo[2];

	// and check 3DNow! support
	__cpuid(cpuInfo, 0x80000001);
	m_f81_ECX = cpuInfo[2];
	m_f81_EDX = cpuInfo[3];

	// Calculate available processors
	DWORD returnedLength = 0;
	DWORD byteOffset = 0;
	GetLogicalProcessorInformation(nullptr, &returnedLength);

	auto buffer = std::make_unique<u8[]>(returnedLength);
	auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(buffer.get());
	GetLogicalProcessorInformation(ptr, &returnedLength);

	u16 processorCoreCount = 0;
	u16 logicalProcessorCount = 0;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnedLength)
	{
		if (ptr->Relationship == RelationProcessorCore)
		{
			processorCoreCount++;

			ULONG_PTR LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
			u16 bitSetCount = 0;
			ULONG_PTR bitTest = static_cast<ULONG_PTR>(1) << LSHIFT;

			for (DWORD i = 0; i <= LSHIFT; ++i)
			{
				bitSetCount += ((ptr->ProcessorMask & bitTest) ? 1 : 0);
				bitTest /= 2;
			}

			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += bitSetCount;
		}

		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}

	// All logical processors
	coresCount = processorCoreCount;
	threadCount = logicalProcessorCount;

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_dwNumberOfProcessors = sysInfo.dwNumberOfProcessors;
	fUsage = std::make_unique<float[]>(m_dwNumberOfProcessors);
	m_idleTime = std::make_unique<LARGE_INTEGER[]>(m_dwNumberOfProcessors);
	performanceInfo = std::make_unique<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[]>(m_dwNumberOfProcessors);
}

float _processor_info::getCPULoad()
{
	float summa_cpu{};
	for (DWORD i = 0; i < m_dwNumberOfProcessors; i++)
		summa_cpu += fUsage[i];

	return summa_cpu / m_dwNumberOfProcessors;
}

void _processor_info::MTCPULoad()
{
	NtQuerySystemInformation(SystemProcessorPerformanceInformation, performanceInfo.get(),
		sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) * (ULONG)m_dwNumberOfProcessors, nullptr);

	ULONGLONG dwTickCount = GetTickCount64();
	if (!m_dwCount)
		m_dwCount = dwTickCount;

	const ULONGLONG pertime = dwTickCount - m_dwCount;

	for (DWORD i = 0; i < m_dwNumberOfProcessors; i++)
	{
		auto* cpuPerfInfo = &performanceInfo[i];
		cpuPerfInfo->KernelTime.QuadPart -= cpuPerfInfo->IdleTime.QuadPart;

		fUsage[i] = 100.0f - 0.01f * (cpuPerfInfo->IdleTime.QuadPart - m_idleTime[i].QuadPart) / ((dwTickCount - m_dwCount));
		clamp(fUsage[i], 0.f, 100.f);

		m_idleTime[i] = cpuPerfInfo->IdleTime;
	}

	m_dwCount = dwTickCount;
}