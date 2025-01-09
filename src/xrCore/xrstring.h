#pragma once

#include <cstdio>

#include "_types.h"
#include "xrCommon/inlining_macros.h"
#include "xrMemory.h"

#include <cstring>

#pragma pack(push, 4)
#pragma warning(push)
#pragma warning(disable : 4200)
struct XRCORE_API str_value
{
	u32 dwReference;
	u32 dwLength;
	u32 dwCRC;
	str_value* next;
	char value[];
};

struct XRCORE_API str_value_cmp
{
	// less
	IC bool operator()(const str_value* A, const str_value* B) const { return A->dwCRC < B->dwCRC; };
};

struct XRCORE_API str_hash_function
{
	IC u32 operator()(str_value const* const value) const { return value->dwCRC; };
};

#pragma warning(pop)

struct str_container_impl;
class IWriter;
//////////////////////////////////////////////////////////////////////////
class XRCORE_API str_container
{
	str_container_impl* impl;

public:
	str_container();
	~str_container();

	str_value* dock(pcstr value);
	void clean();
	void dump();
	void dump(IWriter* W);
	void verify();
	u32 stat_economy();
};
XRCORE_API extern str_container* g_pStringContainer;

//////////////////////////////////////////////////////////////////////////
class shared_str
{
	str_value* p_{ nullptr };

protected:
	// ref-counting
	void _dec() noexcept
	{
		if (p_)
		{
			p_->dwReference--;
			if (0 == p_->dwReference)
				p_ = nullptr;
		}
	}

public:
	void _set(pcstr rhs)
	{
		str_value* v = g_pStringContainer->dock(rhs);
		if (v)
			v->dwReference++;
		_dec();
		p_ = v;
	}

	void _set(shared_str const& rhs) noexcept
	{
		str_value* v = rhs.p_;
		if (v)
			v->dwReference++;
		_dec();
		p_ = v;
	}

	void _set(nullptr_t) noexcept
	{
		_dec();
		p_ = nullptr;
	}

	[[nodiscard]]
	const str_value* _get() const { return p_; }

public:
	// construction
	shared_str() = default;
	shared_str(pcstr rhs)
	{
		p_ = nullptr;
		_set(rhs);
	}
	shared_str(shared_str const& rhs) noexcept
	{
		p_ = nullptr;
		_set(rhs);
	}

	shared_str(shared_str&& rhs) noexcept
		: p_(rhs.p_)
	{
		rhs.p_ = nullptr;
	}
	~shared_str() { _dec(); }
	// assignment & accessors
	shared_str& operator=(pcstr rhs)
	{
		_set(rhs);
		return *this;
	}
	shared_str& operator=(shared_str const& rhs) noexcept
	{
		_set(rhs);
		return *this;
	}
	// XXX tamlin: Remove operator*(). It may be convenient, but it's dangerous. Use
	[[nodiscard]]
	pcstr operator*() const { return p_ ? p_->value : 0; }

	[[nodiscard]]
	bool operator!() const { return p_ == nullptr; }

	[[nodiscard]]
	char operator[](size_t id) { return p_->value[id]; }

	[[nodiscard]]
	char operator[](size_t id) const { return p_->value[id]; }

	[[nodiscard]]
	pcstr c_str() const { return p_ ? p_->value : 0; }

	// misc func
	[[nodiscard]]
	size_t size() const
	{
		if (nullptr == p_)
			return 0;

		return p_->dwLength;
	}

	[[nodiscard]]
	bool empty() const
	{
		return size() == 0;
	}

	void swap(shared_str& rhs) noexcept
	{
		str_value* tmp = p_;
		p_ = rhs.p_;
		rhs.p_ = tmp;
	}

	[[nodiscard]]
	bool equal(const shared_str& rhs) const { return (p_ == rhs.p_); }

	shared_str& __cdecl printf(const char* format, ...)
	{
		string4096 buf;
		va_list p;
		va_start(p, format);
		int vs_sz = vsnprintf(buf, sizeof(buf) - 1, format, p);
		buf[sizeof(buf) - 1] = 0;
		va_end(p);
		if (vs_sz)
			_set(buf);
		return (shared_str&)*this;
	}
};

// res_ptr == res_ptr
// res_ptr != res_ptr
// const res_ptr == ptr
// const res_ptr != ptr
// ptr == const res_ptr
// ptr != const res_ptr
// res_ptr < res_ptr
// res_ptr > res_ptr
IC bool operator==(shared_str const& a, shared_str const& b) { return a._get() == b._get(); }
IC bool operator!=(shared_str const& a, shared_str const& b) { return a._get() != b._get(); }
IC bool operator<(shared_str const& a, shared_str const& b) { return a._get() < b._get(); }
IC bool operator>(shared_str const& a, shared_str const& b) { return a._get() > b._get(); }
// externally visible standard functionality
IC void swap(shared_str& lhs, shared_str& rhs) { lhs.swap(rhs); }
IC u32 xr_strlen(const shared_str& a) noexcept { return a.size(); }

ICF int xr_strcmp(const char* S1, const char* S2)
{
	return strcmp(S1, S2);
}
IC int xr_strcmp(const shared_str& a, const char* b) noexcept { return xr_strcmp(*a, b); }
IC int xr_strcmp(const char* a, const shared_str& b) noexcept { return xr_strcmp(a, *b); }
IC int xr_strcmp(const shared_str& a, const shared_str& b) noexcept
{
	if (a.equal(b))
		return 0;
	else
		return xr_strcmp(*a, *b);
}

IC void xr_strlwr(shared_str& src)
{
	if (*src)
	{
		char* lp = xr_strdup(*src);
		xr_strlwr(lp);
		src = lp;
		xr_free(lp);
	}
}

IC char* xr_strlwr(char* src)
{
	int i = 0;
	while (src[i])
	{
		src[i] = (char)toupper(src[i]);// TODO rewrite locale-independent toupper_l()
		i++;
	}
	return src;
}

#pragma pack(pop)
