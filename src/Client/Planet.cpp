// Copyright (C) 2023 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Client/Planet.hpp>
#include <Nazara/Graphics/GraphicalMesh.hpp>
#include <Nazara/JoltPhysics3D/JoltCollider3D.hpp>
#include <fmt/format.h>

namespace fixme
{
	float sdRoundBox(const Nz::Vector3f& pos, const Nz::Vector3f& halfDims, float cornerRadius)
	{
		Nz::Vector3f edgeDistance = pos.GetAbs() - halfDims + Nz::Vector3f(cornerRadius);
		float outsideDistance = edgeDistance.Maximize(Nz::Vector3f::Zero()).GetLength();
		float insideDistance = std::min(std::max({ edgeDistance.x, edgeDistance.y, edgeDistance.z }), 0.f);
		return outsideDistance + insideDistance - cornerRadius;
	}

	Nz::Vector3f calcNormal(Nz::Vector3f p, Nz::Vector3f b, float r)
	{
		const float epsilon = 0.0001; // Petite valeur pour l'échantillonnage de distance

		float d = sdRoundBox(p, b, r); // Calcule la distance signée à la boîte arrondie

		// Calcule la normale en utilisant des différences finies
		Nz::Vector3f n = Nz::Vector3f(
			sdRoundBox(p + Nz::Vector3f(epsilon, 0.0, 0.0), b, r) - d,
			sdRoundBox(p + Nz::Vector3f(0.0, epsilon, 0.0), b, r) - d,
			sdRoundBox(p + Nz::Vector3f(0.0, 0.0, epsilon), b, r) - d
		);

		return Nz::Vector3f::Normalize(n); // Normalise le vecteur pour obtenir une normale unitaire
	}
}

namespace tsom
{
	Planet::Planet(std::size_t gridDims, float tileSize, float cornerRadius) :
	m_gridDimensions(gridDims),
	m_cornerRadius(cornerRadius),
	m_tileSize(tileSize)
	{
		RebuildGrid();
	}
	
	std::shared_ptr<Nz::JoltCollider3D> Planet::BuildCollider()
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(indices, vertices);

#if 0
		return std::make_shared<Nz::JoltConvexHullCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size());
#else
		return std::make_shared<Nz::JoltMeshCollider3D>(Nz::SparsePtr<Nz::Vector3f>(&vertices[0].position, sizeof(vertices[0])), vertices.size(), indices.data(), indices.size());
