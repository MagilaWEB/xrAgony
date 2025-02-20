// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef LUABIND_INHERITANCE_090217_HPP
# define LUABIND_INHERITANCE_090217_HPP

# include <luabind/config.hpp>
# include <cassert>
# include <limits>
# include <map>
# include <memory>
# include <vector>
# include <luabind/typeid.hpp>

namespace luabind {

	namespace detail {

		using cast_function = void*(*)(void*);
		using class_id	  = std::size_t;

		constexpr class_id unknown_class = std::numeric_limits<class_id>::max();

		class class_rep;

		class cast_graph
		{
		public:
			LUABIND_API cast_graph();
			LUABIND_API ~cast_graph();

			// `src` and `p` here describe the *most derived* object. This means that
			// for a polymorphic type, the pointer must be cast with
			// dynamic_cast<void*> before being passed in here, and `src` has to
			// match typeid(*p).
			LUABIND_API std::pair<void*, int> cast(void* p, class_id src, class_id target, class_id dynamic_id, void const* dynamic_ptr) const;
			LUABIND_API void insert(class_id src, class_id target, cast_function cast);

		private:
			class impl;
			luabind::unique_ptr<impl> m_impl;
		};

		// Maps a type_id to a class_id. Note that this actually partitions the
		// id-space into two, using one half for "local" ids; ids that are used only as
		// keys into the conversion cache. This is needed because we need a unique key
		// even for types that hasn't been registered explicitly.
		class class_id_map
		{
		public:
			LUABIND_API class_id_map();

			LUABIND_API class_id get(type_id const& type) const;
			LUABIND_API class_id get_local(type_id const& type);
			LUABIND_API void put(class_id id, type_id const& type);

		private:
			using map_type = luabind::map<type_id, class_id>;
			map_type m_classes;
			class_id m_local_id;

			static class_id const local_id_base;
		};

		inline class_id_map::class_id_map()
			: m_local_id(local_id_base)
		{}

		inline class_id class_id_map::get(type_id const& type) const
		{
			map_type::const_iterator i = m_classes.find(type);
			if(i == m_classes.end() || i->second >= local_id_base) {
				return unknown_class;
			}
			else {
				return i->second;
			}
		}

		inline class_id class_id_map::get_local(type_id const& type)
		{
			const std::pair<map_type::iterator, bool> result = m_classes.emplace(type, 0);

			if(result.second) result.first->second = m_local_id++;
			assert(m_local_id >= local_id_base);
			return result.first->second;
		}

		inline void class_id_map::put(class_id id, type_id const& type)
		{
			assert(id < local_id_base);

			const std::pair<map_type::iterator, bool> result = m_classes.emplace(type, 0);

			assert(
				result.second
				|| result.first->second == id
				|| result.first->second >= local_id_base
			);

			result.first->second = id;
		}

		class class_map
		{
		public:
			class_rep* get(class_id id) const;
			void put(class_id id, class_rep* cls);

		private:
			luabind::vector<class_rep*> m_classes;
		};

		inline class_rep* class_map::get(class_id id) const
		{
			if(id >= m_classes.size())
				return 0;
			return m_classes[id];
		}

		inline void class_map::put(class_id id, class_rep* cls)
		{
			if(id >= m_classes.size())
				m_classes.resize(id + 1);
			m_classes[id] = cls;
		}

		template <class S, class T>
		struct static_cast_
		{
			static void* execute(void* p)
			{
				return static_cast<T*>(static_cast<S*>(p));
			}
		};

		template <class S, class T>
		struct dynamic_cast_
		{
			static void* execute(void* p)
			{
				return dynamic_cast<T*>(static_cast<S*>(p));
			}
		};

		// Thread safe class_id allocation.
		LUABIND_API class_id allocate_class_id(type_id const& cls);

		template <class T>
		struct registered_class
		{
			static class_id const id;
		};

		template <class T>
		class_id const registered_class<T>::id = allocate_class_id(typeid(T));

		template <class T>
		struct registered_class<T const>
			: registered_class<T>
		{};

	}	// namespace detail

} // namespace luabind

#endif // LUABIND_INHERITANCE_090217_HPP

