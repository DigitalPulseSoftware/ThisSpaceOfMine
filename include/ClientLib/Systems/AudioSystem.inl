// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

namespace tsom
{
	inline Nz::AudioEngine& AudioSystem::GetAudioEngine()
	{
		return *m_audioEngine;
	}

	inline const Nz::AudioEngine& AudioSystem::GetAudioEngine() const
	{
		return *m_audioEngine;
	}
}
