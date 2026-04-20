// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Core/HandledObject.hpp>
#include <Nazara/Core/ObjectHandle.hpp>
#include <Nazara/Math/Box.hpp>
#include <Nazara/Math/EulerAngles.hpp>
#include <Nazara/Math/Quaternion.hpp>
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
	struct lua_size<Nz::Box<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::EulerAngles<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Quaternion<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Rect<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Vector2<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_size<Nz::Vector3<T>> : std::integral_constant<int, 1> {};

	template<typename T>
	struct lua_type_of<Nz::Box<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::EulerAngles<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Quaternion<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Rect<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Vector2<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	struct lua_type_of<Nz::Vector3<T>> : std::integral_constant<sol::type, sol::type::table> {};

	template<typename T>
	Nz::Box<T> sol_lua_get(sol::types<Nz::Box<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table box = sol::stack::get<sol::table>(L, absoluteIndex);
		T x = box["x"];
		T y = box["y"];
		T z = box["z"];
		T width = box["width"];
		T height = box["height"];
		T depth = box["depth"];

		tracking.use(1);

		return Nz::Box<T>(x, y, z, width, height, depth);
	}

	template<typename T>
	Nz::EulerAngles<T> sol_lua_get(sol::types<Nz::EulerAngles<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table angles = sol::stack::get<sol::table>(L, absoluteIndex);
		T pitch = angles["pitch"];
		T yaw = angles["yaw"];
		T roll = angles["roll"];

		tracking.use(1);

		return Nz::EulerAngles<T>(Nz::DegreeAngle<T>(pitch), Nz::DegreeAngle<T>(yaw), Nz::DegreeAngle<T>(roll));
	}

	template<typename T>
	Nz::Quaternion<T> sol_lua_get(sol::types<Nz::Quaternion<T>>, lua_State* L, int index, sol::stack::record& tracking)
	{
		int absoluteIndex = lua_absindex(L, index);

		sol::table quat = sol::stack::get<sol::table>(L, absoluteIndex);
		T x = quat["x"];
		T y = quat["y"];
		T z = quat["z"];
		T w = quat["w"];

		tracking.use(1);

		return Nz::Quaternion<T>(w, x, y, z);
	}

	template<typename T>
	Nz::Rect<T> sol_lua_get(sol::types<Nz::Rect<T>>, lua_State* L, int index, sol::stack::record& tracking)
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
	int sol_lua_push(sol::types<Nz::Box<T>>, lua_State* L, const Nz::Box<T>& box)
	{
		lua_createtable(L, 0, 6);
		luaL_setmetatable(L, "box");
		sol::stack_table boxTable(L);
		boxTable["x"] = box.x;
		boxTable["y"] = box.y;
		boxTable["z"] = box.z;
		boxTable["width"] = box.width;
		boxTable["height"] = box.height;
		boxTable["depth"] = box.height;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::EulerAngles<T>>, lua_State* L, const Nz::EulerAngles<T>& angles)
	{
		lua_createtable(L, 0, 3);
		luaL_setmetatable(L, "eulerangles");
		sol::stack_table anglesTable(L);
		anglesTable["pitch"] = angles.pitch.ToDegrees();
		anglesTable["yaw"] = angles.yaw.ToDegrees();
		anglesTable["roll"] = angles.roll.ToDegrees();

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Quaternion<T>>, lua_State* L, const Nz::Quaternion<T>& quat)
	{
		lua_createtable(L, 0, 4);
		luaL_setmetatable(L, "quaternion");
		sol::stack_table quatTable(L);
		quatTable["x"] = quat.x;
		quatTable["y"] = quat.y;
		quatTable["z"] = quat.z;
		quatTable["w"] = quat.w;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Rect<T>>, lua_State* L, const Nz::Rect<T>& rect)
	{
		lua_createtable(L, 0, 4);
		luaL_setmetatable(L, "rect");
		sol::stack_table rectTable(L);
		rectTable["x"] = rect.x;
		rectTable["y"] = rect.y;
		rectTable["width"] = rect.width;
		rectTable["height"] = rect.height;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Vector2<T>>, lua_State* L, const Nz::Vector2<T>& vec)
	{
		lua_createtable(L, 0, 2);
		luaL_setmetatable(L, "vec2");
		sol::stack_table vecTable(L);
		vecTable["x"] = vec.x;
		vecTable["y"] = vec.y;

		return 1;
	}

	template<typename T>
	int sol_lua_push(sol::types<Nz::Vector3<T>>, lua_State* L, const Nz::Vector3<T>& vec)
	{
		lua_createtable(L, 0, 3);
		luaL_setmetatable(L, "vec3");
		sol::stack_table vecTable(L);
		vecTable["x"] = vec.x;
		vecTable["y"] = vec.y;
		vecTable["z"] = vec.z;

		return 1;
	}
}
