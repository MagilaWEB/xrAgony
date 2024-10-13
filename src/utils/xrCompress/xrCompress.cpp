#include "stdafx.h"
#include "xrCompress.h"
#include <filesystem> 

#pragma comment(lib, "winmm")

extern bool send_console_prosses;

xrCompressor::xrCompressor(shared_str ltx)
{
	config_ltx = new CInifile(ltx.c_str());

	if (config_ltx->line_exist("header", "max_size"))
	{
		const size_t max_size = config_ltx->r_u32("header", "max_size");
		XRP_MAX_SIZE = 1024 * 1024 * max_size;
		console_print("\nINFO: Pack in ~%dmb", max_size);
	}
	else
		XRP_MAX_SIZE = 1024 * 1024 * 1024; // bytes (1024Mb)

	if (config_ltx->line_exist("header", "thread_max_size_memory"))
	{
		const size_t max_size = config_ltx->r_u32("header", "thread_max_size_memory");
		THREAD_MAX_MEMORY_SIZE = 1024 * 1024 * max_size;
		console_print("\nINFO: Pack in threads ~%dmb", max_size);
	}
	else
		THREAD_MAX_MEMORY_SIZE = 1024 * 1024 * 128; // bytes (128Mb)

	multi_thread = config_ltx->r_bool("header", "multi_thread");

	if (config_ltx->line_exist("header", "name_file"))
		SetFileName(config_ltx->r_string("header", "name_file"));
		
}

xrCompressor::~xrCompressor()
{
	//if (pPackHeader)
	//	FS.r_close(pPackHeader);

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

void xrCompressor::SetPackHeaderName(LPCSTR n)
{
	/*pPackHeader = FS.r_open(n);
	R_ASSERT2(pPackHeader, n);*/
}

void xrCompressor::PerformWork()
{
	if (IsMultiThread())
	{
		size_t count_compress = 0;
		auto compress = PackCompress.emplace_back(new xrCompressorPack{ this, count_compress++ });

		for (LPCSTR& file : files_list)
		{
			if (compress->GetGatherFilesSize() > THREAD_MAX_MEMORY_SIZE)
				compress = PackCompress.emplace_back(new xrCompressorPack{ this, count_compress++ });

			compress->PushFile(file);
		}
		
		tbb::parallel_for_each(PackCompress, [](xrCompressorPack* compress)
		{
			compress->StartCompress();
		});

		for (auto& compress : PackCompress)
			xr_delete(compress);

		PackCompress.clear();

		return;
	}

	xrCompressorPack* compress = new xrCompressorPack{ this, 0 };

	for (LPCSTR& file : files_list)
		compress->PushFile(file);

	compress->StartCompress();

	xr_delete(compress);
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
	auto v_files = FS.file_list_open("$target_folder$", path, FS_ListFiles | FS_RootOnly);

	if (!v_files)
	{
		console_print("ERROR: Unable to open file list:%s", path);
		return;
	}

	for(auto & it : *v_files)
	{
		LPCSTR file = xr_strdup(xr_string(xr_string(path) + xr_string(it)).c_str());
		if (!testSKIP(file))
		{
			files_list.push_back(file);
			if (send_console_prosses)
				console_print("+File: %s", file);
		}
		else
		{
			if (send_console_prosses)
				console_print("-File: %s", file);
		}
	}

	FS.file_list_close(v_files);
}

bool xrCompressor::IsFolderAccepted(LPCSTR path)
{
	// exclude folders
	if (filter_folders.Data.empty() && config_ltx->section_exist("filter_folders"))
		filter_folders = config_ltx->r_section("filter_folders");

	if(!filter_folders.Data.empty())
		for (const auto& it : filter_folders.Data)
			if (path == strstr(path, it.first.c_str()))
				return true;

	return false;
}

void xrCompressor::ProcessLTX()
{
	if (config_ltx->line_exist("options", "exclude_exts"))
		_SequenceToList(exclude_exts, config_ltx->r_string("options", "exclude_exts"));

	if (!file_name)
		SetFileName(GetTargetName());

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
				console_print("ERROR: Unable to open folder list:", path);
				continue;
			}

			for (LPCSTR it : *i_fl_list)
			{
				LPCSTR forder = xr_strdup(xr_string(xr_string(path) + xr_string(it)).c_str());
				if (IsFolderAccepted(forder))
				{
					folders_list.push_back(forder);

					if (send_console_prosses)
						console_print("+Folder: %s", forder);
					// collect files
					GatherFiles(forder);
				}
				else if (send_console_prosses)
					console_print("-Folder: %s", forder);
			}

			FS.file_list_close(i_fl_list);
		}
	}

	if (config_ltx->section_exist("include_files"))
	{
		CInifile::Sect& if_sect = config_ltx->r_section("include_files");
		for (const auto& it : if_sect.Data)
			files_list.push_back(xr_strdup(it.first.c_str()));
	}
}
