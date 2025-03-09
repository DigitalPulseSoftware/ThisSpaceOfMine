// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Scripting/ClientAssetScriptingLibrary.hpp>
#include <ClientLib/ClientAssetLibraryAppComponent.hpp>
#include <ClientLib/ClientSessionHandler.hpp>
#include <ClientLib/Scripting/ClientAssetScriptingLibrary.hpp>
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

namespace sol
{
	// Don't treat Nz::Flags as containers (despite having begin() and end())
	template<typename E>
	struct is_container<Nz::Flags<E>> : std::false_type {};

	// Not sure why it's required for sol::var_wrapper as well
	template<typename E>
	struct is_container<sol::var_wrapper<Nz::Flags<E>>> : std::false_type {};
}

namespace tsom
{
	void ClientAssetScriptingLibrary::Register(sol::state& state)
	{
		AssetScriptingLibrary::Register(state);

		state["CLIENT"] = true;
		state["SERVER"] = false;

		RegisterAssetLibrary(state);
		RegisterMaterialInstance(state);
		RegisterRenderables(state);
		RegisterRenderStates(state);
		RegisterTexture(state);
	}

	void ClientAssetScriptingLibrary::RegisterAssetLibrary(sol::state& state)
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

	void ClientAssetScriptingLibrary::RegisterMaterialInstance(sol::state& state)
	{
		state.new_enum("MaterialType",
			"Basic", Nz::MaterialType::Basic,
			"Phong", Nz::MaterialType::Phong,
			"PhysicallyBased", Nz::MaterialType::PhysicallyBased
		);

		state.new_usertype<Nz::MaterialInstancePresetFlags>("MaterialInstancePresetFlags",
			sol::no_constructor,

			"Default",      sol::var(Nz::MaterialInstancePresetFlags{}),
			"NoDepth",      sol::var(Nz::MaterialInstancePresetFlags(Nz::MaterialInstancePreset::NoDepth)),
			"AlphaBlended", sol::var(Nz::MaterialInstancePresetFlags(Nz::MaterialInstancePreset::AlphaBlended)),

			sol::meta_function::bitwise_or, &Nz::MaterialInstancePresetFlags::operator|
		);

		state.new_usertype<Nz::MaterialInstance>(
			"MaterialInstance", sol::no_constructor,

			"SetTextureProperty", LuaFunction(Nz::Overload<std::string_view, std::shared_ptr<Nz::TextureAsset>>(&Nz::MaterialInstance::SetTextureProperty)),
			"SetValueProperty", sol::overload(
				LuaFunction([](Nz::MaterialInstance& matInstance, std::string_view propertyName, bool propertyValue)
				{
					matInstance.SetValueProperty(propertyName, propertyValue);
				}),
				LuaFunction([](Nz::MaterialInstance& matInstance, std::string_view propertyName, float propertyValue)
				{
					matInstance.SetValueProperty(propertyName, propertyValue);
				})
			),

			"UpdatePassStates", LuaFunction([](Nz::MaterialInstance& matInstance, std::string_view passName, sol::protected_function callback)
			{
				matInstance.UpdatePassStates(passName, [&](Nz::RenderStates& renderStates)
				{
					auto result = callback(&renderStates);
					if (!result.valid())
					{
						sol::error err = result;
						throw std::runtime_error(fmt::format("callback failed: {}", err.what()));
					}

					return true;
				});
			}),

			"UpdatePassesStates", LuaFunction([](Nz::MaterialInstance& matInstance, sol::protected_function callback)
			{
				matInstance.UpdatePassesStates([&](Nz::RenderStates& renderStates)
				{
					auto result = callback(&renderStates);
					if (!result.valid())
					{
						sol::error err = result;
						throw std::runtime_error(fmt::format("callback failed: {}", err.what()));
					}

					return true;
				});
			}),

			"Instantiate", LuaFunction([this](Nz::MaterialType matType, std::optional<Nz::MaterialInstancePresetFlags> presetFlags)
			{
				return Nz::MaterialInstance::Instantiate(matType, presetFlags.value_or(Nz::MaterialInstancePresetFlags{}) | Nz::MaterialInstancePreset::ReverseZ);
			})
		);
	}

	void ClientAssetScriptingLibrary::RegisterRenderables(sol::state& state)
	{
		state.new_usertype<Nz::InstancedRenderable>("InstancedRenderable",
			sol::no_constructor,
			"GetAABB", LuaFunction(&Nz::InstancedRenderable::GetAABB),
			"GetMaterial", LuaFunction(&Nz::InstancedRenderable::GetMaterial),
			"GetMaterialCount", LuaFunction(&Nz::InstancedRenderable::GetMaterialCount)
		);

		state.new_usertype<Nz::Model>("Model",
			sol::no_constructor,
			sol::base_classes, sol::bases<Nz::InstancedRenderable>(),
			"SetMaterial", LuaFunction(&Nz::Model::SetMaterial),
			"BuildFromMesh", LuaFunction([](const Nz::Mesh& mesh)
			{
				std::shared_ptr<Nz::Model> model = Nz::Model::BuildFromMesh(mesh);

				// Fix reverse-depth
				std::size_t materialCount = model->GetMaterialCount();
				for (std::size_t i = 0; i < materialCount; ++i)
				{
					const auto& matPtr = model->GetMaterial(i);
					matPtr->ApplyPreset(Nz::MaterialInstancePreset::ReverseZ);
				}

				return model;
			}),
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

				if (params.loadMaterials)
				{
					// Fix reverse-depth
					std::size_t materialCount = model->GetMaterialCount();
					for (std::size_t i = 0; i < materialCount; ++i)
					{
						const auto& matPtr = model->GetMaterial(i);
						matPtr->ApplyPreset(Nz::MaterialInstancePreset::ReverseZ);
					}
				}

				return model;
			})
		);
	}

	void ClientAssetScriptingLibrary::RegisterRenderStates(sol::state& state)
	{
		state.new_enum("FaceCulling",
			"Basic", Nz::FaceCulling::Back,
			"Front", Nz::FaceCulling::Front,
			"FrontAndBack", Nz::FaceCulling::FrontAndBack,
			"None", Nz::FaceCulling::None
		);

		state.new_usertype<Nz::RenderStates>("RenderStates",
			"faceCulling", &Nz::RenderStates::faceCulling,

			"blending",    &Nz::RenderStates::blending,
			"depthBias",   &Nz::RenderStates::depthBias,
			"depthBuffer", &Nz::RenderStates::depthBuffer,
			"depthClamp",  &Nz::RenderStates::depthClamp,
			"depthWrite",  &Nz::RenderStates::depthWrite,
			"scissorTest", &Nz::RenderStates::scissorTest,
			"stencilTest", &Nz::RenderStates::stencilTest,

			"depthBiasConstantFactor", &Nz::RenderStates::depthBiasConstantFactor,
			"depthBiasSlopeFactor", &Nz::RenderStates::depthBiasSlopeFactor,
			"lineWidth", &Nz::RenderStates::lineWidth,
			"pointSize", &Nz::RenderStates::pointSize
		);
	}

	void ClientAssetScriptingLibrary::RegisterTexture(sol::state& state)
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
