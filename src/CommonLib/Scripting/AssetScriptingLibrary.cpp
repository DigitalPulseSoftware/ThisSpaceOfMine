// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <CommonLib/Scripting/AssetScriptingLibrary.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Core/Mesh.hpp>
#include <Nazara/Core/SkeletalMesh.hpp>
#include <Nazara/Core/StaticMesh.hpp>

SOL_BASE_CLASSES(Nz::SkeletalMesh, Nz::SubMesh);
SOL_BASE_CLASSES(Nz::StaticMesh, Nz::SubMesh);
SOL_DERIVED_CLASSES(Nz::SubMesh, Nz::SkeletalMesh, Nz::StaticMesh);

namespace tsom
{
	void AssetScriptingLibrary::Register(sol::state& state)
	{
		RegisterMesh(state);
		RegisterSubMesh(state);
	}

	void AssetScriptingLibrary::RegisterMesh(sol::state& state)
	{
		state.new_usertype<Nz::Mesh>("Mesh",
			sol::no_constructor,
			"AddSubMesh", sol::overload(
				LuaFunction(Nz::Overload<std::shared_ptr<Nz::SubMesh>>(&Nz::Mesh::AddSubMesh)),
				LuaFunction(Nz::Overload<std::string, std::shared_ptr<Nz::SubMesh>>(&Nz::Mesh::AddSubMesh))
			),
			"GetAABB", LuaFunction(&Nz::Mesh::GetAABB),
			"GetMaterialData", LuaFunction([](const Nz::Mesh& mesh, std::size_t index)
			{
				return mesh.GetMaterialData(index);
			}),
			"GetSubMesh", sol::overload(
				LuaFunction(Nz::Overload<std::string_view>(&Nz::Mesh::GetSubMesh)),
				LuaFunction(Nz::Overload<std::size_t>(&Nz::Mesh::GetSubMesh))
			),
			"GetSubMeshCount", LuaFunction(&Nz::Mesh::GetSubMeshCount),
			"Recenter", LuaFunction(&Nz::Mesh::Recenter),
			"SetMaterialCount", LuaFunction(&Nz::Mesh::SetMaterialCount),
			"SetMaterialData", LuaFunction(&Nz::Mesh::SetMaterialData),
			"Translate", LuaFunction([](Nz::Mesh& mesh, const Nz::Vector3f& translation)
			{
				mesh.Transform(Nz::Matrix4f::Translate(translation));
			}),

			"CreateSkeletal", LuaFunction([](std::size_t jointCount)
			{
				std::shared_ptr<Nz::Mesh> mesh = std::make_shared<Nz::Mesh>();
				mesh->CreateSkeletal(jointCount);
				return mesh;
			}),
			"CreateStatic", LuaFunction([]
			{
				std::shared_ptr<Nz::Mesh> mesh = std::make_shared<Nz::Mesh>();
				mesh->CreateStatic();
				return mesh;
			}),
			"Load", LuaFunction([this](std::string assetPath, sol::optional<sol::table> paramOpt)
			{
				Nz::Mesh::Params params;
				if (paramOpt)
				{
					sol::table& meshParams = *paramOpt;
					params.animated = meshParams.get_or("animated", params.animated);
					params.center = meshParams.get_or("center", params.center);
					params.texCoordOffset = meshParams.get_or("texCoordOffset", params.texCoordOffset);
					params.texCoordScale = meshParams.get_or("texCoordScale", params.texCoordScale);
					params.vertexOffset = meshParams.get_or("vertexOffset", params.vertexOffset);
					params.vertexRotation = meshParams.get_or<Nz::EulerAnglesf>("vertexRotation", params.vertexRotation);
					params.vertexScale = meshParams.get_or("vertexScale", params.vertexScale);
				}

				auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();
				std::shared_ptr<Nz::Mesh> mesh = fs.Load<Nz::Mesh>(assetPath, std::move(params));
				if (!mesh)
					throw std::runtime_error("failed to load " + assetPath);

				return mesh;
			})
		);
	}

	void AssetScriptingLibrary::RegisterSubMesh(sol::state& state)
	{
		state.new_usertype<Nz::SubMesh>("SubMesh",
			sol::no_constructor,
			"GetMaterialIndex", LuaFunction(&Nz::SubMesh::GetMaterialIndex),
			"SetMaterialIndex", LuaFunction(&Nz::SubMesh::SetMaterialIndex)
		);

		state.new_usertype<Nz::SkeletalMesh>("SkeletalMesh",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::SubMesh>()
		);

		state.new_usertype<Nz::StaticMesh>("StaticMesh",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::SubMesh>()
		);
	}
}
