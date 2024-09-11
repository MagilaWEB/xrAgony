#include "stdafx.h"
#include <filesystem> 
#include "xrCompress.h"

#pragma comment(lib, "winmm")

xrCompressor::xrCompressor(size_t num_thread, shared_str ltx, size_t max_threads)
{
	corrent_thread = num_thread;
	max_thread = max_threads;

	LPCSTR params = GetCommandLine();

	if (strstr(params, "-128"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 128; // bytes (128Mb)
		console_print("\nINFO: Pack in ~128mb");
	}

	if (strstr(params, "-512"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 512; // bytes (512Mb)
		console_print("\nINFO: Pack in ~512mb");
	}

	if (strstr(params, "-256"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 256; // bytes (256Mb)
		console_print("\nINFO: Pack in ~256mb");
	}

	if (strstr(params, "-768"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 768; // bytes (768Mb)

		console_print("\nINFO: Pack in ~768mb");
	}

	if (strstr(params, "-1024"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 1024; // bytes (1024Mb)
		console_print("\nINFO: Pack in ~1024mb");
	}

	if (strstr(params, "-640"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 640; // bytes (640Mb)
		console_print("\nINFO: Pack in ~640mb");
	}

	config_ltx = new CInifile(ltx.c_str());

	multi_thread = config_ltx->r_bool("header", "multi_thread");
}

xrCompressor::~xrCompressor()
{
	if (pPackHeader)
		FS.r_close(pPackHeader);

	xr_delete(config_ltx);

	// free
	for (auto& it : files_list)
		xr_free(it);
	files_list.clear();

	for (auto& it : folders_list)
		xr_free(it);
	folders_list.clear();

	exclude_exts.clear();
}

bool is_tail(LPCSTR name, LPCSTR tail, const size_t tlen)
{
	LPCSTR p = strstr(name, tail);
	if (!p)
		return false;

	size_t nlen = xr_strlen(name);
	return (p == name + nlen - tlen);
}

bool xrCompressor::testSKIP(LPCSTR path)
{
	std::filesystem::path p(path);

	xr_string name = p.stem().string();
	xr_string extension = p.extension().string();

	//	if (strstr(path,"textures\\lod\\"))				return true;
	//	if (strstr(path,"textures\\det\\"))				return true;

	if (!xr_stricmp(name.c_str(), "level"))
		return false;

	if (xr_stricmp(extension.c_str(), ".thm") && strstr(path, "textures\\terrain\\terrain_") && !is_tail(name.c_str(), "_mask", 5))
		return true;

	if (strstr(path, "textures\\") && is_tail(name.c_str(), "_nmap", 5) && !strstr(name.c_str(), "water_flowing_nmap"))
		return true;

	if (!xr_stricmp(name.c_str(), "build"))
	{
		if (0 == xr_stricmp(extension.c_str(), ".aimap"))
			return true;
		if (0 == xr_stricmp(extension.c_str(), ".cform"))
			return true;
		if (0 == xr_stricmp(extension.c_str(), ".details"))
			return true;
		if (0 == xr_stricmp(extension.c_str(), ".prj"))
			return true;
		if (0 == xr_stricmp(extension.c_str(), ".lights"))
			return false;
	}
	if (!xr_stricmp(name.c_str(), "do_light") && 0 == xr_stricmp(extension.c_str(), ".ltx"))
		return true;

	if (!xr_stricmp(extension.c_str(), ".txt"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".tga"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".db"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".smf"))
		return true;

	if ('~' == extension.c_str()[1])
		return true;
	if ('_' == extension.c_str()[1])
		return true;
	if (!xr_stricmp(extension.c_str(), ".vcproj"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".sln"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".old"))
		return true;
	if (!xr_stricmp(extension.c_str(), ".rc"))
		return true;

	for (const auto& it : exclude_exts)
		if (PatternMatch(extension.c_str(), it.c_str()))
			return true;

	return false;
}

bool xrCompressor::testVFS(LPCSTR path) const
{
	std::filesystem::path p(path);
	xr_string extension = p.extension().string();

	if (bnoFast)
	{
		if (bStoreFiles)
			return true;
		if (!xr_stricmp(extension.c_str(), ".xml"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".ltx"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".script"))
			return false;
	}
	else
	{
		if (!xr_stricmp(extension.c_str(), ".xml"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".ltx"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".script"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".ogf"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".dds"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".ogg"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".xr"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".spawn"))
			return false;

		/*
		* ? Что за бред, .geom и .geomx используются в потоковой передачи, их нельзя сжимать!
		if (!xr_stricmp(extension.c_str(), ".geom"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".geomx"))
			return false;
		*/

		if (!xr_stricmp(extension.c_str(), ".cform"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".details"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".ai"))
			return false;

		if (!xr_stricmp(extension.c_str(), ".omf"))
			return false;

		if (!xr_stricmp(extension.c_str(), ""))
			return false;
	}

	return true;
}

bool xrCompressor::testEqual(LPCSTR path, IReader* base)
{
	bool res = false;
	IReader* test = FS.r_open(path);

	if (test->length() == base->length())
	{
		if (0 == memcmp(test->pointer(), base->pointer(), base->length()))
			res = true;
	}
	FS.r_close(test);
	return res;
}

xrCompressor::ALIAS* xrCompressor::testALIAS(IReader* base, size_t crc, size_t& a_tests)
{
	xr_multimap<size_t, ALIAS>::iterator I = aliases.lower_bound(base->length());

	while (I != aliases.end() && (I->first == base->length()))
	{
		if (I->second.crc == crc)
		{
			a_tests++;
			if (testEqual(I->second.path, base))
				return &I->second;
		}
		I++;
	}
	return nullptr;
}

void xrCompressor::write_file_header(LPCSTR file_name, const size_t& crc, const size_t& ptr, const size_t& size_real, const size_t& size_compressed)
{
	xrCriticalSection::raii mt{ lock };
	size_t file_name_size = (xr_strlen(file_name) + 0) * sizeof(char);
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

	memcpy(buffer, file_name, file_name_size);
	buffer += file_name_size;

	*(size_t*)buffer = ptr;

	fs_desc.w(buffer_start, full_buffer_size);
}

void xrCompressor::CompressOne(LPCSTR path)
{
	filesTOTAL++;

	if (testSKIP(path))
	{
		filesSKIP++;
		console_print(" - a SKIP");
		return;
	}

	string_path fn;
	strconcat(sizeof(fn), fn, target_name.c_str(), "\\", path);

	//	if (::GetFileAttributes(fn)==size_t(-1))
	//	{
	//		filesSKIP	++;
	//		printf		(" - CAN'T OPEN");
	//		Msg			("%-80s	- CAN'T OPEN",path);
	//		return;
	//	}

	IReader* src = FS.r_open(fn);
	if (!src)
	{
		filesSKIP++;
		console_print("%-80s	- CAN'T OPEN", path);
		return;
	}

	bytesSRC += src->length();
	size_t c_crc32 = crc32(src->pointer(), src->length());
	size_t c_ptr = 0;
	size_t c_size_real = 0;
	size_t c_size_compressed = 0;
	size_t a_tests = 0;

	ALIAS* A = testALIAS(src, c_crc32, a_tests);
	//console_print("%3da ", a_tests);
	if (A)
	{
		filesALIAS++;
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

				t_compress.Begin();

				c_size_compressed = c_size_max;

				R_ASSERT(LZO_E_OK ==
					lzo1x_999_compress(
						(u8*)src->pointer(), c_size_real, c_data, (lzo_uintp)&c_size_compressed, c_heap));

				t_compress.End();

				if ((c_size_compressed + 16) >= c_size_real)
				{
					// Failed to compress - revert to VFS
					filesVFS++;
					c_size_compressed = c_size_real;
					fs_pack_writer->w(src->pointer(), c_size_real);
					console_print("%-80s	- No compression (R)", path);
				}
				else
				{
					// Compressed OK - optimize
					if (!bnoFast)
					{
						u8* c_out = xr_alloc<u8>(c_size_real);
						size_t c_orig = c_size_real;
						R_ASSERT(
							LZO_E_OK == lzo1x_optimize(c_data, c_size_compressed, c_out, (lzo_uintp)&c_orig, NULL));
						R_ASSERT(c_orig == c_size_real);
						xr_free(c_out);
					} // bnoFast
					fs_pack_writer->w(c_data, c_size_compressed);

					const auto progress = 100.f * float(c_size_compressed) / float(src->length());
					console_print("%-80s	- OK (%3.1f%%)", path, progress);
				}

				// cleanup
				xr_free(c_data);
			}
			else
			{
				filesVFS++;
				c_size_compressed = c_size_real;
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

void xrCompressor::OpenPack(LPCSTR tgt_folder, int num)
{
	xrCriticalSection::raii mt{ lock };

	VERIFY(nullptr == fs_pack_writer);

	string_path fname;
	string128 s_num;
	string128 s_num_thread;
	LPCSTR params = GetCommandLine();

	strconcat(
		sizeof(fname),
		fname,
		tgt_folder,
		corrent_thread ? "_" : "",
		corrent_thread ? xr_itoa(corrent_thread, s_num_thread, 20) : "",
		num ? "_" : "",
		num ? xr_itoa(num, s_num, 20) : "",
		".db"
	);

	xr_unlink(fname);
	fs_pack_writer = FS.w_open(fname);
	fs_desc.clear();

	bytesSRC = 0;
	bytesDST = 0;
	filesTOTAL = 0;
	filesSKIP = 0;
	filesVFS = 0;
	filesALIAS = 0;

	dwTimeStart = timeGetTime();

	if (config_ltx && config_ltx->section_exist("header"))
	{
		CMemoryWriter W;
		CInifile::Sect& S = config_ltx->r_section("header");
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
	else if (pPackHeader)
	{
		console_print("...Writing pack header");
		fs_pack_writer->open_chunk(CFS_HeaderChunkID);
		fs_pack_writer->w(pPackHeader->pointer(), pPackHeader->length());
		fs_pack_writer->close_chunk();
	}
	else
		console_print("...Pack header not found");

	//	g_dummy_stuff	= _dummy_stuff_subst;

	fs_pack_writer->open_chunk(0);
}

void xrCompressor::SetPackHeaderName(LPCSTR n)
{
	pPackHeader = FS.r_open(n);
	R_ASSERT2(pPackHeader, n);
}

void xrCompressor::ClosePack()
{
	xrCriticalSection::raii mt{ lock };

	fs_pack_writer->close_chunk();
	// save list
	bytesDST = fs_pack_writer->tell();
	console_print("...Writing pack desc");

	fs_pack_writer->w_chunk(1 | CFS_CompressMark, fs_desc.pointer(), fs_desc.size());

	Msg("Data size: %d. Desc size: %d.", bytesDST.load(), fs_desc.size());
	FS.w_close(fs_pack_writer);
	Msg("Pack saved.");
	size_t dwTimeEnd = timeGetTime();

	console_print("\n\nFiles total/skipped/No compression/aliased: %d/%d/%d/%d\nOveral: %dK/%dK, %3.1f%%\nElapsed time: "
		"%d:%d\nCompression speed: %3.1f Mb/s\n\n",
		filesTOTAL.load(), filesSKIP.load(), filesVFS.load(), filesALIAS.load(), bytesDST.load() / 1024, bytesSRC.load() / 1024,
		100.f * float(bytesDST.load()) / float(bytesSRC.load()), ((dwTimeEnd - dwTimeStart.load()) / 1000) / 60,
		((dwTimeEnd - dwTimeStart.load()) / 1000) % 60,
		float((float(bytesDST.load()) / float(1024 * 1024)) / (t_compress.GetElapsed_sec())));

	for (auto& it : aliases)
		xr_free(it.second.path);
	aliases.clear();
}

void xrCompressor::PerformWork()
{
	if (!files_list.empty() && target_name.size())
	{
		string256 caption{""};

		int pack_num = 0;
		OpenPack(file_name.c_str(), pack_num++);

		for (const auto& it : folders_list)
			write_file_header(it, 0, 0, 0, 0);

		if (!bStoreFiles)
			c_heap = xr_alloc<u8>(LZO1X_999_MEM_COMPRESS);

		size_t file_it{ 0 };
		for (auto & file_path : files_list)
		{
			file_it++;

			xr_sprintf(caption, "Compress files: %d/%d - %d%%", file_it, files_list.size(), (file_it * 100) / files_list.size());
			SetWindowText(GetConsoleWindow(), caption);

			if (fs_pack_writer->tell() > XRP_MAX_SIZE)
			{
				ClosePack();
				OpenPack(file_name.c_str(), pack_num++);
			}

			CompressOne(file_path);
		}
		ClosePack();

		if (!bStoreFiles)
			xr_free(c_heap);
		else
			Msg("ERROR: folder not found.");
	}
}

void xrCompressor::ProcessTargetFolder()
{
	//// collect files
	//auto f_list = FS.file_list_open("$target_folder$", FS_ListFiles);
	//R_ASSERT2(f_list, "Unable to open folder!!!");
	//files_list = *f_list;
	//
	//// collect folders
	//auto fr_list = FS.file_list_open("$target_folder$", FS_ListFolders);
	//R_ASSERT2(fr_list, "Unable to open folder!!!");

	//folders_list = *fr_list;

	//// compress
	//PerformWork();
	//// free lists
	//FS.file_list_close(*folders_list);
	//FS.file_list_close(files_list);
}

void xrCompressor::GatherFiles(LPCSTR path)
{
	auto i_list = FS.file_list_open("$target_folder$", path, FS_ListFiles | FS_RootOnly);
	if (!i_list)
	{
		console_print("ERROR: Unable to open file list:%s", path);
		return;
	}

	for (const auto& it : *i_list)
	{
		xr_string tmp_path = xr_string(path) + xr_string(it);

		if (!testSKIP(tmp_path.c_str()))
		{
			
			files_list.push_back(xr_strdup(tmp_path.c_str()));
			console_print("+f: %s", tmp_path.c_str());
		}
		else
		{
			console_print("-f: %s", tmp_path.c_str());
		}
	}
	FS.file_list_close(i_list);
}

bool xrCompressor::IsFolderAccepted(LPCSTR path)
{
	// exclude folders
	if (config_ltx->section_exist("filter_folders"))
	{
		CInifile::Sect& ef_sect = config_ltx->r_section("filter_folders");
		for (const auto& it : ef_sect.Data)
			if (path == strstr(path, it.first.c_str()))
				return true;
	}
	return false;
}

void xrCompressor::ProcessLTX()
{
	if (config_ltx->line_exist("options", "exclude_exts"))
		_SequenceToList(exclude_exts, config_ltx->r_string("options", "exclude_exts"));

	SetFileName(config_ltx->r_string("header", "name_file"));

	if (config_ltx->section_exist("include_folders"))
	{
		CInifile::Sect& if_sect = config_ltx->r_section("include_folders");

		for (const auto& it : if_sect.Data)
		{
			string_path path;
			LPCSTR _path = 0 == xr_strcmp(it.first.c_str(), ".\\") ? "" : it.first.c_str();
			xr_strcpy(path, _path);
			size_t path_len = xr_strlen(path);
			if ((0 != path_len) && (path[path_len - 1] != '\\'))
				xr_strcat(path, "\\");

			if (IsFolderAccepted(path))
				GatherFiles(path);
			else if (config_ltx->line_exist("header", "core_folders_files") && config_ltx->r_bool("header", "core_folders_files"))
				GatherFiles(path);

			auto i_fl_list = FS.file_list_open("$target_folder$", path, FS_ListFolders);
			if (!i_fl_list)
			{
				Msg("ERROR: Unable to open folder list:", path);
				continue;
			}

			size_t it_size = 0;
			for (const auto& it : *i_fl_list)
			{
				if (it_size == (max_thread - 1))
					it_size = 0;

				if (it_size == corrent_thread || !multi_thread)
				{
					xr_string tmp_path = xr_string(path) + xr_string(it);
					if (IsFolderAccepted(tmp_path.c_str()))
					{
						folders_list.push_back(xr_strdup(tmp_path.c_str()));
						
						console_print("+Folder: %s", tmp_path.c_str());
						// collect files
						GatherFiles(tmp_path.c_str());
					}
					else
						console_print("-Folder: %s", tmp_path.c_str());
				}

				it_size++;
			}
			FS.file_list_close(i_fl_list);
		}
	}

	if (config_ltx->section_exist("include_files"))
	{
		CInifile::Sect& if_sect = config_ltx->r_section("include_files");
		size_t it_size = 0;
		for (const auto& it : if_sect.Data)
		{
			if (it_size == (max_thread - 1))
				it_size = 0;

			if (it_size == corrent_thread || !multi_thread)
				files_list.push_back(xr_strdup(it.first.c_str()));
	
			it_size++;
		}
			
	}
}
