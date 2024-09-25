#pragma once
#include "xrCompressPack.h"
#include "../xrCore/FTimer.h"

class xrCompressor final
{
	CInifile* config_ltx{ nullptr };

	bool multi_thread{ false };

	std::atomic_bool bnoFast{ false };
	std::atomic_bool bStoreFiles{ false };

	shared_str target_name{ nullptr };
	shared_str file_name{ nullptr };

	xr_vector<LPCSTR> files_list;
	xr_vector<LPCSTR> folders_list;
	xr_vector<shared_str> exclude_exts;
	xr_vector<xrCompressorPack*> PackCompress;
	CInifile::Sect filter_folders;

	size_t XRP_MAX_SIZE{ 0 };
	size_t THREAD_MAX_MEMORY_SIZE{ 0 };

	enum ConsoleColor
	{
		Black = 0,
		Blue = 1,
		Green = 2,
		Cyan = 3,
		Red = 4,
		Magenta = 5,
		Brown = 6,
		LightGray = 7,
		DarkGray = 8,
		LightBlue = 9,
		LightGreen = 10,
		LightCyan = 11,
		LightRed = 12,
		LightMagenta = 13,
		Yellow = 14,
		White = 15
	};

	bool testSKIP(LPCSTR path);
	bool IsFolderAccepted(LPCSTR path);

	void GatherFiles(LPCSTR folder);

	//std::atomic_uint bytesSRC{ 0 };
	//std::atomic_uint bytesDST{ 0 };
	//std::atomic_uint filesTOTAL{ 0 };
	//std::atomic_uint filesSKIP{ 0 };
	//std::atomic_uint filesVFS{ 0 };
	//std::atomic_uint filesALIAS{ 0 };
	//size_t GatherFilesSIZE{ 0 };
	//u8* c_heap{ nullptr };

	//CTimer timer;
public:
	IC static xr_list<xrCompressor*> parallel_Compress;

public:
	xrCompressor(shared_str ltx);
	~xrCompressor();

	IC CInifile* GetConfig()
	{
		return config_ltx;
	};

	IC bool IsMultiThread() const
	{
		return multi_thread;
	};

	IC void SetFastMode(bool b)
	{
		bnoFast = b;
	};

	IC bool GetFastMode() const
	{
		return bnoFast;
	};

	IC void SetStoreFiles(bool b) {
		bStoreFiles = b;
	};

	IC bool GetStoreFiles() const
	{
		return bStoreFiles;
	};

	IC void SetMaxVolumeSize(size_t sz)
	{
		XRP_MAX_SIZE = sz;
	};

	IC void SetTargetName(LPCSTR n)
	{
		target_name = n;
	};

	IC LPCSTR GetTargetName() const
	{
		return target_name.c_str();
	};

	void SetFileName(LPCSTR n)
	{
		file_name = n;
	}

	LPCSTR GetFileName() const
	{
		return file_name.c_str();
	}

	IC auto& GetFolders()
	{
		return folders_list;
	}

	IC size_t MAX_SIZE() const
	{
		return XRP_MAX_SIZE;
	};

	void SetPackHeaderName(LPCSTR n);

	void ProcessLTX();
	void ProcessTargetFolder();
	void PerformWork();
};

static xrCriticalSection lock_print;

template <typename... Args>
void console_print(Args&&... args)
{
	xrCriticalSection::raii mt{ lock_print };
	Msg(std::forward<Args>(args)...);
	printf(std::forward<Args>(args)...);
	printf("\n");
}