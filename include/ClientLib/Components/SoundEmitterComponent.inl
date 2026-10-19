// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline SoundEmitterComponent::SoundEmitterComponent() :
	m_audioSystem(nullptr)
	{
	}

	inline void SoundEmitterComponent::SetAudioSystem(AudioSystem* audioSystem)
	{
		m_audioSystem = audioSystem;
	}

	inline void SoundEmitterComponent::SetLastPosition(const Nz::Vector3f& position)
	{
		m_lastPosition = position;
	}
}