#endif
	}
	
	std::shared_ptr<Nz::GraphicalMesh> Planet::BuildGfxMesh()
	{
		std::vector<Nz::UInt32> indices;
		std::vector<Nz::VertexStruct_XYZ_Color_UV> vertices;
		BuildMesh(indices, vertices);

		std::shared_ptr<Nz::IndexBuffer> indexBuffer = std::make_shared<Nz::IndexBuffer>(Nz::IndexType::U32, indices.size(), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, indices.data());
		std::shared_ptr<Nz::VertexBuffer> vertexBuffer = std::make_shared<Nz::VertexBuffer>(Nz::VertexDeclaration::Get(Nz::VertexLayout::XYZ_Color_UV), vertices.size(), Nz::BufferUsage::Read, Nz::SoftwareBufferFactory, vertices.data());

		std::shared_ptr<Nz::StaticMesh> staticMesh = std::make_shared<Nz::StaticMesh>(std::move(vertexBuffer), std::move(indexBuffer));
		staticMesh->GenerateAABB();
		staticMesh->SetPrimitiveMode(Nz::PrimitiveMode::LineList);

		return Nz::GraphicalMesh::BuildFromMesh(*Nz::Mesh::Build(staticMesh));
	}

	void Planet::BuildMesh(std::vector<Nz::UInt32>& indices, std::vector<Nz::VertexStruct_XYZ_Color_UV>& vertices)
	{
		Nz::Matrix4f transform;

		auto BuildFace = [&](std::size_t gridVec, const Nz::Color& color, const Nz::Vector3f& translation, const Nz::Quaternionf& rotation)
		{
			for (std::size_t i = 0; i < m_grids[gridVec].size(); ++i)
			{
				float offset = i * m_tileSize;
				Nz::Matrix4f transform = Nz::Matrix4f::Transform(translation + rotation * Nz::Vector3f{ offset, -offset, offset }, rotation);
				m_grids[gridVec][i]->BuildMesh(transform, m_tileSize, color, indices, vertices);
			}
		};

		float gridSize = m_gridDimensions * m_tileSize;

		// Up
		BuildFace(0, Nz::Color::Green(), Nz::Vector3f::Zero(), Nz::Quaternionf::Identity());

		// Down
		BuildFace(1, Nz::Color::Magenta(), Nz::Vector3f(gridSize, -gridSize, 0.f), Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Down()));

		// Front
		BuildFace(2, Nz::Color::Blue(), Nz::Vector3f(0.f, -gridSize, 0.f), Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Forward()));

		// Back
		BuildFace(3, Nz::Color::White(), Nz::Vector3f(0.f, 0.f, gridSize), Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Backward()));

		// Left
		//BuildFace(4, Nz::Color::Red(), Nz::Vector3f(0.f, -gridSize, 0.f), Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Left()));

		// Right
		BuildFace(5, Nz::Color::Yellow(), Nz::Vector3f(gridSize, 0.f, 0.f), Nz::Quaternionf::RotationBetween(Nz::Vector3f::Up(), Nz::Vector3f::Right()));

		float minDist = 1000.f;
		float maxDist = -1.f;

		Nz::Boxf planetBox(Nz::Vector3f(0.f, -gridSize, 0.f), Nz::Vector3f(gridSize));
		for (Nz::VertexStruct_XYZ_Color_UV& vert : vertices)
		{
			Nz::Vector3f center = planetBox.GetCenter();

			float distToCenter = std::max({
				std::abs(vert.position.x - center.x),
				std::abs(vert.position.y - center.y),
				std::abs(vert.position.z - center.z),
			});

			vert.color *= Nz::Color(std::clamp(distToCenter / gridSize * 2.f, 0.f, 1.f));

#if 1
			//fmt::print("{}\n", distToCenter);
			//Nz::Boxf box(center - Nz::Vector3f(distToCenter, distToCenter, distToCenter), Nz::Vector3f(distToCenter * 2.f));
			float dist = fixme::sdRoundBox(vert.position - center, Nz::Vector3f(distToCenter), m_cornerRadius);
			//fmt::print("{}\n", dist);

			Nz::Vector3f normal = fixme::calcNormal(vert.position - center, Nz::Vector3f(distToCenter), m_cornerRadius);
			vert.position -= normal * dist;
			vert.position -= center;

			float dist2 = vert.position.GetLength();
			minDist = std::min(minDist, dist2);
			maxDist = std::max(maxDist, dist2);

#else
			float innerReductionSize = std::max(distToCenter - m_cornerRadius, 0.f);
			Nz::Boxf innerBox(center - Nz::Vector3f(innerReductionSize, innerReductionSize, innerReductionSize), Nz::Vector3f(innerReductionSize * 2.f));

			Nz::Vector3f innerPos;
			innerPos.x = std::clamp(vert.position.x, innerBox.GetMinimum().x, innerBox.GetMaximum().x);
			innerPos.y = std::clamp(vert.position.y, innerBox.GetMinimum().y, innerBox.GetMaximum().y);
			innerPos.z = std::clamp(vert.position.z, innerBox.GetMinimum().z, innerBox.GetMaximum().z);

			Nz::Vector3f normal = Nz::Vector3f::Normalize(vert.position - innerPos);

			Nz::Vector3f originalPos = vert.position;

			vert.position = innerPos + normal * std::min(m_cornerRadius, distToCenter);
			vert.position -= center;

			// Try reversing it
			//fmt::print("{}\n", fixme::sdRoundBox(vert.position, Nz::Vector3f(gridSize * 0.5f), m_cornerRadius));
#endif
		}

		fmt::print("minDist = {}, maxDist = {}\n", minDist, maxDist);

		/*for (Nz::VertexStruct_XYZ_Color_UV& vert : vertices)
		{
			// En gros, à partir de mon point, je calcule la normale vers le centre, et je vais vers le centre d'une distance de corner size (que je peux fixer sans min), ça me permet de faire une box interne à la planète dans laquelle je clamp ma position, et je fais la distance par rapport à mon point d'origine. Cette distance + la taille de la box / tilesize c'est mon niveau de hauteur

			Nz::Vector3f normalToCenter = Nz::Vector3f::Normalize(-vert.position);
			Nz::Vector3f pos = vert.position + normalToCenter * m_cornerRadius;
			float distToCenter = std::max({
				std::abs(pos.x),
				std::abs(pos.y),
				std::abs(pos.z),
			});

			Nz::Boxf innerBox(-Nz::Vector3f(distToCenter), Nz::Vector3f(distToCenter * 2.f));
			
			Nz::Vector3f innerPos;
			innerPos.x = std::clamp(vert.position.x, innerBox.GetMinimum().x, innerBox.GetMaximum().x);
			innerPos.y = std::clamp(vert.position.y, innerBox.GetMinimum().y, innerBox.GetMaximum().y);
			innerPos.z = std::clamp(vert.position.z, innerBox.GetMinimum().z, innerBox.GetMaximum().z);

			//float dist = fixme::sdRoundBox(vert.position, Nz::Vector3f(10.f), m_cornerRadius);
			fmt::print("{}\n", innerPos.Distance(vert.position));
		}*/
	}

	void Planet::RebuildGrid()
	{
		for (auto& gridVec : m_grids)
		{
			gridVec.clear();

			std::size_t gridSize = m_gridDimensions;
			while (gridSize > 0)
			{
				gridVec.emplace_back(std::make_unique<VoxelGrid>(gridSize, gridSize));
				//if (gridSize <= 3)
					break;

				gridSize -= 2;
			}
		}

		// Attach neighbors
		for (auto& gridVec : m_grids)
		{
			for (std::size_t i = 0; i < gridVec.size(); ++i)
			{
				if (i > 0)
					gridVec[i]->AttachNeighborGrid(Direction::Up, { 1, 1 }, gridVec[i - 1].get());

				if (i < gridVec.size() - 1)
					gridVec[i]->AttachNeighborGrid(Direction::Down, { -1, -1 }, gridVec[i + 1].get());
			}
		}
	}
}
