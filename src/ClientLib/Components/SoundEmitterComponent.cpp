// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Components/SoundEmitterComponent.hpp>
#include <ClientLib/Systems/AudioSystem.hpp>
#include <Nazara/Core/Node.hpp>

#pragma optimize("", off)

namespace tsom
{
	const std::shared_ptr<Nz::Sound>& SoundEmitterComponent::AddSound(std::shared_ptr<Nz::SoundDataSource> soundSource)
	{
		NazaraAssertMsg(m_audioSystem, "no audio system");

		std::shared_ptr<Nz::Sound> sound = std::make_shared<Nz::Sound>(Nz::Sound::Config { &m_audioSystem->GetAudioEngine(), std::move(soundSource) });
		sound->EnableSpatialization(true);
		sound->SetMaxDistance(100.0f);

		m_sounds.push_back(std::move(sound));

		return m_sounds.back();
	}

	void SoundEmitterComponent::StopAllSounds()
	{
		for (const auto& soundPtr : m_sounds)
			soundPtr->Stop();

		m_sounds.clear();
	}

	void SoundEmitterComponent::Update(Nz::Node& node, float deltaTime)
	{
		// Velocity is computed by position difference (TODO: Handle origin change)
		Nz::Vector3f position = node.GetGlobalPosition();
		Nz::Vector3f direction = node.GetForward();
		Nz::Vector3f velocity = (position - m_lastPosition) / deltaTime;
		m_lastPosition = position;

		for (auto it = m_sounds.begin(); it != m_sounds.end();)
		{
			std::shared_ptr<Nz::Sound>& sound = *it;
			if (!sound->IsPlaying() && sound->GetPlayingFrame() == 0)
			{
				// Sound has stopped and reset
				it = m_sounds.erase(it);
				continue;
			}

			//sound->SetDirection(direction);
			sound->SetPosition(position);
			sound->SetVelocity(velocity);
			++it;
		}
	}
}

#pragma optimize("", on)
