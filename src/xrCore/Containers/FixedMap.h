#pragma once
#include "xrCore/xrDebug_macros.h"
#include "Common/xr_allocator.h"

// Both xr_fixed_map doesn't support move operation
// Trying to do so will result in a crash at some point
// (at first it would be just fine, but this is a lie)

template <typename K, typename T, typename allocator = xr_allocator>
class xr_fixed_map
{
	static constexpr size_t SG_REALLOC_ADVANCE = 64;

public:
	struct value_type
	{
		K first{};
		T second{};
		value_type* left{}, * right{};
		~value_type()
		{
			left = nullptr;
			right = nullptr;
		}
	};
	typedef void __fastcall callback(value_type*);

private:
	value_type* nodes{};
	size_t pool{};
	size_t limit{};

	IC size_t Size(size_t Count) { return Count * sizeof(value_type); }

	void Realloc()
	{
		size_t newLimit = limit + SG_REALLOC_ADVANCE;
		VERIFY(newLimit % SG_REALLOC_ADVANCE == 0);
		value_type* newNodes = (value_type*)allocator::alloc(Size(newLimit));
		VERIFY(newNodes);

		if constexpr (std::is_pod_v<T>)
		{
			ZeroMemory(newNodes, Size(newLimit));
			if (pool)
				CopyMemory(newNodes, nodes, Size(limit));
		}
		else
		{
			for (value_type* cur = newNodes; cur != newNodes + newLimit; ++cur)
				allocator::construct(cur);

			if (pool)
				std::copy(begin(), last(), newNodes);
		}

		for (size_t I = 0; I < pool; I++)
		{
			VERIFY(nodes);
			value_type* Nold = nodes + I;
			value_type* Nnew = newNodes + I;

			if (Nold->left)
			{
				size_t Lid = Nold->left - nodes;
				Nnew->left = newNodes + Lid;
			}
			if (Nold->right)
			{
				size_t Rid = Nold->right - nodes;
				Nnew->right = newNodes + Rid;
			}
		}

		if (nodes)
			allocator::dealloc(nodes);

		nodes = newNodes;
		limit = newLimit;
	}

	IC value_type* Alloc(const K& first)
	{
		if (pool == limit)
			Realloc();
		value_type* node = nodes + pool;
		node->first = first;
		node->right = node->left = nullptr;
		pool++;
		return node;
	}
	IC value_type* CreateChild(value_type*& parent, const K& first)
	{
		size_t PID = size_t(parent - nodes);
		value_type* N = Alloc(first);
		parent = nodes + PID;
		return N;
	}

	IC void recurseLR(value_type* N, callback CB)
	{
		if (N->left)
			recurseLR(N->left, CB);
		CB(N);
		if (N->right)
			recurseLR(N->right, CB);
	}
	IC void recurseRL(value_type* N, callback CB)
	{
		if (N->right)
			recurseRL(N->right, CB);
		CB(N);
		if (N->left)
			recurseRL(N->left, CB);
	}
	IC void getLR(value_type* N, xr_vector<T>& D)
	{
		if (N->left)
			getLR(N->left, D);
		D.push_back(N->second);
		if (N->right)
			getLR(N->right, D);
	}
	IC void getRL(value_type* N, xr_vector<T>& D)
	{
		if (N->right)
			getRL(N->right, D);
		D.push_back(N->second);
		if (N->left)
			getRL(N->left, D);
	}

public:
	~xr_fixed_map() { destroy(); }
	void destroy()
	{
		if (nodes)
		{
			for (value_type* cur = begin(); cur != last(); cur++)
				allocator::destroy(cur);
			allocator::dealloc(nodes);
		}
	}
	IC value_type* insert(const K& k)
	{
		if (pool)
		{
			value_type* node = nodes;

		once_more:
			if (k < node->first)
			{
				if (node->left)
				{
					node = node->left;
					goto once_more;
				}
				else
				{
					value_type* N = CreateChild(node, k);
					node->left = N;
					return N;
				}
			}
			else if (k > node->first)
			{
				if (node->right)
				{
					node = node->right;
					goto once_more;
				}
				else
				{
					value_type* N = CreateChild(node, k);
					node->right = N;
					return N;
				}
			}
			else
				return node;
		}
		else
		{
			return Alloc(k);
		}
	}

	IC value_type* insertInAnyWay(const K& k)
	{
		if (pool)
		{
			value_type* node = nodes;

		once_more:
			if (k <= node->first)
			{
				if (node->left)
				{
					node = node->left;
					goto once_more;
				}
				else
				{
					value_type* N = CreateChild(node, k);
					node->left = N;
					return N;
				}
			}
			else
			{
				if (node->right)
				{
					node = node->right;
					goto once_more;
				}
				else
				{
					value_type* N = CreateChild(node, k);
					node->right = N;
					return N;
				}
			}
		}
		else
		{
			return Alloc(k);
		}
	}

	IC size_t allocated() { return this->limit; }
	bool empty() const { return pool == 0; }
	IC void clear() { pool = 0; }
	IC value_type* begin() { return nodes; }
	IC value_type* end() { return nodes + pool; }
	IC value_type* last() { return nodes + limit; } // for setup only
	IC size_t size() { return pool; }
	IC value_type& operator[](int v) { return nodes[v]; }

	IC void traverseLR(callback CB)
	{
		if (pool)
			recurseLR(nodes, CB);
	}
	IC void traverseRL(callback CB)
	{
		if (pool)
			recurseRL(nodes, CB);
	}

	IC void getLR(xr_vector<T>& D)
	{
		if (pool)
			getLR(nodes, D);
	}

	IC void getRL(xr_vector<T>& D)
	{
		if (pool)
			getRL(nodes, D);
	}

	IC void getANY_P(xr_vector<value_type*>& D)
	{
		if (empty())
			return;

		D.resize(size());
		value_type** _it = &D.front();
		value_type* _end = end();
		for (value_type* cur = begin(); cur != _end; cur++, _it++)
			*_it = cur;
	}

	IC void setup(callback CB)
	{
		for (size_t i = 0; i < limit; i++)
			CB(nodes + i);
	}
};