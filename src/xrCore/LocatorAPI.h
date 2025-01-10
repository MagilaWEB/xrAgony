#pragma once

#pragma warning(push)
#pragma warning(disable : 4995)
#if defined(WINDOWS)
#include <io.h>
#endif
#pragma warning(pop)
#include "Common/Util.hpp"
#include "LocatorAPI_defs.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/predicates.h"
#include "Common/Noncopyable.hpp"

#if defined(LINUX)
#include <stdint.h>
#define _A_HIDDEN	  0x02
#define _A_SUBDIR 0x00000010

#ifdef XR_X64
#define _finddata_t _finddata64i32_t
#elif XR_X86
#define _finddata_t _finddata32_t
#endif // XR_X64 or XR_X86

typedef int64_t __int64;
typedef __int64 __time64_t;
typedef long __time32_t;
typedef unsigned long _fsize_t;

struct _finddata64i32_t {
	unsigned attrib;
	__time64_t time_create;
	__time64_t time_access;
	__time64_t time_write;
	_fsize_t size;
	char name[FILENAME_MAX];
};

struct _finddata32_t
{
	unsigned attrib;
	__time32_t time_create;
	__time32_t time_access;
	__time32_t time_write;
	_fsize_t size;
	char name[FILENAME_MAX];
};

#define _finddata_t	 _finddata64i32_t
#endif

class CStreamReader;

enum class FSType
{
	Virtual = 1,
	External = 2,
	Any = Virtual | External,
};

IMPLEMENT_ENUM_FLAG_OPERATORS(FSType, int);

class FileStatus
{
public:
	bool Exists;
	bool External; // File can be accessed only as external

	FileStatus(bool exists, bool external)
	{
		Exists = exists;
		External = external;
	}

	operator bool() const { return Exists; }
};

class XRCORE_API CLocatorAPI : Noncopyable
{
	friend class FS_Path;

public:

	struct fileWinApi
	{
	private:
		template<class s> static bool is_dots(const s& path) noexcept
		{
			const auto& a = path[0];
			const auto& b = path[1];
			const auto& c = path[2];
			if (a == '.' && (b == 0 || (b == '.' && c == 0)))
				return true;
			return false;
		}

		struct raiiA
		{
			raiiA(const xr_string& dir) : hFind()
			{
				xr_string mask = dir + "\\*";

				this->hFind = FindFirstFile(mask.c_str(), &this->ffd);

				if (this->hFind == INVALID_HANDLE_VALUE)
					throw std::runtime_error(
						"remove_directory(INVALID_HANDLE_VALUE): '"
						+ dir + "'"
					);
			}

			bool next() noexcept
			{
				return FindNextFile(this->hFind, &this->ffd);
			}

			~raiiA()
			{
				FindClose(this->hFind);
			}

			::HANDLE hFind;
			::WIN32_FIND_DATAA ffd;
		};

