#include "stdafx.h"
#include "xrCompress.h"
#include <cstdio>

// Edit source: LostAlphaRus

xrCompressor::xrCompressor()
	: fs_pack_writer(NULL), bnoFast(false), files_list(NULL), folders_list(NULL), bStoreFiles(false), pPackHeader(NULL),
	  config_ltx(NULL)
{
	bytesSRC = 0;
	bytesDST = 0;
	filesTOTAL = 0;
	filesSKIP = 0;
	filesVFS = 0;
	filesALIAS = 0;
	c_heap = NULL;
	dwTimeStart = 0;

	LPCSTR params = GetCommandLine();

	if (strstr(params, "-128"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 128; // bytes (128Mb)
		printf("\nINFO: Pack in ~128mb");
	}

	if (strstr(params, "-512"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 512; // bytes (512Mb)
		printf("\nINFO: Pack in ~512mb");
	}

	if (strstr(params, "-256"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 256; // bytes (256Mb)
		printf("\nINFO: Pack in ~256mb");
	}

	if (strstr(params, "-768"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 768; // bytes (768Mb)

		printf("\nINFO: Pack in ~768mb");
	}

	if (strstr(params, "-1024"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 1024; // bytes (1024Mb)
		printf("\nINFO: Pack in ~1024mb");
	}

	if (strstr(params, "-640"))
	{
		XRP_MAX_SIZE = 1024 * 1024 * 640; // bytes (640Mb)
		printf("\nINFO: Pack in ~640mb");
	}
}

xrCompressor::~xrCompressor()
{
	if (pPackHeader)
		FS.r_close(pPackHeader);
}

bool is_tail(LPCSTR name, LPCSTR tail, const u32 tlen)
{
	LPCSTR p = strstr(name, tail);
	if (!p)
		return false;

	u32 nlen = xr_strlen(name);
	return (p == name + nlen - tlen);
}

bool xrCompressor::testSKIP(LPCSTR path)
{
	string256 p_name;
	string256 p_ext;
	_splitpath(path, 0, 0, p_name, p_ext);

	//	if (strstr(path,"textures\\lod\\"))				return true;
	//	if (strstr(path,"textures\\det\\"))				return true;

	if (0 == xr_stricmp(p_name, "level"))
		return false;

	if (xr_stricmp(p_ext, ".thm") && strstr(path, "textures\\terrain\\terrain_") && !is_tail(p_name, "_mask", 5))
		return true;

	if (strstr(path, "textures\\") && is_tail(p_name, "_nmap", 5) && !strstr(p_name, "water_flowing_nmap"))
		return true;

	if (0 == xr_stricmp(p_name, "build"))
	{
		if (0 == xr_stricmp(p_ext, ".aimap"))
			return true;
		if (0 == xr_stricmp(p_ext, ".cform"))
			return true;
		if (0 == xr_stricmp(p_ext, ".details"))
			return true;
		if (0 == xr_stricmp(p_ext, ".prj"))
			return true;
		if (0 == xr_stricmp(p_ext, ".lights"))
			return false;
	}
	if (0 == xr_stricmp(p_name, "do_light") && 0 == xr_stricmp(p_ext, ".ltx"))
		return true;

	if (0 == xr_stricmp(p_ext, ".txt"))
		return true;
	if (0 == xr_stricmp(p_ext, ".tga"))
		return true;
	if (0 == xr_stricmp(p_ext, ".db"))
		return true;
	if (0 == xr_stricmp(p_ext, ".smf"))
		return true;

	if ('~' == p_ext[1])
		return true;
	if ('_' == p_ext[1])
		return true;
	if (0 == xr_stricmp(p_ext, ".vcproj"))
		return true;
	if (0 == xr_stricmp(p_ext, ".sln"))
		return true;
	if (0 == xr_stricmp(p_ext, ".old"))
		return true;
	if (0 == xr_stricmp(p_ext, ".rc"))
		return true;

	for (const auto& it : exclude_exts)
		if (PatternMatch(p_ext, it.c_str()))
			return true;

	return false;
}

bool xrCompressor::testVFS(LPCSTR path)
{
	string256 p_ext;
	_splitpath(path, 0, 0, 0, p_ext);

	if (bnoFast)
	{
		if (bStoreFiles)
			return (true);
		{
			if (!xr_stricmp(p_ext, ".xml"))
				return (false);

			if (!xr_stricmp(p_ext, ".ltx"))
				return (FALSE);

			if (!xr_stricmp(p_ext, ".script"))
				return (FALSE);
		}
	}
	else
	{
		if (!xr_stricmp(p_ext, ".xml"))
			return (false);

		if (!xr_stricmp(p_ext, ".ltx"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".script"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".ogf"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".dds"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".ogg"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".xr"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".spawn"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".geom"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".geomx"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".cform"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".details"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".ai"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ".omf"))
			return (FALSE);

		if (!xr_stricmp(p_ext, ""))
			return (FALSE);
	}

	return (TRUE);
}

bool xrCompressor::testEqual(LPCSTR path, IReader* base)
{
	bool res = false;
	IReader* test = FS.r_open(path);

	if (test->length() == base->length())
	{
		if (0 == memcmp(test->pointer(), base->pointer(), base->length()))
			res = TRUE;
	}
	FS.r_close(test);
	return res;
}

xrCompressor::ALIAS* xrCompressor::testALIAS(IReader* base, u32 crc, u32& a_tests)
{
	xr_multimap<u32, ALIAS>::iterator I = aliases.lower_bound(base->length());

	while (I != aliases.end() && (I->first == base->length()))
	{
		if (I->second.crc == crc)
		{
			a_tests++;
			if (testEqual(I->second.path, base))
			{
				return &I->second;
			}
		}
		I++;
	}
	return NULL;
}

void xrCompressor::write_file_header(
	LPCSTR file_name, const u32& crc, const u32& ptr, const u32& size_real, const u32& size_compressed)
{
	u32 file_name_size = (xr_strlen(file_name) + 0) * sizeof(char);
	u32 buffer_size = file_name_size + 4 * sizeof(u32);
	VERIFY(buffer_size <= 65535);
	u32 full_buffer_size = buffer_size + sizeof(u16);
	u8* buffer = (u8*)_alloca(full_buffer_size);
	u8* buffer_start = buffer;
	*(u16*)buffer = (u16)buffer_size;
	buffer += sizeof(u16);

	*(u32*)buffer = size_real;
	buffer += sizeof(u32);

	*(u32*)buffer = size_compressed;
	buffer += sizeof(u32);

	*(u32*)buffer = crc;
	buffer += sizeof(u32);

	memcpy(buffer, file_name, file_name_size);
	buffer += file_name_size;

	*(u32*)buffer = ptr;

	fs_desc.w(buffer_start, full_buffer_size);
}

void xrCompressor::CompressOne(LPCSTR path)
{
	filesTOTAL++;

	if (testSKIP(path))
	{
		filesSKIP++;
		printf(" - a SKIP");
		Msg("%-80s	- SKIP", path);
		return;
	}

	string_path fn;
	strconcat(sizeof(fn), fn, target_name.c_str(), "\\", path);

	//	if (::GetFileAttributes(fn)==u32(-1))
	//	{
	//		filesSKIP	++;
	//		printf		(" - CAN'T OPEN");
	//		Msg			("%-80s	- CAN'T OPEN",path);
	//		return;
	//	}

	IReader* src = FS.r_open(fn);
	if (0 == src)
	{
		filesSKIP++;
		printf(" - CAN'T OPEN");
		Msg("%-80s	- CAN'T OPEN", path);
		return;
	}

	bytesSRC += src->length();
	u32 c_crc32 = crc32(src->pointer(), src->length());
	u32 c_ptr = 0;
	u32 c_size_real = 0;
	u32 c_size_compressed = 0;
	u32 a_tests = 0;

	ALIAS* A = testALIAS(src, c_crc32, a_tests);
	printf("%3da ", a_tests);
	if (A)
	{
		filesALIAS++;
		printf("ALIAS");
		Msg("%-80s	- ALIAS (%s)", path, A->path);

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
			printf("No compression");
			Msg("%-80s	- No compression", path);
		}
		else
		{ // if(testVFS(path))
			// Compress into BaseFS
			c_ptr = fs_pack_writer->tell();
			c_size_real = src->length();
			if (0 != c_size_real)
			{
				u32 c_size_max = rtc_csize(src->length());
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
					printf("No compression (R)");
					Msg("%-80s	- No compression (R)", path);
				}
				else
				{
					// Compressed OK - optimize
					if (!bnoFast)
					{
						u8* c_out = xr_alloc<u8>(c_size_real);
						u32 c_orig = c_size_real;
						R_ASSERT(
							LZO_E_OK == lzo1x_optimize(c_data, c_size_compressed, c_out, (lzo_uintp)&c_orig, NULL));
						R_ASSERT(c_orig == c_size_real);
						xr_free(c_out);
					} // bnoFast
					fs_pack_writer->w(c_data, c_size_compressed);
					printf("%3.1f%%", 100.f * float(c_size_compressed) / float(src->length()));
					Msg("%-80s	- OK (%3.1f%%)", path, 100.f * float(c_size_compressed) / float(src->length()));
				}

				// cleanup
				xr_free(c_data);
			}
			else
			{
				filesVFS++;
				c_size_compressed = c_size_real;
				printf("No compression (R)");
				Msg("%-80s	- EMPTY FILE", path);
			}
		} // test VFS
	} //(A)

	// Write description
	write_file_header(path, c_crc32, c_ptr, c_size_real, c_size_compressed);

	if (0 == A)
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
	VERIFY(0 == fs_pack_writer);

	string_path fname;
	string128 s_num;
	LPCSTR params = GetCommandLine();

	if (strstr(params, "-xdb"))
	{
		strconcat(sizeof(fname), fname, tgt_folder, ".xdb", xr_itoa(num, s_num, 20));
	}
	if (strstr(params, "-db"))
	{
		strconcat(sizeof(fname), fname, tgt_folder, ".db", xr_itoa(num, s_num, 20));
	}

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

		printf("...Writing pack header\n");
		fs_pack_writer->open_chunk(CFS_HeaderChunkID);
		fs_pack_writer->w(R.pointer(), R.length());
		fs_pack_writer->close_chunk();
	}
	else if (pPackHeader)
	{
		printf("...Writing pack header\n");
		fs_pack_writer->open_chunk(CFS_HeaderChunkID);
		fs_pack_writer->w(pPackHeader->pointer(), pPackHeader->length());
		fs_pack_writer->close_chunk();
	}
	else
		printf("...Pack header not found\n");

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
	fs_pack_writer->close_chunk();
	// save list
	bytesDST = fs_pack_writer->tell();
	Msg("...Writing pack desc");

	fs_pack_writer->w_chunk(1 | CFS_CompressMark, fs_desc.pointer(), fs_desc.size());

	Msg("Data size: %d. Desc size: %d.", bytesDST, fs_desc.size());
	FS.w_close(fs_pack_writer);
	Msg("Pack saved.");
	u32 dwTimeEnd = timeGetTime();
	printf(
		"\n\nFiles total/skipped/No compression/aliased: %d/%d/%d/%d\nOveral: %dK/%dK, %3.1f%%\nElapsed time: "
		"%d:%d\nCompression speed: %3.1f Mb/s",
		filesTOTAL, filesSKIP, filesVFS, filesALIAS, bytesDST / 1024, bytesSRC / 1024,
		100.f * float(bytesDST) / float(bytesSRC), ((dwTimeEnd - dwTimeStart) / 1000) / 60,
		((dwTimeEnd - dwTimeStart) / 1000) % 60,
		float((float(bytesDST) / float(1024 * 1024)) / (t_compress.GetElapsed_sec())));
	Msg("\n\nFiles total/skipped/No compression/aliased: %d/%d/%d/%d\nOveral: %dK/%dK, %3.1f%%\nElapsed time: "
		"%d:%d\nCompression speed: %3.1f Mb/s\n\n",
		filesTOTAL, filesSKIP, filesVFS, filesALIAS, bytesDST / 1024, bytesSRC / 1024,
		100.f * float(bytesDST) / float(bytesSRC), ((dwTimeEnd - dwTimeStart) / 1000) / 60,
		((dwTimeEnd - dwTimeStart) / 1000) % 60,
		float((float(bytesDST) / float(1024 * 1024)) / (t_compress.GetElapsed_sec())));

	for (auto& it : aliases)
		xr_free(it.second.path);
	aliases.clear();
}

void xrCompressor::PerformWork()
{
	if (!files_list->empty() && target_name.size())
	{
		string256 caption;

		int pack_num = 0;
		OpenPack(target_name.c_str(), pack_num++);

		for (const auto& it : *folders_list)
			write_file_header(it, 0, 0, 0, 0);

		if (!bStoreFiles)
			c_heap = xr_alloc<u8>(LZO1X_999_MEM_COMPRESS);

		for (u32 it = 0; it < files_list->size(); it++)
		{
			xr_sprintf(caption, "Compress files: %d/%d - %d%%", it + 1, files_list->size(),
				((it + 1) * 100) / files_list->size());
			SetWindowText(GetConsoleWindow(), caption);
			printf("\n%-80s	", (*files_list)[it]);

			if (fs_pack_writer->tell() > XRP_MAX_SIZE)
			{
				ClosePack();
				OpenPack(target_name.c_str(), pack_num++);
			}

			CompressOne((*files_list)[it]);
		}
		ClosePack();

		if (!bStoreFiles)
		{
			xr_free(c_heap);
		}
		else
		{
			Msg("ERROR: folder not found.");
		}
	}
}

void xrCompressor::ProcessTargetFolder()
{
	// collect files
	files_list = FS.file_list_open("$target_folder$", FS_ListFiles);
	R_ASSERT2(files_list, "Unable to open folder!!!");
	// collect folders
	folders_list = FS.file_list_open("$target_folder$", FS_ListFolders);
	R_ASSERT2(folders_list, "Unable to open folder!!!");
	// compress
	PerformWork();
	// free lists
	FS.file_list_close(folders_list);
	FS.file_list_close(files_list);
}

void xrCompressor::GatherFiles(LPCSTR path)
{
	auto i_list = FS.file_list_open("$target_folder$", path, FS_ListFiles | FS_RootOnly);
	if (!i_list)
	{
		Msg("ERROR: Unable to open file list:%s", path);
		return;
	}

	for (const auto& it : *i_list)
	{
		xr_string tmp_path = xr_string(path) + xr_string(it);

		if (!testSKIP(tmp_path.c_str()))
		{
			printf("%s\n", tmp_path.c_str());
			files_list->push_back(xr_strdup(tmp_path.c_str()));
		}
		else
		{
			Msg("-f: %s", tmp_path.c_str());
		}
	}
	FS.file_list_close(i_list);
}

bool xrCompressor::IsFolderAccepted(CInifile& ltx, LPCSTR path, BOOL& recurse)
{
	// exclude folders
	if (ltx.section_exist("exclude_folders"))
	{
		CInifile::Sect& ef_sect = ltx.r_section("exclude_folders");
		for (const auto& it : ef_sect.Data)
		{
			recurse = CInifile::isBool(it.second.c_str());
			if (recurse)
			{
				if (path == strstr(path, it.first.c_str()))
					return false;
			}
			else
			{
				if (0 == xr_strcmp(path, it.first.c_str()))
					return false;
			}
		}
	}
	return true;
}

void xrCompressor::ProcessLTX(CInifile& ltx)
{
	config_ltx = &ltx;

	if (ltx.line_exist("options", "exclude_exts"))
		_SequenceToList(exclude_exts, ltx.r_string("options", "exclude_exts"));

	files_list = new xr_vector<char*>();
	folders_list = new xr_vector<char*>();

	if (ltx.section_exist("include_folders"))
	{
		CInifile::Sect& if_sect = ltx.r_section("include_folders");

		for (const auto& it : if_sect.Data)
		{
			BOOL ifRecurse = CInifile::isBool(it.second.c_str());
			u32 folder_mask = FS_ListFolders | (ifRecurse ? 0 : FS_RootOnly);

			string_path path;
			LPCSTR _path = 0 == xr_strcmp(it.first.c_str(), ".\\") ? "" : it.first.c_str();
			xr_strcpy(path, _path);
			u32 path_len = xr_strlen(path);
			if ((0 != path_len) && (path[path_len - 1] != '\\'))
				xr_strcat(path, "\\");

			Msg("");
			Msg("Processing folder: '%s'", path);
			BOOL efRecurse;
			BOOL val = IsFolderAccepted(ltx, path, efRecurse);
			if (val || (!val && !efRecurse))
			{
				if (val)
					GatherFiles(path);

				auto i_fl_list = FS.file_list_open("$target_folder$", path, folder_mask);
				if (!i_fl_list)
				{
					Msg("ERROR: Unable to open folder list:", path);
					continue;
				}

				for (const auto& it : *i_fl_list)
				{
					xr_string tmp_path = xr_string(path) + xr_string(it);
					bool val = IsFolderAccepted(ltx, tmp_path.c_str(), efRecurse);
					if (val)
					{
						folders_list->push_back(xr_strdup(tmp_path.c_str()));
						Msg("+F: %s", tmp_path.c_str());
						// collect files
						if (ifRecurse)
							GatherFiles(tmp_path.c_str());
					}
					else
					{
						Msg("-F: %s", tmp_path.c_str());
					}
				}
				FS.file_list_close(i_fl_list);
			}
			else
			{
				Msg("-F: %s", path);
			}
		}
	} // if(ltx.section_exist("include_folders"))

	if (ltx.section_exist("include_files"))
	{
		CInifile::Sect& if_sect = ltx.r_section("include_files");
		for (const auto& it : if_sect.Data)
			files_list->push_back(xr_strdup(it.first.c_str()));
	}

	PerformWork();

	// free
	for (auto& it : *files_list)
		xr_free(it);
	xr_delete(files_list);

	for (auto& it : *folders_list)
		xr_free(it);
	xr_delete(folders_list);

	exclude_exts.clear();
}
