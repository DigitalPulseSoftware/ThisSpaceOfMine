// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_SYSTEMS_AUDIOSYSTEM_HPP
#define TSOM_CLIENTLIB_SYSTEMS_AUDIOSYSTEM_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Core/EnttObserver.hpp>
#include <Nazara/Core/Time.hpp>
#include <NazaraUtils/TypeList.hpp>
#include <entt/fwd.hpp>
#include <vector>

namespace Nz
{
	class AudioEngine;
	class DisabledComponent;
	class NodeComponent;
	class Sound;
	class SoundDataSource;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API AudioSystem
	{
		public:
			static constexpr bool AllowConcurrent = true;
			static constexpr Nz::Int64 ExecutionOrder = 0;
			using Components = Nz::TypeList<Nz::NodeComponent, class AudioListenerComponent, class SoundEmitterComponent>;

			AudioSystem(entt::registry& registry, std::shared_ptr<Nz::AudioEngine> audioEngine);
			AudioSystem(const AudioSystem&) = delete;
			AudioSystem(AudioSystem&&) = delete;
			~AudioSystem();

			inline Nz::AudioEngine& GetAudioEngine();
			inline const Nz::AudioEngine& GetAudioEngine() const;

			const std::shared_ptr<Nz::Sound>& PlaySound(std::shared_ptr<Nz::SoundDataSource> soundSource);

			void Update(Nz::Time elapsedTime);

			AudioSystem& operator=(const AudioSystem&) = delete;
			AudioSystem& operator=(AudioSystem&&) = delete;

		private:
			std::shared_ptr<Nz::AudioEngine> m_audioEngine;
			std::vector<std::shared_ptr<Nz::Sound>> m_oneShotSounds;
			entt::registry& m_registry;
			Nz::EnttObserver<Nz::TypeList<Nz::NodeComponent, class AudioListenerComponent>, Nz::TypeList<>> m_listenerObserver;
			Nz::EnttObserver<Nz::TypeList<Nz::NodeComponent, class SoundEmitterComponent>, Nz::TypeList<>> m_soundObserver;
	};
}

#include <ClientLib/Systems/AudioSystem.inl>

#endif // TSOM_CLIENTLIB_SYSTEMS_AUDIOSYSTEM_HPP