		//==============================================================================
		//==============================================================================
	public:
		static bool FileExists(const string_path path)
		{
			DWORD dwAttrib = GetFileAttributes(path);

			return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
				!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		static bool is_directory(const string_path path)
		{
			const DWORD attr = GetFileAttributes(path);
			return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
		}

		static void remove_file(const string_path path)
		{
			if (FileExists(path))
				DeleteFile(path);
		}

		static void copy_file(const string_path path_from, const string_path path_to)
		{
			if (FileExists(path_from))
				CopyFile(path_from, path_to, FALSE);
		}

		static void move_file(const string_path path_from, const string_path path_to)
		{
			if (FileExists(path_from))
				MoveFile(path_from, path_to);
		}

		static void remove_directory(const string_path dir)
		{
			raiiA data(dir);
			while (data.next())
			{
				if (is_dots(data.ffd.cFileName))
					continue;

				const xr_string path = xr_string{ dir } + "\\" + data.ffd.cFileName;

				if (data.ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					remove_directory(path.c_str());
				else
					remove_file(path.c_str());
			}


			// remove the empty directory
			RemoveDirectory(dir);
		}

		static void remove_all(const string_path path)
		{
			if (!is_directory(path))
				remove_file(path);
			else
				remove_directory(path);
		}
	};

public:
	struct file
	{
		pcstr name; // low-case name
		u32 vfs; // 0xffffffff - standart file
		u32 crc; // contents CRC
		u32 ptr; // pointer inside vfs
		u32 size_real; //
		u32 size_compressed; // if (size_real==size_compressed) - uncompressed
		u32 modif; // for editor
	};

	struct archive
	{
		u32 size;
		u32 vfs_idx;
		shared_str path;
		void* hSrcFile, * hSrcMap;
		CInifile* header;

		archive() : size(0), vfs_idx(u32(-1)), hSrcFile(nullptr), hSrcMap(nullptr), header(nullptr) {}
		void open();
		void close();
	};

	using archives_vec = xr_vector<archive>;
	archives_vec m_archives;
	void LoadArchive(archive& A, pcstr entrypoint = nullptr);

private:
	struct file_pred
	{
		bool operator()(const file& x, const file& y) const { return xr_strcmp(x.name, y.name) < 0; }
	};

	using PathMap = xr_map<pcstr, FS_Path*, pred_str>;
	PathMap pathes;

	using files_set = xr_set<file, file_pred>;
	using files_it = files_set::iterator;

	using FFVec = xr_vector<_finddata64i32_t>;
	FFVec rec_files;

	int m_iLockRescan;
	void check_pathes();

	files_set m_files;
	bool bNoRecurse;

	u64 m_auth_code;

	const file* RegisterExternal(pcstr name);
	const file* Register(pcstr name, u32 vfs, u32 crc, u32 ptr, u32 size_real, u32 size_compressed, u32 modif);
	void ProcessArchive(pcstr path);
	void ProcessOne(pcstr path, const _finddata_t& entry);
	bool Recurse(pcstr path);

	files_it file_find_it(pcstr n);

public:
	enum : u32
	{
		flNeedRescan = (1 << 0),
		flBuildCopy = (1 << 1),
		flReady = (1 << 2),
		flEBuildCopy = (1 << 3),
		flEventNotificator = (1 << 4),
		flTargetFolderOnly = (1 << 5),
		flCacheFiles = (1 << 6),
		flScanAppRoot = (1 << 7),
		flNeedCheck = (1 << 8),
		flDumpFileActivity = (1 << 9),
	};
	Flags32 m_Flags;
	u32 dwAllocGranularity;
	u32 dwOpenCounter;

private:
	void check_cached_files(pstr fname, const u32& fname_size, const file& desc, pcstr& source_name);

	void file_from_cache_impl(IReader*& R, LPSTR fname, const file& desc);
	void file_from_cache_impl(CStreamReader*& R, LPSTR fname, const file& desc);
	template <typename T>
	void file_from_cache(T*& R, pstr fname, const u32& fname_size, const file& desc, pcstr& source_name);

	void file_from_archive(IReader*& R, pcstr fname, const file& desc);
	void file_from_archive(CStreamReader*& R, pcstr fname, const file& desc);

	void copy_file_to_build(IWriter* W, IReader* r);
	void copy_file_to_build(IWriter* W, CStreamReader* r);
	template <typename T>
	void copy_file_to_build(T*& R, pcstr source_name);

	bool check_for_file(pcstr path, pcstr _fname, string_path& fname, const file*& desc);

	template <typename T>
	T* r_open_impl(pcstr path, pcstr _fname);

	void setup_fs_path(pcstr fs_name, string_path& fs_path);
	void setup_fs_path(pcstr fs_name);
	IReader* setup_fs_ltx(pcstr fs_name);

public:
	CLocatorAPI();
	~CLocatorAPI();
	void _initialize(u32 flags, pcstr target_folder = nullptr, pcstr fs_name = nullptr);
	void _destroy();

	CStreamReader* rs_open(pcstr initial, pcstr N);
	IReader* r_open(pcstr initial, pcstr N);
	IReader* r_open(pcstr N) { return r_open(nullptr, N); }
	void r_close(IReader*& S);
	void r_close(CStreamReader*& fs);

	IWriter* w_open(pcstr initial, pcstr N);
	IWriter* w_open(pcstr N) { return w_open(nullptr, N); }
	IWriter* w_open_ex(pcstr initial, pcstr N);
	IWriter* w_open_ex(pcstr N) { return w_open_ex(nullptr, N); }
	void w_close(IWriter*& S);
	// For registered files only
	const file* GetFileDesc(pcstr path);

	FileStatus exist(pcstr N, FSType fsType = FSType::Virtual);
	FileStatus exist(pcstr path, pcstr name, FSType fsType = FSType::Virtual);
	FileStatus exist(string_path& fn, pcstr path, pcstr name, FSType fsType = FSType::Virtual);
	FileStatus exist(string_path& fn, pcstr path, pcstr name, pcstr ext, FSType fsType = FSType::Virtual);

	bool can_write_to_folder(pcstr path);
	bool can_write_to_alias(pcstr path);
	bool can_modify_file(pcstr fname);
	bool can_modify_file(pcstr path, pcstr name);

	bool dir_delete(pcstr path, pcstr nm, bool remove_files);
	bool dir_delete(pcstr full_path, bool remove_files) { return dir_delete(nullptr, full_path, remove_files); }

	bool win_dir_delete(pcstr initial, pcstr nm);
	bool win_dir_delete(pcstr full_path) { return win_dir_delete(nullptr, full_path); };

	void file_delete(pcstr path, pcstr nm);
	void file_delete(pcstr full_path) { file_delete(nullptr, full_path); }

	void win_file_delete(pcstr path, pcstr nm);
	void win_file_delete(pcstr full_path) { win_file_delete(nullptr, full_path); }

	void file_copy(pcstr src, pcstr dest);
	void file_rename(pcstr src, pcstr dest, bool overwrite = true);
	int file_length(pcstr src);

	u32 get_file_age(pcstr nm);
	void set_file_age(pcstr nm, u32 age);

	xr_vector<pstr>* file_list_open(pcstr initial, pcstr folder, u32 flags = FS_ListFiles);
	xr_vector<pstr>* file_list_open(pcstr path, u32 flags = FS_ListFiles);
	void file_list_close(xr_vector<pstr>*& lst);

	bool path_exist(pcstr path);
	FS_Path* get_path(pcstr path);
	FS_Path* append_path(pcstr path_alias, pcstr root, pcstr add, bool recursive);
	pcstr update_path(string_path& dest, pcstr initial, pcstr src);

	int file_list(FS_FileSet& dest, pcstr path, u32 flags = FS_ListFiles, pcstr mask = nullptr);

	bool load_all_unloaded_archives();
	void unload_archive(archive& A);

	void rescan_path(pcstr full_path, bool bRecurse);
	// editor functions
	void rescan_pathes();
	void lock_rescan();
	void unlock_rescan();
};

extern XRCORE_API std::unique_ptr<CLocatorAPI> xr_FS;
#define FS (*xr_FS)
