#include "stdafx.h"
#include "xrCompress.h"

#include <filesystem>
#include <iostream>
using namespace std;
namespace fs = filesystem;

constexpr u32 thread_size = 12;

#ifndef MOD_COMPRESS
extern int ProcessDifference();
#endif

int __cdecl main(int argc, char* argv[])
{
	xrDebug::Initialize();
	Core.Initialize("xrCompress", 0, FALSE);
	console_print("\n\nXrCompressor (modifided ""xrAgony)\n----------------------------------------------------------------------------");

	static tbb::task_group parallel;
	static xr_list<xrCompressor*> parallel_Compress;

	LPCSTR params = GetCommandLine();

#ifndef MOD_COMPRESS
	if (strstr(params, "-diff"))
	{
		ProcessDifference();
	}
	else
#endif

#ifndef MOD_XDB
		if (strstr(params, "-pack"))
#endif
	{
#ifndef MOD_COMPRESS
		if (argc < 2)
		{
			console_print("ERROR: u must pass folder name as parameter.");
			console_print("-diff /? option to get information about creating difference.");
			console_print("-fast	- fast compression.");
			console_print("-store	- store files. No compression.");
			console_print("-ltx <file_name.ltx> - pathes to compress.");
			console_print("\n");
			console_print("LTX format:");
			console_print("	[config]");
			console_print("	;<path>	 = <recurse>");
			console_print("	.\\		 = false");
			console_print("	textures	= true");

			Core._destroy();
			return 3;
		}
#endif

		string_path folder;
		strconcat(sizeof(folder), folder, argv[1], "\\");
		_strlwr_s(folder, sizeof(folder));
		console_print("\nCompressing files (%s)...\n", folder);

		FS._initialize(CLocatorAPI::flTargetFolderOnly, folder);
		FS.append_path("$working_folder$", "", 0, false);

		size_t max_threads{ 0 };
		auto send = [&](size_t thread_count, shared_str ltx_name)
		{
			xrCompressor*& C = parallel_Compress.emplace_back(new xrCompressor{ thread_count, ltx_name, max_threads });
			C->SetStoreFiles(NULL != strstr(params, "-store"));
			C->SetFastMode(NULL != strstr(params, "-nocompress"));
			C->SetTargetName(argv[1]);
			C->SetFileName(argv[1]);
			C->ProcessLTX();
		};

		auto melti_thread = [&max_threads](shared_str ltx_name) -> const bool
		{
			if (std::thread::hardware_concurrency() > thread_size)
				max_threads = thread_size;
			else
				max_threads = std::thread::hardware_concurrency();

			CInifile* ini = new CInifile(ltx_name.c_str());
			const bool melti_thread = ini->r_bool("header", "multi_thread");
			if (ini->line_exist("header", "max_threads"))
			{
				const u32 thread_count = ini->r_u32("header", "max_threads");
				if (thread_count <= max_threads)
					max_threads = thread_count;
				else
				{
					console_print("ERROR: It is not possible to set the number of threads to [%d] so currently it is possible to set only [%d]!",
						thread_count, max_threads);
					cin.get();
				}

			}
			xr_delete(ini);

			return melti_thread;
		};

		LPCSTR p = strstr(params, "-ltx");
		if (0 != p)
		{
			string64 ltx_name;
			sscanf(strstr(params, "-ltx ") + 5, "%[^ ] ", ltx_name);

			console_print("Processing %s...", ltx_name);

			if (melti_thread(ltx_name))
				for (size_t it = 0; it < max_threads; it++)
					send(it, ltx_name);
			else
				send(0, ltx_name);
		}
		else
		{
			fs::path path = fs::current_path();
			path.append("compress");
			if (fs::exists(path))
			{
				for (const auto& entry : fs::directory_iterator(path))
				{
					fs::path path_ltx = "compress\\";
					path_ltx.append(entry.path().filename().string());
			
					console_print("\nProcessing %s...\n", path_ltx.string().c_str());

					if (melti_thread(path_ltx.string().c_str()))
						for (size_t it = 0; it < max_threads; it++)
							send(it, path_ltx.string().c_str());
					else
						send(0, path_ltx.string().c_str());
				}
			}
		}

		for (auto& compress : parallel_Compress)
		{
			parallel.run([compress]() {
				compress->PerformWork();
			});
		}

		parallel.wait();

		for (auto& compress : parallel_Compress)
			xr_delete(compress);

		/*LPCSTR p = strstr(params, "-ltx");
		if (0 != p)
		{
			string64 ltx_name;
			sscanf(strstr(params, "-ltx ") + 5, "%[^ ] ", ltx_name);

			CInifile ini(ltx_name);
			printf("Processing %s...\n", ltx_name);
			C.ProcessLTX(ini);
		}
		else
		{
			string64 header_name;
			sscanf(strstr(params, "-header ") + 8, "%[^ ] ", header_name);
			C.SetPackHeaderName(header_name);
			C.ProcessTargetFolder();
		}*/
	}

	cin.get();
	parallel_Compress.clear();

	Core._destroy();
	return 0;
}
