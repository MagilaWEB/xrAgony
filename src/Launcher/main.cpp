#include <windows.h>
#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <filesystem>
#include <intrin.h>
#include <bitset>

using namespace std;
bool logs{ false };

//bool IsAVX2()
//{
//	int nIds_{};
//	vector<array<int, 4>> data_{};
//	array<int, 4> cpui{};
//
//	__cpuid(cpui.data(), 0);
//	nIds_ = cpui[0];
//
//	for (int i = 0; i <= nIds_; ++i)
//	{
//		__cpuidex(cpui.data(), i, 0);
//		data_.push_back(cpui);
//	}
//
//	if (nIds_ >= 7)
//		return bitset<32>(data_[7][1])[5];
//
//	return false;
//}

string readFile(const string& fileName)
{
	ifstream file(fileName);
	if (!file)
		return "";
	string str = string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();
	return str;
}

void RemoveСatalog(const filesystem::path& ptr)
{
	if (filesystem::exists(ptr))
	{
		for (const auto& entry : filesystem::directory_iterator(ptr))
		{
			if (filesystem::exists(entry.path()))
			{
				if (filesystem::is_regular_file(entry.path()))
					filesystem::remove(entry.path());
				else
					RemoveСatalog(entry.path());
			}

			if (logs)
				cout << entry.path() << endl;
		}

		filesystem::remove(ptr);
	}
};

void DeleteFiles(const string& fileName)
{
	ifstream file(fileName);
	if (!file)
		return;

	string str;

	while (getline(file, str))
	{
		if (filesystem::is_regular_file(str))
			filesystem::remove(str);
		else
			RemoveСatalog(str);
	}

	file.close();
	//filesystem::remove(fileName);
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prevInst, char* commandLine, int cmdShow)
{
	string pathToExe = "bin\\xrEngine.exe";
	setlocale(LC_ALL, "Russian");

	// additional information
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// combine the rest of the arguments into a single string
	string command_line = commandLine;
	// Command line from the file.
	string command_line_file = readFile("launch_command.dat");
	if (command_line_file != "")
		command_line.append(command_line_file);

	logs = command_line.find("-launchinglog") != string::npos;

	if (logs)
		cout << command_line << endl;

	//Delete Files
	DeleteFiles("delete_files.dat");

	int state = MessageBox(
		NULL,
		"Нажмите (Нет), чтобы запустить игру на DX11, нажмите (Да), чтобы запустить игру на DXVK (Vulkan).\
		\nPress (No) to launch the game on DX11, press (Yes) to launch the game on DXVK (Vulkan).\n",
		"Режим запуска.",
		MB_YESNO |
		MB_DEFBUTTON1 |
		MB_ICONINFORMATION |
		MB_DEFAULT_DESKTOP_ONLY
	);

	if (state == IDNO)
		command_line.append(" -dxvk");

	/*if (command_line.find("-launchingavx") != string::npos)
	{
		if (logs)
		{
			cout << "Проверка на доступность инструкций AVX2 в процессоре...\n";
			cout << "Checking for availability of AVX2 instructions in the processor...\n";
		}
		if (IsAVX2())
		{
			if (logs)
			{
				cout << "Ваш процессор поддерживает AVX2, будет запущена версия движка под AVX2...\n";
				cout << "Your processor supports AVX2, the AVX2 version of the engine will run...\n";
			}

			pathToExe = "bin\\AVX2\\xrEngine.exe";
		}
		else if (logs)
		{
			cout << "Ваш процессор не поддерживает AVX2, будет запущена версия движка под SSE2...\n";
			cout << "Your processor doesn't have AVX2, the SSE2 version of the engine will run...\n";
		}
		if (logs)
		{
			cout << "Нажмите Enter для продолжения...";
			cout << "Press Enter to continue...";
			cin.get();
		}
	}*/

	// start the program up
	CreateProcessA
	(
		pathToExe.c_str(),							// the path
		(LPSTR)command_line.c_str(),				// Command line
		NULL,										// Process handle not inheritable
		NULL,										// Thread handle not inheritable
		FALSE,										// Set handle inheritance to FALSE
		DETACHED_PROCESS,
		NULL,										// Use parent's environment block
		NULL,										// Use parent's starting directory 
		&si,										// Pointer to STARTUPINFO structure
		&pi											// Pointer to PROCESS_INFORMATION structure
	);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}