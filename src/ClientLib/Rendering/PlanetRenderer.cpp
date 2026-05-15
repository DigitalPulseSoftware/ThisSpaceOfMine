// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Rendering/PlanetRenderer.hpp>
#include <ClientLib/ClientChunkEntities.hpp>
#include <ClientLib/Rendering/RenderPlanetLayer.hpp>
#include <Nazara/Graphics/AbstractViewer.hpp>
#include <Nazara/Graphics/Graphics.hpp>
#include <Nazara/Graphics/Material.hpp>
#include <Nazara/Graphics/MaterialInstance.hpp>
#include <Nazara/Graphics/RenderResourceReferences.hpp>
#include <Nazara/Graphics/TextureAsset.hpp>
#include <Nazara/Graphics/ViewerInstance.hpp>
#include <Nazara/Graphics/WorldInstance.hpp>
#include <Nazara/Platform/Mouse.hpp>
#include <Nazara/Renderer/CommandBufferBuilder.hpp>
#include <Nazara/Math/Ray.hpp>
#include <spdlog/spdlog.h>

//#pragma optimize("", off)

namespace tsom
{
	Nz::RenderElementPoolBase& PlanetRenderer::GetPool()
	{
		return m_pool;
	}

	std::unique_ptr<Nz::ElementRendererData> PlanetRenderer::InstanciateData()
	{
		return std::make_unique<PlanetRendererData>();
	}

