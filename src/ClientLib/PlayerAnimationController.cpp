// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/PlayerAnimationController.hpp>
#include <ClientLib/ClientBlockLibrary.hpp>
#include <ClientLib/Systems/AudioSystem.hpp>
#include <CommonLib/Chunk.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/GameConstants.hpp>
#include <CommonLib/Components/ChunkComponent.hpp>
#include <CommonLib/Utility/RandomNumberGenerator.hpp>
#include <Nazara/Audio/Sound.hpp>
#include <Nazara/Audio/SoundBuffer.hpp>
#include <Nazara/Core/Animation.hpp>
#include <Nazara/Core/EnttWorld.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <Nazara/Physics3D/Systems/Physics3DSystem.hpp>
#include <fmt/core.h>

namespace tsom
{
	PlayerAnimationController::PlayerAnimationController(ClientBlockLibrary& blockLibrary, entt::handle entity, std::shared_ptr<PlayerAssets> playerAssets) :
	m_playerAssets(std::move(playerAssets)),
	m_entity(entity),
	m_idleRunBlender(m_playerAssets->referenceSkeleton),
	m_footstepAccumulator(Nz::Time::Zero()),
	m_playerVelocities(20),
	m_blockLibrary(blockLibrary)
	{
		m_idleRunBlender.AddPoint(0.f, m_playerAssets->idleAnimation);
		m_idleRunBlender.AddPoint(Constants::PlayerWalkSpeed, m_playerAssets->walkingAnimation);
		m_idleRunBlender.AddPoint(Constants::PlayerSprintSpeed, m_playerAssets->runningAnimation);

		Nz::NodeComponent& node = entity.get<Nz::NodeComponent>();
		m_playerPosition = node.GetPosition();
	}

	void PlayerAnimationController::Animate(Nz::Skeleton& skeleton, Nz::Time elapsedTime)
	{
		m_idleRunBlender.UpdateAnimation(elapsedTime);
		m_idleRunBlender.AnimateSkeleton(&skeleton);
	}

	void PlayerAnimationController::UpdateStates(Nz::Time elapsedTime)
	{
		float deltaSeconds = elapsedTime.AsSeconds();

		// Client doesn't know player speed as physics is not yet replicated from server
		Nz::NodeComponent& node = m_entity.get<Nz::NodeComponent>();
		Nz::Vector3f newPosition = node.GetPosition();
		Nz::Vector3f up = node.GetUp();

		float playerVelocity = Nz::Vector3f::Distance(newPosition.ProjectOnPlane(up), m_playerPosition.ProjectOnPlane(up)) / deltaSeconds;
		m_playerPosition = newPosition;

		m_playerVelocities.InsertValue(playerVelocity);
		float averageVelocity = m_playerVelocities.GetAverageValue();

		m_idleRunBlender.UpdateValue(averageVelocity);

		if (averageVelocity > 0.1f) //< epsilon
		{
			m_footstepAccumulator += Nz::Time::Seconds(deltaSeconds * averageVelocity / Constants::PlayerWalkSpeed);
			if (m_footstepAccumulator >= Nz::Time::Seconds(0.5f))
			{
				entt::registry* reg = m_entity.registry();
				Nz::EnttWorld* world = reg->ctx().get<Nz::EnttWorld*>();
				Nz::Physics3DSystem& physWorld = world->GetSystem<Nz::Physics3DSystem>();

				Nz::Physics3DSystem::RaycastHit closestHit;
				auto callback = [&](const Nz::Physics3DSystem::RaycastHit& hitInfo)
				{
					closestHit = hitInfo;
				};

				if (physWorld.RaycastQueryFirst(m_playerPosition, m_playerPosition - up * (Constants::PlayerColliderHeight * 0.5f) - up * 0.1f, callback) && closestHit.hitEntity)
				{
					if (auto* chunkComponent = closestHit.hitEntity.try_get<ChunkComponent>())
					{
						auto& chunkRigidBody = closestHit.hitEntity.get<Nz::RigidBody3DComponent>();
						auto& chunkNode = closestHit.hitEntity.get<Nz::NodeComponent>();

						const Chunk& hitChunk = *chunkComponent->chunk;
						const ChunkContainer& chunkContainer = hitChunk.GetContainer();

						Nz::Vector3f localPos = chunkNode.ToLocalPosition(closestHit.hitPosition);
						Nz::Vector3f localNormal = chunkNode.ToLocalDirection(closestHit.hitNormal);

						auto hitCoordinates = hitChunk.ComputeHitCoordinates(localPos, localNormal, *chunkRigidBody.GetCollider(), closestHit.subShapeID);
						if (hitCoordinates)
						{
							AudioSystem& audioSystem = world->GetSystem<AudioSystem>();

							BlockIndex blockIndex = hitChunk.GetBlockContent(hitCoordinates->blockIndices);
							const auto& blockData = m_blockLibrary.GetBlockData(blockIndex);

							const auto& footstepSounds = m_playerAssets->footstepSounds[blockData.surfaceType];
							if (!footstepSounds.empty())
							{
								RandomNumberGenerator& rng = RandomNumberGenerator::GetThreadGenerator();

								std::size_t footstepIndex = rng.RandomInteger<std::size_t>(0, footstepSounds.size() - 1);
								float footstepPitch = rng.RandomFloat(0.8f, 1.2f);

								const auto& footstepSound = audioSystem.PlaySound(footstepSounds[footstepIndex]);
								footstepSound->SetAttenuationModel(Nz::SoundAttenuationModel::Linear);
								footstepSound->SetPitch(footstepPitch);
								footstepSound->SetPosition(closestHit.hitPosition);
								footstepSound->SetMaxDistance(20.f);
								footstepSound->SetMinDistance(5.f);
								footstepSound->Play();
							}
							//fmt::println("Block index: {}", blockIndex);
						}
					}
				}

				m_footstepAccumulator = Nz::Time::Zero();
			}
		}
		else
			m_footstepAccumulator = Nz::Time::Zero();
	}
}
