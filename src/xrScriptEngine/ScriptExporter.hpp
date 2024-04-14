#pragma once
#include "xrScriptEngine/xrScriptEngine.hpp"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_list.h"

struct lua_State;

namespace XRay
{
	class XRSCRIPTENGINE_API ScriptExporter
	{
	public:
		class XRSCRIPTENGINE_API Node
		{
		public:
			using ExporterFunc = void(__cdecl*)(lua_State* luaState);

		private:
			pcstr m_id;
			xr_list<shared_str> m_deps;
			ExporterFunc m_exporterFunc;
			bool m_done;
			Node* m_prevNode;
			Node* m_nextNode;
			static Node* FirstNode;
			static Node* LastNode;
			static size_t NodeCount;

		public:
			Node(const char* id, std::initializer_list<shared_str> deps, ExporterFunc exporterFunc);
			~Node();

			void Export(lua_State* luaState);
			void Reset() { m_done = false; }
			const char* GetId() const { return m_id; }
			size_t GetDependencyCount() const { return m_deps.size(); }
			const xr_list<shared_str>& GetDependencyIds() const { return m_deps; }
			Node* GetPrev() const { return m_prevNode; }
			Node* GetNext() const { return m_nextNode; }
			static Node* GetFirst() { return FirstNode; }
			static Node* GetLast() { return LastNode; }
			static size_t GetCount() { return NodeCount; }
		private:
			bool HasDependency(const Node* node) const;
			static void InsertAfter(Node* target, Node* node);
		};

		ScriptExporter() = delete;
		static void Export(lua_State* luaState);
		static void Reset();
	};
}

#define SCRIPT_INHERIT_1(...) #__VA_ARGS__
#define SCRIPT_INHERIT_2(_1, _2) #_1, #_2
#define SCRIPT_INHERIT_3(_1, _2, _3) #_1, #_2, #_3
#define SCRIPT_INHERIT_4(_1, _2, _3, _4) #_1, #_2, #_3, #_4

#define EXPAND_MACRO(x) x
#define OVERLOAD_MACRO(_1, _2, _3, _4, count, ...) SCRIPT_INHERIT_##count
#define SCRIPT_INHERIT(...) EXPAND_MACRO(OVERLOAD_MACRO(__VA_ARGS__, 4, 3, 2, 1, 0)(__VA_ARGS__))

// clang-format off
#define SCRIPT_EXPORT(id, dependencies, ...)					\
using namespace luabind;										\
namespace														\
{																\
	__pragma(optimize("s", on))									\
	static void id##_ScriptExport(lua_State* luaState)			\
	__VA_ARGS__													\
	static const XRay::ScriptExporter::Node id##_Node(			\
		#id, {SCRIPT_INHERIT dependencies}, id##_ScriptExport);	\
}

/////////////////////////////////////////////////////////////////////////////

#define SCRIPT_EXPORT_FUNC(id, dependencies, func)				\
namespace 														\
{																\
	__pragma(optimize("s", on))									\
	static void id##_ScriptExport(lua_State* luaState)			\
	{ func(luaState); }											\
	static const XRay::ScriptExporter::Node id##_FuncNode(		\
		#id, {SCRIPT_INHERIT dependencies}, id##_ScriptExport);	\
}