	void PlanetRenderer::Render(const Nz::AbstractViewer& viewer, Nz::ElementRendererData& /*rendererData*/, Nz::RenderResources& renderResources, Nz::CommandBufferBuilder& commandBuffer, std::size_t elementCount, const Nz::Pointer<const Nz::RenderElement>* elements, Nz::SparsePtr<const RenderStates> renderStates)
	{
		const Nz::RenderPipeline* currentPipeline = nullptr;

		Nz::Graphics* graphics = Nz::Graphics::Instance();
		auto& renderDevice = *graphics->GetRenderDevice();

		const Nz::ViewerInstance& viewerInstance = viewer.GetViewerInstance();

		Nz::Vector2f targetSize = viewerInstance.GetTargetSize();
		Nz::Recti fullscreenScissorBox(0, 0, Nz::SafeCast<int>(std::floor(targetSize.x)), Nz::SafeCast<int>(std::floor(targetSize.y)));
		Nz::Recti currentScissorBox(-1, -1, -1, -1);

		const auto& depthTexture2D = graphics->GetDefaultTextures().depthTextures[Nz::ImageType::E2D];
		const auto& depthTexture2DArray = graphics->GetDefaultTextures().depthTextures[Nz::ImageType::E2D_Array];
		const auto& depthTextureCube = graphics->GetDefaultTextures().depthTextures[Nz::ImageType::Cubemap];
		const auto& whiteTexture2D = graphics->GetDefaultTextures().whiteTextures[Nz::ImageType::E2D];
		const auto& defaultSampler = graphics->GetSamplerCache().Get({});
		const auto& shadowSampler = graphics->GetSamplerCache().Get({ .depthCompare = true });

		Nz::Frustumf frustum = Nz::Frustumf::Extract(viewerInstance.GetViewProjMatrix(), viewer.IsZReversed());

		for (std::size_t elementIndex = 0; elementIndex < elementCount; ++elementIndex)
		{
			const RenderPlanetLayer* renderPlanetLayer = Nz::SafeCast<const RenderPlanetLayer*>(elements[elementIndex]);

			ClientChunkEntities& clientChunkEntities = renderPlanetLayer->GetClientChunkEntities();

			const Nz::MaterialInstance& materialInstance = *clientChunkEntities.GetMaterial();
			const Nz::RenderPipeline* renderPipeline = renderPlanetLayer->GetRenderPipeline();
			const Nz::Recti& scissorBox = renderPlanetLayer->GetScissorBox();
			const Nz::WorldInstance& worldInstance = renderPlanetLayer->GetWorldInstance();

			const Nz::Material& material = *materialInstance.GetParentMaterial();

			if (currentPipeline != renderPipeline)
			{
				commandBuffer.BindRenderPipeline(*renderPipeline);
				currentPipeline = renderPipeline;
			}

			const Nz::Recti& targetScissorBox = (scissorBox.width >= 0) ? scissorBox : fullscreenScissorBox;
			if (currentScissorBox != targetScissorBox)
			{
				commandBuffer.SetScissor(targetScissorBox);
				currentScissorBox = targetScissorBox;
			}

			Nz::RenderResourceReferences renderResourceReferences;

			const RenderStates& renderState = renderStates[elementIndex];

			m_bindingCache.clear();
			m_textureBindingCache.clear();
			m_textureBindingCache.reserve(renderState.shadowMapsSpot.size() + renderState.shadowMapsDirectional.size() + renderState.shadowMapsPoint.size());

			materialInstance.FillShaderBinding(renderResourceReferences, m_bindingCache);

			// Predefined shader bindings
			if (Nz::UInt32 bindingIndex = material.GetEngineBindingIndex(Nz::EngineShaderBinding::LightDataUbo); bindingIndex != Nz::Material::InvalidBindingIndex && renderState.lightData)
			{
				auto& bindingEntry = m_bindingCache.emplace_back();
				bindingEntry.bindingIndex = bindingIndex;
				bindingEntry.content = Nz::ShaderBinding::UniformBufferBinding{
					renderState.lightData.GetBuffer(),
					renderState.lightData.GetOffset(), renderState.lightData.GetSize()
				};
			}

			if (Nz::UInt32 bindingIndex = material.GetEngineBindingIndex(Nz::EngineShaderBinding::ShadowmapDirectional); bindingIndex != Nz::Material::InvalidBindingIndex)
			{
				std::size_t textureBindingBaseIndex = m_textureBindingCache.size();

				for (std::size_t j = 0; j < renderState.shadowMapsDirectional.size(); ++j)
				{
					const Nz::Texture* texture = renderState.shadowMapsDirectional[j];
					if (!texture)
						texture = depthTexture2DArray->GetOrCreateTexture(renderDevice).get();

					auto& textureEntry = m_textureBindingCache.emplace_back();
					textureEntry.texture = texture;
					textureEntry.sampler = shadowSampler.get();
				}

				auto& bindingEntry = m_bindingCache.emplace_back();
				bindingEntry.bindingIndex = bindingIndex;
				bindingEntry.content = Nz::ShaderBinding::SampledTextureBindings{
					Nz::SafeCast<Nz::UInt32>(renderState.shadowMapsDirectional.size()), &m_textureBindingCache[textureBindingBaseIndex]
				};
			}

			if (Nz::UInt32 bindingIndex = material.GetEngineBindingIndex(Nz::EngineShaderBinding::ShadowmapPoint); bindingIndex != Nz::Material::InvalidBindingIndex)
			{
				std::size_t textureBindingBaseIndex = m_textureBindingCache.size();

				for (std::size_t j = 0; j < renderState.shadowMapsPoint.size(); ++j)
				{
					const Nz::Texture* texture = renderState.shadowMapsPoint[j];
					if (!texture)
						texture = depthTextureCube->GetOrCreateTexture(renderDevice).get();

					auto& textureEntry = m_textureBindingCache.emplace_back();
					textureEntry.texture = texture;
					textureEntry.sampler = defaultSampler.get(); //< cube shadowmap don't use depth compare
				}

				auto& bindingEntry = m_bindingCache.emplace_back();
				bindingEntry.bindingIndex = bindingIndex;
				bindingEntry.content = Nz::ShaderBinding::SampledTextureBindings{
					Nz::SafeCast<Nz::UInt32>(renderState.shadowMapsPoint.size()), &m_textureBindingCache[textureBindingBaseIndex]
				};
			}

			if (Nz::UInt32 bindingIndex = material.GetEngineBindingIndex(Nz::EngineShaderBinding::ShadowmapSpot); bindingIndex != Nz::Material::InvalidBindingIndex)
			{
				std::size_t textureBindingBaseIndex = m_textureBindingCache.size();

				for (std::size_t j = 0; j < renderState.shadowMapsSpot.size(); ++j)
				{
					const Nz::Texture* texture = renderState.shadowMapsSpot[j];
					if (!texture)
						texture = depthTexture2D->GetOrCreateTexture(renderDevice).get();

					auto& textureEntry = m_textureBindingCache.emplace_back();
					textureEntry.texture = texture;
					textureEntry.sampler = shadowSampler.get();
				}

				auto& bindingEntry = m_bindingCache.emplace_back();
				bindingEntry.bindingIndex = bindingIndex;
				bindingEntry.content = Nz::ShaderBinding::SampledTextureBindings{
					Nz::SafeCast<Nz::UInt32>(renderState.shadowMapsSpot.size()), &m_textureBindingCache[textureBindingBaseIndex]
				};
			}

			if (Nz::UInt32 bindingIndex = material.GetEngineBindingIndex(Nz::EngineShaderBinding::ViewerDataUbo); bindingIndex != Nz::Material::InvalidBindingIndex)
			{
				const auto& viewerBuffer = viewer.GetViewerInstance().GetViewerBuffer();

				auto& bindingEntry = m_bindingCache.emplace_back();
				bindingEntry.bindingIndex = bindingIndex;
				bindingEntry.content = Nz::ShaderBinding::UniformBufferBinding{
					viewerBuffer.get(),
					0, viewerBuffer->GetSize()
				};
			}

			Nz::RenderPipelineLayout& renderPipelineLayout = *currentPipeline->GetPipelineInfo().pipelineLayout;

			Nz::ShaderBindingPtr drawDataBinding = renderPipelineLayout.AllocateShaderBinding(0);
			drawDataBinding->Update(m_bindingCache.data(), m_bindingCache.size());

			commandBuffer.BindRenderShaderBinding(0, *drawDataBinding);

			const Nz::Matrix4f& worldMatrix = worldInstance.GetWorldMatrix();
			const Nz::Matrix4f& invWorldMatrix = worldInstance.GetWorldMatrix();
			const ChunkContainer& chunkContainer = clientChunkEntities.GetChunkContainer();

			m_drawList.clear();

			constexpr Nz::UInt8 maxSteps = 10;

			struct PendingQueue
			{
				ChunkIndices indices;
				std::optional<Direction> from;
				DirectionMask allowedDirections;
				Nz::UInt8 remainingSteps = maxSteps;
			};
			std::vector<PendingQueue> pendingList;

			Nz::Vector3f relativePosition = invWorldMatrix * viewerInstance.GetEyePosition();

			Nz::Vector3f cameraDirection = invWorldMatrix * Nz::Vector3f(viewerInstance.GetInvViewMatrix() * Nz::Vector4f(Nz::Vector3f::Forward(), 0.f));

			/*DirectionMask cameraAllowedDirections;
			for (auto&& [direction, normal] : s_dirNormals.iter_kv())
			{
				float dot = cameraDirection.DotProduct(normal);
				if (dot >= -0.9)
					cameraAllowedDirections |= direction;
			}*/

			tsl::hopscotch_set<ChunkIndices> visited;

			Direction cameraDir = DirectionFromNormal(cameraDirection);

			ChunkIndices startingIndices;
			Nz::Boxf containerAABB = chunkContainer.GetAABB();
			if (containerAABB.Contains(relativePosition))
			{
				startingIndices = chunkContainer.GetChunkIndicesByPosition(relativePosition);
				pendingList.push_back({ startingIndices, {}, DirectionMask_All});
			}
			else
			{
				Nz::Rayf ray(relativePosition, cameraDirection);

				float closestHit;
				if (ray.Intersect(containerAABB, &closestHit, nullptr))
				{
					startingIndices = chunkContainer.GetChunkIndicesByPosition(ray.GetPoint(closestHit) + cameraDirection);
					pendingList.push_back({ startingIndices, {}, DirectionMask_All});
				}
			}

			while (!pendingList.empty())
			{
				PendingQueue pendingChunk = pendingList.front();
				pendingList.erase(pendingList.begin());

				const Chunk* chunk = chunkContainer.GetChunk(pendingChunk.indices);
				if (!chunk)
					continue;

				Nz::Vector3f chunkSize = Nz::Vector3f(chunk->GetSize()) * chunk->GetBlockSize();
				Nz::Boxf chunkAABB(chunkSize * -0.5f, chunkSize);
				chunkAABB.Translate(chunkContainer.GetChunkOffset(pendingChunk.indices));

				if (frustum.Intersect(chunkAABB) == Nz::IntersectionSide::Outside)
					continue;

				if (chunk->HasContent())
					m_drawList.push_back(pendingChunk.indices);

				if (pendingChunk.remainingSteps == 0)
					continue;


				DirectionMask faceVisibilityMask = DirectionMask_All;
				if (pendingChunk.from)
				{
					faceVisibilityMask = chunk->GetFaceVisibilityMasks()[*pendingChunk.from];
					faceVisibilityMask &= pendingChunk.allowedDirections;
					//faceVisibilityMask &= cameraAllowedDirections;
				}

				for (Direction visibleDirection : faceVisibilityMask)
				{
					ChunkIndices neighborIndices = pendingChunk.indices + s_chunkDirOffset[visibleDirection];
					if (!visited.contains(neighborIndices))
					{
						visited.emplace(neighborIndices);

						// Only consume steps if direction doesn't line up with the camera
						Nz::UInt8 remainingSteps = pendingChunk.remainingSteps;
						if (Nz::Vector3f::DotProduct(cameraDirection, s_dirNormals[visibleDirection]) < 0.0) //< cos(60)
							remainingSteps--;

						pendingList.push_back({ neighborIndices, visibleDirection, pendingChunk.allowedDirections & ~s_oppositeDirections[visibleDirection], remainingSteps });
					}
				}
			}

			/*std::size_t nonEmptyChunk = 0;
			chunkContainer.ForEachChunk([&](const ChunkIndices& chunkIndices, const Chunk& chunk)
			{
				if (chunk.HasContent())
					nonEmptyChunk++;
			});

			spdlog::info("{}/{} chunks visible ({}%)", m_drawList.size(), nonEmptyChunk, m_drawList.size() * 100 / std::max<std::size_t>(nonEmptyChunk, 1));*/

			for (const ChunkIndices& indices : m_drawList)
			{
				const Nz::GraphicalMesh* graphicalMesh = clientChunkEntities.GetChunkMesh(indices);
				if (!graphicalMesh)
					continue;

				NazaraAssert(graphicalMesh->GetSubMeshCount() == 1);

				commandBuffer.BindIndexBuffer(*graphicalMesh->GetIndexBuffer(0), graphicalMesh->GetIndexType(0));
				commandBuffer.BindVertexBuffer(0, *graphicalMesh->GetVertexBuffer(0));

				renderResourceReferences.renderBuffers.emplace(graphicalMesh->GetIndexBuffer(0));
				renderResourceReferences.renderBuffers.emplace(graphicalMesh->GetVertexBuffer(0));

				Nz::Matrix4f chunkWorldMatrix = Nz::Matrix4f::Translate(chunkContainer.GetChunkOffset(indices)) * worldMatrix;
				commandBuffer.PushConstants(renderPipelineLayout, 0, sizeof(chunkWorldMatrix), &chunkWorldMatrix);

				commandBuffer.DrawIndexed(graphicalMesh->GetIndexCount(0), 1U, 0);
			}

			renderResources.PushForRelease(std::move(renderResourceReferences));
			renderResources.PushForRelease(std::move(drawDataBinding));
		}
	}
}

#pragma optimize("", on)
