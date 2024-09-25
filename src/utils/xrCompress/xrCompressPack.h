#pragma once

class xrCompressor;

class xrCompressorPack final
{
	CMemoryWriter fs_desc;

	xrCompressor* compressor{ nullptr };
	size_t num_thread{ 0 };

	string_path name_path;

	IWriter* fs_pack_writer{ nullptr };
	u8* c_heap{ nullptr };

	std::atomic_uint bytesSRC{ 0 };
	std::atomic_uint bytesDST{ 0 };
	std::atomic_uint filesTOTAL{ 0 };
	std::atomic_uint filesSKIP{ 0 };
	std::atomic_uint filesVFS{ 0 };
	std::atomic_uint filesALIAS{ 0 };
	std::atomic_uint GatherFilesSIZE{ 0 };

	CTimer timer;

	xr_vector<LPCSTR> files;

	struct ALIAS
	{
		LPCSTR path;
		size_t crc{ 0 };
		size_t c_ptr{ 0 };
		size_t c_size_real{ 0 };
		size_t c_size_compressed{ 0 };
	};
	xr_map<size_t, ALIAS> aliases;

public:
	xrCompressorPack(xrCompressor* _compressor, size_t _num_thread);
	~xrCompressorPack();

	void StartCompress();

	void PushFile(LPCSTR path);

	IC size_t GetGatherFilesSize() const
	{
		return GatherFilesSIZE.load();
	};

	IC const string_path& NamePath()
	{
		return name_path;
	};

	

private:
	void CompressFile(LPCSTR path);
	void write_file_header(LPCSTR folder, const size_t& crc, const size_t& ptr, const size_t& size_real, const size_t& size_compressed);
	bool testVFS(LPCSTR path) const;
	ALIAS* testALIAS(IReader* base, size_t crc);
	void OpenPack(size_t num);
	void ClosePack();
};