#pragma once

#include "ResourceManager.h"

template <typename T>
struct ShaderTypeTraits;

template <>
struct ShaderTypeTraits<SVS>
{
	typedef CResourceManager::map_VS MapType;

	using HWShaderType = ID3DVertexShader*;

	static inline const char* GetShaderExt() { return ".vs"; }
	static inline const char* GetCompilationTarget()
	{
		return "vs_2_0";
	}

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
	{
		target = "vs_2_0";

#if defined(R2_SHADERS_BACKWARDS_COMPATIBILITY)
		if (strstr(data, "main_vs_1_1"))
		{
			target = "vs_1_1";
			entry = "main_vs_1_1";
		}
#endif

		if (strstr(data, "main_vs_2_0"))
		{
			target = "vs_2_0";
			entry = "main_vs_2_0";
		}
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreateVertexShader(buffer, size, 0, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_vertex; }
};

template <>
struct ShaderTypeTraits<SPS>
{
	typedef CResourceManager::map_PS MapType;
	using HWShaderType = ID3DPixelShader*;

	static inline const char* GetShaderExt() { return ".ps"; }
	static inline const char* GetCompilationTarget()
	{
		return "ps_4_0";
	}

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* data)
	{
#if defined(R2_SHADERS_BACKWARDS_COMPATIBILITY)
		if (strstr(data, "main_ps_1_1"))
		{
			target = "ps_1_1";
			entry = "main_ps_1_1";
		}
		if (strstr(data, "main_ps_1_2"))
		{
			target = "ps_1_2";
			entry = "main_ps_1_2";
		}
		if (strstr(data, "main_ps_1_3"))
		{
			target = "ps_1_3";
			entry = "main_ps_1_3";
		}
		if (strstr(data, "main_ps_1_4"))
		{
			target = "ps_1_4";
			entry = "main_ps_1_4";
		}
#endif
		if (strstr(data, "main_ps_2_0"))
		{
			target = "ps_2_0";
			entry = "main_ps_2_0";
		}
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreatePixelShader(buffer, size, 0, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_pixel; }
	};

template <>
struct ShaderTypeTraits<SGS>
{
	typedef CResourceManager::map_GS MapType;

	using HWShaderType = ID3DGeometryShader*;


	static inline const char* GetShaderExt() { return ".gs"; }
	static inline const char* GetCompilationTarget() { return "gs_4_0"; }

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
	{
		target = GetCompilationTarget();
		entry = "main";
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreateGeometryShader(buffer, size, 0, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_geometry; }
};

template <>
struct ShaderTypeTraits<SHS>
{
	typedef CResourceManager::map_HS MapType;

	using HWShaderType = ID3D11HullShader*;

	static inline const char* GetShaderExt() { return ".hs"; }
	static inline const char* GetCompilationTarget() { return "hs_5_0"; }

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
	{
		target = GetCompilationTarget();
		entry = "main";
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreateHullShader(buffer, size, nullptr, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_hull; }
};

template <>
struct ShaderTypeTraits<SDS>
{
	typedef CResourceManager::map_DS MapType;

	using HWShaderType = ID3D11DomainShader*;

	static inline const char* GetShaderExt() { return ".ds"; }
	static inline const char* GetCompilationTarget() { return "ds_5_0"; }

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
	{
		target = GetCompilationTarget();
		entry = "main";
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreateDomainShader(buffer, size, nullptr, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_domain; }
};

template <>
struct ShaderTypeTraits<SCS>
{
	using MapType =  CResourceManager::map_CS;

	using HWShaderType = ID3D11ComputeShader*;

	static inline const char* GetShaderExt() { return ".cs"; }
	static inline const char* GetCompilationTarget() { return "cs_5_0"; }

	static void GetCompilationTarget(const char*& target, const char*& entry, const char* /*data*/)
	{
		target = GetCompilationTarget();
		entry = "main";
	}

	static inline HRESULT CreateHWShader(DWORD const* buffer, size_t size, HWShaderType& sh)
	{
		HRESULT _res = 0;
		_res = HW.pDevice->CreateComputeShader(buffer, size, nullptr, &sh);
		return _res;
	}

