// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/Systems/SoundPoolSystem.hpp>
#include <optional>

namespace tsom
{
	void SoundPoolSystem::PlaySound(std::shared_ptr<Nz::SoundBuffer> soundBuffer, const Nz::Vector3f& position, const SoundParameters& params)
	{
		std::optional<Nz::Sound> sound;
		if (!m_soundPool.empty())
		{
			sound = std::move(m_soundPool.back());
			m_soundPool.pop_back();
		}
		else
			sound.emplace();

		sound->EnableSpatialization(true);
		sound->SetBuffer(std::move(soundBuffer));
		sound->SetPitch(params.pitch);
		sound->SetPosition(position);
		sound->SetVolume(params.volume);

		sound->SeekToSampleOffset(0);
		sound->Play();

		m_playingSounds.push_back(PlayingSound{
			.sound = std::move(*sound)
		});
	}

	void SoundPoolSystem::Update(Nz::Time /*elapsedTime*/)
	{
		for (auto it = m_playingSounds.begin(); it != m_playingSounds.end();)
		{
			auto& playingSound = *it;
			if (playingSound.sound.IsPlaying())
			{
				++it;
				continue;
			}

			// Sound has finished, put it back in the pool
			m_soundPool.push_back(std::move(playingSound.sound));
			it = m_playingSounds.erase(it);
		}
	}
}
