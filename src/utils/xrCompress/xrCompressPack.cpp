#include "stdafx.h"
#include "xrCompress.h"
#include <filesystem> 

using namespace std;

extern bool send_console_prosses;

xrCompressorPack::xrCompressorPack(CInifile* _config, xrCompressor* _compressor, size_t _num_thread)
{
	config = move(_config);
	compressor = move(_compressor);
	num_thread = _num_thread;

	string128 s_num;
	strconcat(
		sizeof(name_path),
		name_path,
		compressor->GetTargetName(),
		"\\..\\database\\",
		compressor->GetFileName(),
		num_thread ? "_" : "",
		num_thread ? xr_itoa(num_thread, s_num, 10) : "",
		".db"
	);

	xr_unlink(name_path);
}

xrCompressorPack::~xrCompressorPack()
{
	config = nullptr;
	compressor = nullptr;

	bytesSRC = 0;
	bytesDST = 0;
	filesTOTAL = 0;
	filesSKIP = 0;
	filesVFS = 0;
	filesALIAS = 0;

	files.clear();
}

void xrCompressorPack::StartCompress()
{
	if (files.empty() || !compressor)
		return;

	//string256 caption{ "" };

	OpenPack();

	for (const auto& folder : compressor->GetFolders())
		write_file_header(folder, 0, 0, 0, 0);

	if (!compressor->GetStoreFiles())
		c_heap = xr_alloc<u8>(LZO1X_999_MEM_COMPRESS);

	//size_t file_it{ 0 };
	for (auto& file_path : files)
	{
		//file_it++;

		//xr_sprintf(caption, "Compress files: %d/%d - %d%%", file_it, files.size(), (file_it * 100) / files_list.size());
		//SetWindowText(GetConsoleWindow(), caption);

		//if (fs_pack_writer->tell() > compressor->MAX_SIZE()) //XRP_MAX_SIZE
		//{
		//	ClosePack();
		//	OpenPack(pack_num++);
		//}

		CompressFile(file_path);
	}
	ClosePack();

	if (!compressor->GetStoreFiles())
		xr_free(c_heap);
	else
		console_print("ERROR: folder not found.");
}