	static inline u32 GetShaderDest() { return RC_dest_compute; }
};

template <>
inline CResourceManager::map_PS& CResourceManager::GetShaderMap()
{
	return m_ps;
}

template <>
inline CResourceManager::map_VS& CResourceManager::GetShaderMap()
{
	return m_vs;
}

template <>
inline CResourceManager::map_GS& CResourceManager::GetShaderMap()
{
	return m_gs;
}
template <>
inline CResourceManager::map_DS& CResourceManager::GetShaderMap()
{
	return m_ds;
}

template <>
inline CResourceManager::map_HS& CResourceManager::GetShaderMap()
{
	return m_hs;
}

template <>
inline CResourceManager::map_CS& CResourceManager::GetShaderMap()
{
	return m_cs;
}

template <typename T>
inline T* CResourceManager::CreateShader(const char* name, const char* filename /*= nullptr*/, const bool searchForEntryAndTarget /*= false*/)
{
	typename ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<typename ShaderTypeTraits<T>::MapType>();
	LPSTR N = LPSTR(name);
	auto iterator = sh_map.find(N);

	if (iterator != sh_map.end())
		return iterator->second;
	else
	{
		T* sh = new T();

		sh->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		sh_map.insert(std::make_pair(sh->set_name(name), sh));
		if (0 == xr_stricmp(name, "null"))
		{
			sh->sh = nullptr;
			return sh;
		}

		// Remove ( and everything after it
		string_path shName;
		{
			if (filename == nullptr)
				filename = name;

			pcstr pchr = strchr(filename, '(');
			ptrdiff_t size = pchr ? pchr - filename : xr_strlen(filename);
			strncpy(shName, filename, size);
			shName[size] = 0;
		}

		// Open file
		string_path cname;
		pcstr shaderExt = ShaderTypeTraits<T>::GetShaderExt();
		strconcat(sizeof(cname), cname, shName, shaderExt);
		FS.update_path(cname, "$game_shaders$", cname);

		// Try to open
		IReader* file = FS.r_open(cname);
		if (!file /*&& strstr(Core.Params, "-lack_of_shaders")*/)
		{
			string1024 tmp;
			xr_sprintf(tmp, "CreateShader: %s is missing. Replacing it with stub_default%s", cname, shaderExt);
			Msg(tmp);
			strconcat(sizeof(cname), cname, "stub_default", shaderExt);
			FS.update_path(cname, "$game_shaders$", cname);
			file = FS.r_open(cname);
		}

		R_ASSERT3(file, "Shader file doesnt exist:", cname);

		// Duplicate and zero-terminate
		const auto size = file->length();
		char* const data = (LPSTR)_alloca(size + 1);
		CopyMemory(data, file->pointer(), size);
		data[size] = 0;

		// Select target
		LPCSTR c_target = ShaderTypeTraits<T>::GetCompilationTarget();
		LPCSTR c_entry = "main";

		if (searchForEntryAndTarget)
			ShaderTypeTraits<T>::GetCompilationTarget(c_target, c_entry, data);

		DWORD flags = D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;

		if (Core.ParamFlags.test(Core.verboselog))
			Msg("compiling shader %s", name);

		// Compile
		HRESULT const _hr = ::Render->shader_compile(name, file, c_entry, c_target, flags, (void*&)sh);

		FS.r_close(file);

		VERIFY(SUCCEEDED(_hr));

		CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");

		SET_DEBUG_NAME(sh->sh, name);

		return sh;
	}
}

template <typename T>
inline void CResourceManager::DestroyShader(const T* sh)
{
	if (0 == (sh->dwFlags & xr_resource_flagged::RF_REGISTERED))
		return;

	typename ShaderTypeTraits<T>::MapType& sh_map = GetShaderMap<typename ShaderTypeTraits<T>::MapType>();

	LPSTR N = LPSTR(*sh->cName);
	auto iterator = sh_map.find(N);

	if (iterator != sh_map.end())
	{
		sh_map.erase(iterator);
		return;
	}
	Msg("! ERROR: Failed to find compiled shader '%s'", sh->cName.c_str());
}
