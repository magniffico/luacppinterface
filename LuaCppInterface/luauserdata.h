#ifndef LUAUSERDATA_H
#define LUAUSERDATA_H

#include <cassert>
#include <functional>
#include <cstddef>
#include <type_traits>
#include "luatype.h"
#include "luareference.h"

class Lua;
class LuaTable;

template<int...> struct int_sequence {};

template<int N, int... Is> struct make_int_sequence
	: make_int_sequence<N - 1, N - 1, Is...> {};
template<int... Is> struct make_int_sequence<0, Is...>
	: int_sequence<Is...>{};

template<int>
struct placeholder_template {};


namespace std
{
	template<int N>
	struct is_placeholder< placeholder_template<N> >
		: integral_constant<int, N + 1> // the one is important
	{};
}

template<typename TYPE>
class LuaUserdata : public LuaReference
{
	TYPE* pointer;

public:
	LuaUserdata(std::shared_ptr<lua_State> state, int index) :
	  LuaReference(state, index)
	{
		assert(GetType() == LuaType::userdata);
		assert(typeid(TYPE*) == typeid(RetrieveData()));

		auto wrap = (UserdataWrapper*)lua_touserdata(state.get(), index);
		pointer = wrap->actualData;
	}

	template<typename OBJ>
	void Set(std::string key, const OBJ& value)
	{
		LuaTable table = GetMetaTable();
		table.template Set<OBJ>(key, value);
	}

	template<typename OBJ>
	OBJ Get(std::string key) const
	{
		LuaTable table = GetMetaTable();
		return table.template Get<OBJ>(key);
	}

	TYPE* GetPointer() const
	{
		return pointer;
	}
	
    TYPE* operator->() const { return pointer; }

	template<typename RET, typename ...ARGS, size_t ...IS>
	auto Bind_helper(RET(TYPE::*func)(ARGS...), TYPE* t, int_sequence<IS...>)
		-> decltype(std::bind(func, t, placeholder_template<IS>{}...)) {
		return std::bind(func, t, placeholder_template<IS>{}...);
	}

	template<typename RET>
	void Bind(std::string name, RET(TYPE::*func)())
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateFunction<RET()>(std::bind(func, t));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET, typename ...ARGS>
	void Bind(std::string name, RET(TYPE::*func)(ARGS...))
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateFunction<RET(ARGS...)>(Bind_helper(func, t, make_int_sequence < sizeof...(ARGS)> {}));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET>
	void Bind(std::string name, RET(TYPE::*func)() const)
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateFunction<RET()>(std::bind(func, t));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET, typename ...ARGS>
	void Bind(std::string name, RET(TYPE::*func)(ARGS...) const)
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		[func](...ARGS) { return function<RET(ARGS...)>(); }
		auto luaFunc = lua.CreateFunction<RET(ARGS...)>(Bind_helper(func, t, make_int_sequence < sizeof...(ARGS)> {}));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}
	template<typename RET>
	void BindYield(std::string name, RET(TYPE::*func)())
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateYieldingFunction<RET()>(std::bind(func, t));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET, typename ...ARGS>
	void BindYield(std::string name, RET(TYPE::*func)(ARGS...))
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateYieldingFunction<RET(ARGS...)>(Bind_helper(func, t, make_int_sequence < sizeof...(ARGS)> {}));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET>
	void BindYield(std::string name, RET(TYPE::*func)() const)
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateYieldingFunction<RET()>(std::bind(func, t));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	template<typename RET, typename ...ARGS>
	void BindYield(std::string name, RET(TYPE::*func)(ARGS...) const)
	{
		using namespace std::placeholders;
		TYPE* t = RetrieveData();
		Lua lua(state);
		auto luaFunc = lua.CreateYieldingFunction<RET(ARGS...)>(Bind_helper(func, t, make_int_sequence < sizeof...(ARGS)> {}));
		LuaTable table = GetMetaTable();
		table.Set(name, luaFunc);
	}

	struct UserdataWrapper
	{
		TYPE* actualData;
		std::function< void(TYPE*) >* destructor;
	};

	static int lua_userdata_finalizer(lua_State* state)
	{
		UserdataWrapper* wrap = (UserdataWrapper*)lua_touserdata(state, lua_upvalueindex(1));
		(*wrap->destructor)(wrap->actualData);
        delete(wrap->destructor);
		return 0;
	};

	TYPE* RetrieveData() const
	{
		PushToStack(state.get());
		UserdataWrapper* wrap = (UserdataWrapper*)lua_touserdata(state.get(), -1);
		lua_pop(state.get(), 1);
		return wrap->actualData;
	}

};


#endif // LUAUSERDATA_H
