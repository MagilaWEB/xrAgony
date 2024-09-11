#pragma once

class xrCompressor final
{
	size_t corrent_thread{ 0 };
	size_t max_thread{ 0 };
	CMemoryWriter fs_desc;

	bool multi_thread{ false };
	std::atomic_bool bnoFast{ false };
	std::atomic_bool bStoreFiles{ false };
	IWriter* fs_pack_writer{ nullptr };

	shared_str compress_forder{ nullptr };
	shared_str target_name{ nullptr };
	shared_str file_name{ nullptr };

	IReader* pPackHeader{ nullptr };
	CInifile* config_ltx{ nullptr };
	xr_vector<LPCSTR> files_list;
	xr_vector<LPCSTR> folders_list;

	xrCriticalSection lock;

	size_t XRP_MAX_SIZE{ 0 };

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

	struct ALIAS
	{
		LPCSTR path;
		size_t crc{ 0 };
		size_t c_ptr{ 0 };
		size_t c_size_real{ 0 };
		size_t c_size_compressed{ 0 };
	};
	xr_multimap<size_t, ALIAS> aliases;

	xr_vector<shared_str> exclude_exts;
	bool testSKIP(LPCSTR path);
	ALIAS* testALIAS(IReader* base, size_t crc, size_t& a_tests);
	bool testEqual(LPCSTR path, IReader* base);
	bool testVFS(LPCSTR path) const;
	bool IsFolderAccepted(LPCSTR path);

	void GatherFiles(LPCSTR folder);
	void write_file_header(
		LPCSTR file_name, const size_t& crc, const size_t& ptr, const size_t& size_real, const size_t& size_compressed);
	void ClosePack();
	void OpenPack(LPCSTR tgt_folder, int num);

	void CompressOne(LPCSTR path);

	std::atomic_uint bytesSRC{ 0 };
	std::atomic_uint bytesDST{ 0 };
	std::atomic_uint filesTOTAL{ 0 };
	std::atomic_uint filesSKIP{ 0 };
	std::atomic_uint filesVFS{ 0 };
	std::atomic_uint filesALIAS{ 0 };
	CStatTimer t_compress;
	u8* c_heap{ nullptr };
	std::atomic_uint dwTimeStart{ 0 };

public:
	xrCompressor(size_t num_thread, shared_str ltx, size_t max_threads);
	~xrCompressor();
	void PerformWork();
	void SetFastMode(bool b) { bnoFast = b; }
	void SetStoreFiles(bool b) { bStoreFiles = b; }
	void SetMaxVolumeSize(size_t sz) { XRP_MAX_SIZE = sz; }
	void SetTargetName(LPCSTR n) { target_name = n; }
	void SetFileName(LPCSTR n) { file_name = n; }
	void SetCompressForder(LPCSTR n) { compress_forder = n; }
	void SetPackHeaderName(LPCSTR n);

	void ProcessLTX();
	void ProcessTargetFolder();
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