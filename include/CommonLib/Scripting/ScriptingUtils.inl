// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Core/HandledObject.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <Nazara/Math/Rect.hpp>
#include <Nazara/Math/Vector2.hpp>
#include <Nazara/Math/Vector3.hpp>

namespace tsom
{
	namespace Detail
	{
		template<typename L>
		struct LuaLambdaWrapper;

		template<typename L>
		struct LuaCallWrapper
		{
			template<typename F>
			static auto Wrap(F&& func)
			{
				return LuaLambdaWrapper<decltype(&F::operator())>::template Wrap(std::forward<F>(func));
			}
		};

		template<typename Ret, typename... Args>
		struct LuaCallWrapper<Ret(*)(Args...)>
		{
			using FuncPtr = Ret(*)(Args...);

			static auto Wrap(FuncPtr funcPtr)
			{
				return [funcPtr](sol::this_state L, Args... args)
				{
					try
					{
						return std::invoke(funcPtr, std::forward<Args>(args)...);
					}
					catch (const std::exception& e)
					{
						TriggerLuaError(L, e.what());
					}
				};
			}
		};

		template<typename O, typename Ret, typename... Args>
		struct LuaWrapper
		{
			template<typename F>
			static auto Wrap(F&& funcPtr)
			{
				return [func = std::forward<F>(funcPtr)](sol::this_state L, Args... args)
				{
					try
					{
						return std::invoke(func, std::forward<Args>(args)...);
					}
					catch (const std::exception& e)
					{
						TriggerLuaError(L, e.what());
					}
				};
			}

			template<typename F>
			static auto WrapMethod(F&& funcPtr)
			{
				static constexpr bool IsHandledObject = std::is_base_of_v<Nz::HandledObject<std::decay_t<O>>, std::decay_t<O>>;
				using ObjectParam = std::conditional_t<IsHandledObject, const Nz::ObjectHandle<std::remove_const_t<O>>&, O&>;

				return [func = std::forward<F>(funcPtr)](sol::this_state L, ObjectParam object, Args... args)
				{
					try
					{
						if constexpr (IsHandledObject)
						{
							if (!object.IsValid())
								TriggerLuaError(L, "invalid object");

							return std::invoke(func, *object, std::forward<Args>(args)...);
						}
						else
							return std::invoke(func, object, std::forward<Args>(args)...);
					}
					catch (const std::exception& e)
					{
						TriggerLuaError(L, e.what());
					}
				};
			}
		};

		template<typename O, typename Ret, typename... Args>
		struct LuaCallWrapper<Ret(O::*)(Args...)>
		{
			template<typename F>
			static auto Wrap(F&& funcPtr)
			{
				return LuaWrapper<O, Ret, Args...>::template WrapMethod(std::forward<F>(funcPtr));
			}
		};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(O&, Args...)>
		{
			template<typename F>
			static auto Wrap(F&& funcPtr)
			{
				return LuaWrapper<O, Ret, Args...>::template WrapMethod(std::forward<F>(funcPtr));
			}
		};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(const O&, Args...)>
		{
			template<typename F>
			static auto Wrap(F&& funcPtr)
			{
				return LuaWrapper<const O, Ret, Args...>::template WrapMethod(std::forward<F>(funcPtr));
			}
		};

		template<typename O, typename Ret, typename... Args>
		struct LuaCallWrapper<Ret(O::*)(Args...) noexcept> : LuaCallWrapper<Ret(O::*)(Args...)> {};

		template<typename O, typename Ret, typename... Args>
		struct LuaCallWrapper<Ret(O::*)(Args...) const> : LuaCallWrapper<Ret(O::*)(Args...)> {};

