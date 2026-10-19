// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/AudioSystem.hpp>
#include <ClientLib/Components/AudioListenerComponent.hpp>
#include <ClientLib/Components/SoundEmitterComponent.hpp>
#include <Nazara/Audio/AudioEngine.hpp>
#include <Nazara/Core/Components/DisabledComponent.hpp>
#include <Nazara/Core/Components/NodeComponent.hpp>
#include <entt/entt.hpp>

namespace tsom
{
	AudioSystem::AudioSystem(entt::registry& registry, std::shared_ptr<Nz::AudioEngine> audioEngine) :
	m_audioEngine(std::move(audioEngine)),
	m_registry(registry),
	m_listenerObserver(registry),
	m_soundObserver(registry)
	{
		m_listenerObserver.OnEntityAdded.Connect([this](entt::entity entity)
		{
			// Initialize last position with current position to avoid crazy velocities
			auto& node = m_registry.get<Nz::NodeComponent>(entity);
			auto& listener = m_registry.get<AudioListenerComponent>(entity);

			listener.lastPosition = node.GetGlobalPosition();
		});
		m_listenerObserver.SignalExisting();

		m_soundObserver.OnEntityAdded.Connect([this](entt::entity entity)
		{
			auto& node = m_registry.get<Nz::NodeComponent>(entity);
			auto& soundEmitter = m_registry.get<SoundEmitterComponent>(entity);
			soundEmitter.SetAudioSystem(this);
		});
	
		m_soundObserver.OnEntityRemove.Connect([this](entt::entity entity)
		{
			auto& soundEmitter = m_registry.get<SoundEmitterComponent>(entity);
			soundEmitter.StopAllSounds();
			soundEmitter.SetAudioSystem(nullptr);
		});

		m_soundObserver.SignalExisting();
	}

	AudioSystem::~AudioSystem()
	{
		for (entt::entity entity : m_soundObserver)
		{
			auto& soundEmitter = m_registry.get<SoundEmitterComponent>(entity);
			soundEmitter.StopAllSounds();
			soundEmitter.SetAudioSystem(nullptr);
		}
	}

	const std::shared_ptr<Nz::Sound>& AudioSystem::PlaySound(std::shared_ptr<Nz::SoundDataSource> soundSource)
	{
		std::shared_ptr<Nz::Sound> sound = std::make_shared<Nz::Sound>(Nz::Sound::Config{ m_audioEngine.get(), std::move(soundSource)});
		sound->EnableSpatialization(true);
		sound->SetMaxDistance(100.0f);

		m_oneShotSounds.push_back(std::move(sound));

		return m_oneShotSounds.back();
	}

	void AudioSystem::Update(Nz::Time elapsedTime)
	{
		float deltaTime = elapsedTime.AsSeconds();

		// Update listener
		auto listenerView = m_registry.view<Nz::NodeComponent, AudioListenerComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [listenerEntity, listenerNode, listenerComponent] : listenerView.each())
		{
			// Velocity is computed by position difference (TODO: Handle origin change)
			Nz::Vector3f position = listenerNode.GetGlobalPosition();
			Nz::Quaternionf rotation = listenerNode.GetGlobalRotation();

			Nz::Vector3f velocity = (position - listenerComponent.lastPosition) / deltaTime;
			listenerComponent.lastPosition = position;

			m_audioEngine->SetListenerPosition(0, position);
			m_audioEngine->SetListenerRotation(0, rotation);
			m_audioEngine->SetListenerVelocity(0, velocity);
		}

		// Update one-shot sounds
		for (auto it = m_oneShotSounds.begin(); it != m_oneShotSounds.end();)
		{
			std::shared_ptr<Nz::Sound>& sound = *it;
			if (!sound->IsPlaying() && sound->GetPlayingFrame() == 0)
			{
				it = m_oneShotSounds.erase(it);
				continue;
			}

			++it;
		}

		// Update sound emitting entities
		auto soundView = m_registry.view<Nz::NodeComponent, SoundEmitterComponent>(entt::exclude<Nz::DisabledComponent>);
		for (auto&& [soundEntity, soundNode, soundEmitter] : soundView.each())
			soundEmitter.Update(soundNode, deltaTime);
	}
}

