#pragma once
#include "xrCore/_types.h"
#include "xrCore/xrDebug_macros.h"
#include "xrCommon/inlining_macros.h"

// deprecated, use xr_array instead
template <class T, std::size_t dim>
class svector
{
public:
	using size_type = std::size_t;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reference = value_type&;
	using const_reference = const value_type&;

private:
	value_type _array[dim];
	size_type count{ 0 };

public:
	svector() = default;
	svector(iterator p, int c) { assign(p, c); }
	IC iterator begin() { return _array; }
	IC iterator end() { return _array + count; }
	IC const_iterator begin() const { return _array; }
	IC const_iterator end() const { return _array + count; }
	IC const_iterator cbegin() const { return _array; }
	IC const_iterator cend() const { return _array + count; }
	IC size_type size() const { return count; }
	IC void clear() { count = 0; }

	IC void resize(size_type c)
	{
		VERIFY(c <= dim);
		count = c;
	}

	IC void push_back(value_type e)
	{
		VERIFY(count < dim);
		_array[count++] = e;
	}

	IC void pop_back()
	{
		VERIFY(count);
		count--;
	}

	IC reference operator[](size_type id)
	{
		VERIFY(id < count);
		return _array[id];
	}

	IC const_reference operator[](size_type id) const
	{
		VERIFY(id < count);
		return _array[id];
	}

	IC reference front() { return _array[0]; }
	IC reference back() { return _array[count - 1]; }

	IC reference last()
	{
		VERIFY(count < dim);
		return _array[count];
	}

	IC const_reference front() const { return _array[0]; }
	IC const_reference back() const { return _array[count - 1]; }
	IC const_reference last() const
	{
		VERIFY(count < dim);
		return _array[count];
	}

	IC void inc() { count++; }
	IC bool empty() const { return 0 == count; }
	IC void erase(size_type id)
	{
		VERIFY(id < count);
		count--;
		for (size_type i = id; i < count; i++)
			_array[i] = _array[i + 1];
	}

	IC void erase(iterator it)
	{
		erase(size_type(it - begin()));
	}

	IC void insert(size_type id, reference V)
	{
		VERIFY(id < count);
		for (int i = count; i > int(id); i--)
			_array[i] = _array[i - 1];
		count++;
		_array[id] = V;
	}

	IC void assign(iterator p, int c)
	{
		VERIFY(c > 0 && c < dim);
		CopyMemory(_array, p, c * sizeof(value_type));
		count = c;
	}

	IC bool equal(const svector<value_type, dim>& base) const
	{
		if (size() != base.size())
			return false;
		for (size_type cmp = 0; cmp < size(); cmp++)
			if ((*this)[cmp] != base[cmp])
				return false;
		return true;
	}
};