		template<typename O, typename Ret, typename... Args>
		struct LuaCallWrapper<Ret(O::*)(Args...) const noexcept> : LuaCallWrapper<Ret(O::*)(Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(O&, Args...) noexcept> : LuaLambdaWrapper<Ret(T::*)(O&, Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(O&, Args...) const> : LuaLambdaWrapper<Ret(T::*)(O&, Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(O&, Args...) const noexcept> : LuaLambdaWrapper<Ret(T::*)(O&, Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(const O&, Args...) noexcept> : LuaLambdaWrapper<Ret(T::*)(const O&, Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(const O&, Args...) const> : LuaLambdaWrapper<Ret(T::*)(const O&, Args...)> {};

		template<typename T, typename O, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(const O&, Args...) const noexcept> : LuaLambdaWrapper<Ret(T::*)(const O&, Args...)> {};

		template<typename T, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(Args...)>
		{
			template<typename F>
			static auto Wrap(F&& funcPtr)
			{
				return LuaWrapper<T, Ret, Args...>::template Wrap(std::forward<F>(funcPtr));
			}
		};

		template<typename T, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(Args...) noexcept> : LuaLambdaWrapper<Ret(T::*)(Args...)> {};

		template<typename T, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(Args...) const> : LuaLambdaWrapper<Ret(T::*)(Args...)> {};

		template<typename T, typename Ret, typename... Args>
		struct LuaLambdaWrapper<Ret(T::*)(Args...) const noexcept> : LuaLambdaWrapper<Ret(T::*)(Args...)> {};
	}

	template<typename... Args>
	[[noreturn]] void TriggerLuaError(lua_State* L, const char* format, Args&&... args)
	{
		luaL_error(L, format, std::forward<Args>(args)...);
		std::abort(); //< Will never be triggered as luaL_error will never returns, it's there because luaL_error isn't tagged noreturn
	}

	template<typename F> 
	auto LuaFunction(F funcPtr)
	{
		using Wrapper = Detail::LuaCallWrapper<F>;
		return Wrapper::Wrap(std::move(funcPtr));
	}
}

namespace sol
{
	template<typename T>
	struct lua_size<Nz::Rect<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Vector2<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Vector3<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_type_of<Nz::Rect<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Vector2<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Vector3<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	inline Nz::Rect<T> sol_lua_get(sol::types<Nz::Rect<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table rect = sol::stack::get<sol::table>(L, absoluteIndex);
		T x = rect["x"];
		T y = rect["y"];
		T width = rect["width"];
		T height = rect["height"];

		tracking.use(1);

		return Nz::Rect<T>(x, y, width, height);
	}

	template<typename T>
	Nz::Vector2<T> sol_lua_get(sol::types<Nz::Vector2<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table vec2 = sol::stack::get<sol::table>(L, absoluteIndex);
		T x = vec2["x"];
		T y = vec2["y"];

		tracking.use(1);

		return Nz::Vector2<T>(x, y);
	}

	template<typename T>
	Nz::Vector3<T> sol_lua_get(sol::types<Nz::Vector3<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table vec3 = sol::stack::get<sol::table>(L, absoluteIndex);
		T x = vec3["x"];
		T y = vec3["y"];
		T z = vec3["z"];

		tracking.use(1);

		return Nz::Vector3<T>(x, y, z);
	}


	template<typename T>
	int sol_lua_push(sol::types<Nz::Rect<T>>, lua_State* L, const Nz::Rect<T>& rect)
	{
		lua_createtable(L, 0, 4);
		luaL_setmetatable(L, "rect");
		sol::stack_table vec(L);
		vec["x"] = rect.x;
		vec["y"] = rect.y;
		vec["width"] = rect.width;
		vec["height"] = rect.height;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Vector2<T>>, lua_State* L, const Nz::Vector2<T>& v)
	{
		lua_createtable(L, 0, 2);
		luaL_setmetatable(L, "vec2");
		sol::stack_table vec(L);
		vec["x"] = v.x;
		vec["y"] = v.y;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Vector3<T>>, lua_State* L, const Nz::Vector3<T>& v)
	{
		lua_createtable(L, 0, 3);
		luaL_setmetatable(L, "vec3");
		sol::stack_table vec(L);
		vec["x"] = v.x;
		vec["y"] = v.y;
		vec["z"] = v.z;

		return 1;
	}
}
