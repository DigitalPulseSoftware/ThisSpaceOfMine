// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#ifndef TSOM_CLIENTLIB_COMPONENTS_SOUNDEMITTERCOMPONENT_HPP
#define TSOM_CLIENTLIB_COMPONENTS_SOUNDEMITTERCOMPONENT_HPP

#include <ClientLib/Export.hpp>
#include <Nazara/Audio/Sound.hpp>
#include <NazaraUtils/FixedVector.hpp>
#include <memory>

namespace Nz
{
	class Node;
	class SoundDataSource;
}

namespace tsom
{
	class TSOM_CLIENTLIB_API SoundEmitterComponent
	{
		friend class AudioSystem;

		public:
			inline SoundEmitterComponent();

			const std::shared_ptr<Nz::Sound>& AddSound(std::shared_ptr<Nz::SoundDataSource> soundSource);

			void StopAllSounds();

		private:
			inline void SetAudioSystem(AudioSystem* audioSystem);
			inline void SetLastPosition(const Nz::Vector3f& position);
			void Update(Nz::Node& node, float deltaTime);

			Nz::HybridVector<std::shared_ptr<Nz::Sound>, 2> m_sounds;
			Nz::Vector3f m_lastPosition;
			AudioSystem* m_audioSystem;
	};
}

#include <ClientLib/Components/SoundEmitterComponent.inl>

#endif // TSOM_CLIENTLIB_COMPONENTS_SOUNDEMITTERCOMPONENT_HPP
