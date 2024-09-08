// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientScriptingLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <CommonLib/Scripting/ScriptingUtils.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <Nazara/Core/FilesystemAppComponent.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/Model.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <NazaraUtils/FunctionTraits.hpp>
#include <fmt/format.h>
#include <sol/state.hpp>

SOL_BASE_CLASSES(Nz::Model, Nz::InstancedRenderable);
SOL_DERIVED_CLASSES(Nz::InstancedRenderable, Nz::Model);

namespace tsom
{
	void ClientScriptingLibrary::Register(sol::state& state)
	{
		state["CLIENT"] = true;
		state["SERVER"] = false;

		RegisterAssetLibrary(state);
		RegisterMaterialInstance(state);
		RegisterRenderables(state);
		RegisterTexture(state);
	}

	void ClientScriptingLibrary::RegisterAssetLibrary(sol::state& state)
	{
		sol::table assetLibrary = state.create_named_table("AssetLibrary");

		assetLibrary["GetModel"] = LuaFunction([this](std::string_view name)
		{
			auto& clientAsset = m_app.GetComponent<ClientAssetLibraryAppComponent>();
			return clientAsset.GetModel(name);
		});

		assetLibrary["RegisterModel"] = LuaFunction([this](std::string name, std::shared_ptr<Nz::Model> model)
		{
			auto& clientAsset = m_app.GetComponent<ClientAssetLibraryAppComponent>();
			clientAsset.RegisterModel(std::move(name), std::move(model));
		});
	}

	void ClientScriptingLibrary::RegisterMaterialInstance(sol::state& state)
	{
		state.new_usertype<Nz::MaterialInstance>(
			"MaterialInstance", sol::no_constructor,

			"SetTextureProperty", LuaFunction(Nz::Overload<std::string_view, std::shared_ptr<Nz::TextureAsset>>(&Nz::MaterialInstance::SetTextureProperty)),

			"Instantiate", LuaFunction([this](std::string_view matType)
			{
				std::shared_ptr<Nz::MaterialInstance> matInstance;
				if (matType == "basic")
					matInstance = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Basic);
				else if (matType == "phong")
					matInstance = Nz::MaterialInstance::Instantiate(Nz::MaterialType::Phong);
				else if (matType == "pbr")
					matInstance = Nz::MaterialInstance::Instantiate(Nz::MaterialType::PhysicallyBased);
				else
					throw std::runtime_error(fmt::format("unknown material type {}", matType));

				return matInstance;
			})
		);
	}

	void ClientScriptingLibrary::RegisterRenderables(sol::state& state)
	{
		state.new_usertype<Nz::InstancedRenderable>("InstancedRenderable",
			sol::no_constructor,
			"GetAABB", LuaFunction(&Nz::InstancedRenderable::GetAABB)
		);

		state.new_usertype<Nz::Model>("Model",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::InstancedRenderable>(),
			"SetMaterial", LuaFunction(&Nz::Model::SetMaterial),
			"Load", LuaFunction([this](std::string assetPath, sol::optional<sol::table> paramOpt)
			{
				Nz::Model::Params params;
				if (paramOpt)
				{
					sol::table& paramTable = *paramOpt;
					params.loadMaterials = paramTable.get_or("loadMaterials", params.loadMaterials);

					if (sol::optional<sol::table> meshParamsOpt = paramTable["mesh"])
					{
						sol::table meshParams = *meshParamsOpt;
						params.mesh.animated = meshParams.get_or("animated", params.mesh.animated);
						params.mesh.center = meshParams.get_or("center", params.mesh.center);
						params.mesh.texCoordOffset = meshParams.get_or("texCoordOffset", params.mesh.texCoordOffset);
						params.mesh.texCoordScale = meshParams.get_or("texCoordScale", params.mesh.texCoordScale);
						params.mesh.vertexOffset = meshParams.get_or("vertexOffset", params.mesh.vertexOffset);
						params.mesh.vertexRotation = meshParams.get_or<Nz::EulerAnglesf>("vertexRotation", params.mesh.vertexRotation);
						params.mesh.vertexScale = meshParams.get_or("vertexScale", params.mesh.vertexScale);
					}
				}

				auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();
				std::shared_ptr<Nz::Model> model = fs.Load<Nz::Model>(assetPath, std::move(params));
				if (!model)
					throw std::runtime_error("failed to load " + assetPath);

				return model;
			})
		);
	}

	void ClientScriptingLibrary::RegisterTexture(sol::state& state)
	{
		state.new_usertype<Nz::TextureAsset>("Texture",
			sol::no_constructor,
			"Load", LuaFunction([this](std::string assetPath)
			{
				auto& fs = m_app.GetComponent<Nz::FilesystemAppComponent>();
				std::shared_ptr<Nz::TextureAsset> texture = fs.Open<Nz::TextureAsset>(assetPath);
				if (!texture)
					throw std::runtime_error("failed to load " + assetPath);

				return texture;
			})
		);
	}
}