void xrCompressorPack::CompressFile(LPCSTR path)
{
	filesTOTAL++;

	//	if (::GetFileAttributes(fn)==size_t(-1))
	//	{
	//		filesSKIP	++;
	//		printf		(" - CAN'T OPEN");
	//		Msg			("%-80s	- CAN'T OPEN",path);
	//		return;
	//	}
	string_path fn;
	strconcat(sizeof(fn), fn, compressor->GetTargetName(), "\\", path);
	IReader* src = FS.r_open(fn);
	if (!src)
	{
		filesSKIP++;
		if (send_console_prosses)
			console_print("%-80s	- CAN'T OPEN", path);
		return;
	}

	bytesSRC += src->length();
	size_t c_crc32 = crc32(src->pointer(), src->length());
	size_t c_ptr = 0;
	size_t c_size_real = 0;
	size_t c_size_compressed = 0;

	ALIAS* A = testALIAS(src, c_crc32);
	if (A)
	{
		filesALIAS++;
		if (send_console_prosses)
			console_print("%-80s	- ALIAS (%s)", path, A->path);

		// Alias found
		c_ptr = A->c_ptr;
		c_size_real = A->c_size_real;
		c_size_compressed = A->c_size_compressed;
	}
	else
	{
		if (testVFS(path))
		{
			filesVFS++;

			// Write into BaseFS
			c_ptr = fs_pack_writer->tell();
			c_size_real = src->length();
			c_size_compressed = src->length();
			fs_pack_writer->w(src->pointer(), c_size_real);
			if (send_console_prosses)
				console_print("%-80s	- No compression", path);
		}
		else
		{ // if(testVFS(path))
			// Compress into BaseFS
			c_ptr = fs_pack_writer->tell();
			c_size_real = src->length();
			if (0 != c_size_real)
			{
				size_t c_size_max = rtc_csize(src->length());
				u8* c_data = xr_alloc<u8>(c_size_max);

				c_size_compressed = c_size_max;

				R_ASSERT(LZO_E_OK ==
					lzo1x_999_compress(
						(u8*)src->pointer(), c_size_real, c_data, (lzo_uintp)&c_size_compressed, c_heap));

				if ((c_size_compressed + 16) >= c_size_real)
				{
					// Failed to compress - revert to VFS
					filesVFS++;
					c_size_compressed = c_size_real;
					fs_pack_writer->w(src->pointer(), c_size_real);
					if (send_console_prosses)
						console_print("%-80s	- Fail compression (R)", path);
				}
				else
				{
					// Compressed OK - optimize
					if (!compressor->GetFastMode())
					{
						u8* c_out = xr_alloc<u8>(c_size_real);
						size_t c_orig = c_size_real;
						R_ASSERT(
							LZO_E_OK == lzo1x_optimize(c_data, c_size_compressed, c_out, (lzo_uintp)&c_orig, NULL));
						R_ASSERT(c_orig == c_size_real);
						xr_free(c_out);
					} // bnoFast

					fs_pack_writer->w(c_data, c_size_compressed);

					if (send_console_prosses)
					{
						const float progress = 100.f - (100.f * float(c_size_compressed) / float(c_size_real));
						console_print("%-80s	- OK compression (%3.1f%%)", path, progress);
					}
				}

				// cleanup
				xr_free(c_data);
			}
			else
			{
				filesVFS++;
				c_size_compressed = c_size_real;
				if (send_console_prosses)
					console_print("%-80s	- EMPTY FILE", path);
			}
		} // test VFS
	} //(A)

	// Write description
	write_file_header(path, c_crc32, c_ptr, c_size_real, c_size_compressed);

	if (!A)
	{
		// Register for future aliasing
		ALIAS R;
		R.path = xr_strdup(fn);
		R.crc = c_crc32;
		R.c_ptr = c_ptr;
		R.c_size_real = c_size_real;
		R.c_size_compressed = c_size_compressed;
		aliases.insert(std::make_pair(R.c_size_real, R));
	}

	FS.r_close(src);
}

void xrCompressorPack::PushFile(LPCSTR path)
{
	string_path fn;
	strconcat(sizeof(fn), fn, compressor->GetTargetName(), "\\", path);

	IReader* src = FS.r_open(fn);
	if (src)
	{
		GatherFilesSIZE.store(GatherFilesSIZE.load() + src->length());
		FS.r_close(src);
	}

	files.push_back(path);
}

void xrCompressorPack::write_file_header(LPCSTR folder, const size_t& crc, const size_t& ptr, const size_t& size_real, const size_t& size_compressed)
{
	size_t file_name_size = (xr_strlen(folder) + 0) * sizeof(char);
	size_t buffer_size = file_name_size + 4 * sizeof(size_t);
	VERIFY(buffer_size <= 65535);
	size_t full_buffer_size = buffer_size + sizeof(u16);
	u8* buffer = (u8*)_alloca(full_buffer_size);
	u8* buffer_start = buffer;
	*(u16*)buffer = (u16)buffer_size;
	buffer += sizeof(u16);

	*(size_t*)buffer = size_real;
	buffer += sizeof(size_t);

	*(size_t*)buffer = size_compressed;
	buffer += sizeof(size_t);

	*(size_t*)buffer = crc;
	buffer += sizeof(size_t);

	memcpy(buffer, folder, file_name_size);
	buffer += file_name_size;

	*(size_t*)buffer = ptr;

	fs_desc.w(buffer_start, full_buffer_size);
}

constexpr LPCSTR FastExtension[] =
{
	".xml",
	".ltx",
	".script"
};

