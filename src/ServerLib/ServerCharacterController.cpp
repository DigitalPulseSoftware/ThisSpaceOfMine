// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ServerLib/ServerCharacterController.hpp>
#include <ServerLib/ServerEnvironment.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/GravityController.hpp>

namespace tsom
{
	ServerCharacterController::ServerCharacterController(ServerEnvironment* environment) :
	m_environment(environment)
	{
	}

	void ServerCharacterController::PreSimulate(Nz::PhysCharacter3D& character, float elapsedTime)
	{
		CharacterController::PreSimulate(character, elapsedTime);

		// FIXME: The water detection is a bit broken due to the collisions being non-convex, which means a player isn't considered submerged
		// when they're not touching the surface, rely on block detection for now for players

		// TODO: Move back this class to CommonLib once this is fixed
		Nz::Vector3f playerPos = character.GetPosition();
		ChunkContainer& chunkContainer = m_environment->GetChunkContainer();
		BlockIndex waterBlockIndex = chunkContainer.GetBlockLibrary().GetBlockIndex("water");
		ChunkIndices chunkIndices = chunkContainer.GetChunkIndicesByPosition(playerPos);
		Chunk* chunk = chunkContainer.GetChunk(chunkIndices);
		if (!chunk)
		{
			SetInWater(false);
			return;
		}

		std::optional<Nz::Vector3ui> blockIndices = chunk->ComputeCoordinates(playerPos - chunkContainer.GetChunkOffset(chunkIndices));
		if (!blockIndices || chunk->GetBlockContent(*blockIndices) != waterBlockIndex)
		{
			SetInWater(false);
			return;
		}

		SetInWater(true);

		if (const GravityController* gravityController = GetGravityController())
		{
			auto gravity = gravityController->ComputeGravity(playerPos);
			character.ApplyBuoyancyImpulse(playerPos, -gravity.direction, 1.f, 0.75f, 0.01f, Nz::Vector3f::Zero(), gravity.acceleration * gravity.direction * gravity.factor, elapsedTime);
		}
	}

	void ServerCharacterController::UpdateEnvironment(ServerEnvironment* environment)
	{
		m_environment = environment;
	}
}
