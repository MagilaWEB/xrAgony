#include "stdafx.h"
#include "ModuleLookup.hpp"

namespace XRay
{
	ModuleHandle::ModuleHandle(const bool dontUnload) : handle(nullptr), dontUnload(dontUnload) {}

	ModuleHandle::ModuleHandle(pcstr moduleName, bool dontUnload /*= false*/) : handle(nullptr), dontUnload(dontUnload)
	{
		this->Open(moduleName);
	}

	ModuleHandle::~ModuleHandle()
	{
		Close();
	}

	void* ModuleHandle::Open(pcstr moduleName)
	{
		if (IsLoaded())
			Close();

		Log("Loading DLL:", moduleName);

		handle = LoadLibraryA(moduleName);

		if (handle == nullptr)
			Msg("! Failed to load DLL: %s", xrDebug::ErrorToString(GetLastError()));

		return handle;
	}

	void ModuleHandle::Close()
	{
		if (dontUnload || (!IsLoaded()) || (!handle))
			return;

		bool closed = false;

		closed = FreeLibrary(static_cast<HMODULE>(handle)) != NULL;

		if (closed == false)
			Msg("! Failed to close DLL: %s", xrDebug::ErrorToString(GetLastError()));

		handle = nullptr;
	}

	bool ModuleHandle::IsLoaded() const
	{
		return handle != nullptr;
	}

	void* ModuleHandle::operator()() const
	{
		return handle;
	}

	void* ModuleHandle::GetProcAddress(pcstr procName) const
	{
		void* proc = nullptr;

		proc = ::GetProcAddress(static_cast<HMODULE>(handle), procName);

		if (proc == nullptr)
			Msg("! Failed to load procedure [%s]: %s", xrDebug::ErrorToString(GetLastError()));

		return proc;
	}
} // namespace XRay