constexpr LPCSTR noFastExtension[] =
{
	".xml",
	".ltx",
	".script",
	".ogf",
	".dds",
	".ogg",
	".xr",
	".spawn",
	".cform",
	".details",
	".ai",
	".omf",
	".ttf",
	""
};

bool xrCompressorPack::testVFS(LPCSTR path) const
{
	std::filesystem::path p(path);
	xr_string extension = p.extension().string();

	if (compressor->GetFastMode())
	{
		if (!compressor->GetStoreFiles())
		{
			for (const auto ex : FastExtension)
				if (!xr_stricmp(extension.c_str(), ex))
					return false;
		}
	}
	else
	{
		for (const auto ex : noFastExtension)
			if (!xr_stricmp(extension.c_str(), ex))
				return false;
	}

	return true;
}

xrCompressorPack::ALIAS* xrCompressorPack::testALIAS(IReader* base, size_t crc)
{
	auto I = aliases.lower_bound(base->length());

	while (I != aliases.end() && (I->first == base->length()))
	{
		if (I->second.crc == crc)
		{
			bool res = false;
			IReader* test = FS.r_open(I->second.path);

			if (test->length() == base->length())
				if (0 == memcmp(test->pointer(), base->pointer(), base->length()))
					res = true;

			FS.r_close(test);

			if (res)
				return &I->second;
		}
		I++;
	}
	return nullptr;
}

void xrCompressorPack::OpenPack()
{
	VERIFY(nullptr == fs_pack_writer);

	console_print("Start compress to file [%s]", name_path);
	fs_pack_writer = FS.w_open(name_path);
	fs_desc.clear();

	bytesSRC = 0;
	bytesDST = 0;
	filesTOTAL = 0;
	filesSKIP = 0;
	filesVFS = 0;
	filesALIAS = 0;

	timer.Start();

	if (config && config->section_exist("header"))
	{
		CMemoryWriter W;
		CInifile::Sect& S = config->r_section("header");
		string4096 buff;
		xr_sprintf(buff, "[%s]", S.Name.c_str());
		W.w_string(buff);

		for (const auto& it : S.Data)
		{
			xr_sprintf(buff, "%s = %s", it.first.c_str(), it.second.c_str());
			W.w_string(buff);
		}

		W.seek(0);

		IReader R(W.pointer(), W.size());

		console_print("...Writing pack header");
		fs_pack_writer->open_chunk(CFS_HeaderChunkID);
		fs_pack_writer->w(R.pointer(), R.length());
		fs_pack_writer->close_chunk();
	}
	/*else if (pPackHeader)
	{
		console_print("...Writing pack header");
		fs_pack_writer->open_chunk(CFS_HeaderChunkID);
		fs_pack_writer->w(pPackHeader->pointer(), pPackHeader->length());
		fs_pack_writer->close_chunk();
	}*/
	else
		console_print("...Pack header not found");

	//	g_dummy_stuff	= _dummy_stuff_subst;

	fs_pack_writer->open_chunk(0);
}

void xrCompressorPack::ClosePack()
{
	fs_pack_writer->close_chunk();
	// save list
	bytesDST = fs_pack_writer->tell();
	console_print("...Writing pack desc");

	fs_pack_writer->w_chunk(1 | CFS_CompressMark, fs_desc.pointer(), fs_desc.size());

	console_print("Data size: %d. Desc size: %d.", bytesDST.load(), fs_desc.size());
	FS.w_close(fs_pack_writer);
	console_print("\nPack saved.");

	console_print("\n\nFiles total/skipped/No compression/aliased: %d/%d/%d/%d\nOveral: %dK/%dK, %3.1f%%\nElapsed time[%3.2f sec]\n",
		filesTOTAL.load(), filesSKIP.load(), filesVFS.load(), filesALIAS.load(), bytesDST.load() / 1024, bytesSRC.load() / 1024,
		100.f * float(bytesDST.load()) / float(bytesSRC.load()), timer.GetElapsed_sec());

	for (auto& it : aliases)
		xr_free(it.second.path);
	aliases.clear();
}