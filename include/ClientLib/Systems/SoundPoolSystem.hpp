// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_SOUNDPOOLSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_SOUNDPOOLSYSTEM_HPP

#include <Nazara/Audio/Sound.hpp>
#include <ClientLib/Export.hpp>
#include <entt/fwd.hpp>
#include <vector>

namespace tsom
{
	class TSOM_CLIENTLIB_API SoundPoolSystem
	{
		public:
			struct SoundParameters;

			inline SoundPoolSystem(entt::registry& registry);
			SoundPoolSystem(const SoundPoolSystem&) = delete;
			SoundPoolSystem(SoundPoolSystem&&) = delete;
			~SoundPoolSystem() = default;

			void PlaySound(std::shared_ptr<Nz::SoundBuffer> soundBuffer, const Nz::Vector3f& position, const SoundParameters& params);

			void Update(Nz::Time /*elapsedTime*/);

			SoundPoolSystem& operator=(const SoundPoolSystem&) = delete;
			SoundPoolSystem& operator=(SoundPoolSystem&&) = delete;

			struct SoundParameters
			{
				float pitch = 1.f;
				float volume = 1.f;
			};

		private:
			struct PlayingSound
			{
				Nz::Sound sound;
			};

			std::vector<Nz::Sound> m_soundPool;
			std::vector<PlayingSound> m_playingSounds;
			entt::registry& m_registry;
	};
}

#include <ClientLib/Systems/SoundPoolSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_SOUNDPOOLSYSTEM_HPP
