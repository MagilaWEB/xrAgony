#include "stdafx.h"
#include "xrCompress.h"

#include <filesystem>
#include <iostream>
using namespace std;
namespace fs = filesystem;
bool send_console_prosses{ true };

#ifndef MOD_COMPRESS
extern int ProcessDifference();
#endif

int __cdecl main(int argc, char* argv[])
{
	xrDebug::Initialize();
	Core.Initialize("xrCompress", 0, FALSE);
	console_print("\n\nXrCompressor (modifided ""xrAgony)\n----------------------------------------------------------------------------");

	LPCSTR params = GetCommandLine();

	CTimer time_global;
	time_global.Start();

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

		send_console_prosses = NULL == strstr(params, "-nosend_prosses");

		auto send = [&](shared_str ltx_name)
		{
			xrCompressor*& C = xrCompressor::parallel_Compress.emplace_back(new xrCompressor{ltx_name});
			C->SetStoreFiles(NULL != strstr(params, "-store"));
			C->SetFastMode(NULL != strstr(params, "-nocompress"));
			C->SetTargetName(argv[1]);
		};

		LPCSTR p = strstr(params, "-ltx");
		if (0 != p)
		{
			string64 ltx_name;
			sscanf(strstr(params, "-ltx ") + 5, "%[^ ] ", ltx_name);

			console_print("Processing %s...", ltx_name);

			send(ltx_name);
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
					send(path_ltx.string().c_str());
				}
			}
		}

		tbb::parallel_for_each(xrCompressor::parallel_Compress, [](xrCompressor* compress)
		{
			compress->ProcessLTX();
			compress->PerformWork();
		});

		for (auto& compress : xrCompressor::parallel_Compress)
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

	console_print("--- Total execution time [%3.2f sec, %3.2f min]! ---",
		time_global.GetElapsed_sec(), time_global.GetElapsed_sec() / 60);

	console_print("--- Please press any key to close the window! ---");

	cin.get();
	xrCompressor::parallel_Compress.clear();

	Core._destroy();
	return 0;
}
