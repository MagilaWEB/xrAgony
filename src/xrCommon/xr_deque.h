#pragma once
#include <deque>

template <typename T, typename allocator = std::allocator<T>>
class xr_deque : public std::deque<T, allocator>
{
private:
    typedef std::deque<T, allocator> inherited;

public:
    xr_deque() : inherited() {}
    xr_deque(size_t _count, const T& _value) : inherited(_count, _value) {}
    explicit xr_deque(size_t _count) : inherited(_count) {}
    size_t size() const { return (size_t)inherited::size(); }

    template <class _Ty>
    IC const_iterator find(const _Ty& obj) const
    {
        return std::find(inherited::begin(), inherited::end(), obj);
    }

    template <class _Ty>
    IC iterator find(const _Ty& obj)
    {
        return std::find(inherited::begin(), inherited::end(), obj);
    }

    template <class _Ty>
    IC bool contain(const _Ty& obj) const
    {
        return find(obj) != inherited::end();
    }

    template <class _Ty>
    IC bool contain(_Ty& obj)
    {
        return find(obj) != inherited::end();
    }

    template <class _Ty>
    IC const_iterator find_if(const _Ty& obj) const
    {
        return std::find_if(inherited::begin(), inherited::end(), obj);
    }

    template <class _Ty>
    IC iterator find_if(const _Ty& obj)
    {
        return std::find_if(inherited::begin(), inherited::end(), obj);
    }

    template <class _Ty>
    IC bool contain_if(const _Ty& obj) const
    {
        return find_if(obj) != inherited::end();
    }

    template <class _Ty>
    IC bool contain_if(_Ty& obj)
    {
        return find_if(obj) != inherited::end();
    }

    template <typename _Pr>
    IC void sort(_Pr _Pred)
    {
        tbb::parallel_sort(inherited::begin(), inherited::end(), _Pred);
    }

    void clear_and_free() { inherited::clear(); }
    void clear_not_free() { inherited::erase(inherited::begin(), inherited::end()); }
    void clear_and_reserve() { if (inherited::capacity() <= (size() + size() / 4)) clear_not_free(); else { u32 old = size(); clear_and_free(); inherited::reserve(old); } }

#ifdef M_DONTDEFERCLEAR_EXT
    void clear() { clear_and_free(); }
#else
    void clear() { clear_not_free(); }
#endif
};

#define DEF_DEQUE(N, T)\
    using N = xr_deque<T>;\
    using N##_it = N::iterator;

#define DEFINE_DEQUE(T, N, I)\
    using N = xr_deque<T>;\
    using I = N::iterator;
