// Copyright (C) 2026 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "This Space Of Mine" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <ClientLib/ClientAssetCookRegistry.hpp>
#include <Nazara/Core/File.hpp>
#include <nlohmann/json.hpp>

namespace nlohmann
{
	template<typename BasicJsonType>
	void from_json(const BasicJsonType& j, Nz::Color& color)
	{
		j.at("r").get_to(color.r);
		j.at("g").get_to(color.g);
		j.at("b").get_to(color.b);
		color.a = j.value("a", 1.0f);
	}

	NLOHMANN_JSON_SERIALIZE_ENUM(tsom::ClientAssetCookRegistry::TextureType, {
		{tsom::ClientAssetCookRegistry::TextureType::None, "none"},
		{tsom::ClientAssetCookRegistry::TextureType::BC1, "bc1"},
		{tsom::ClientAssetCookRegistry::TextureType::BC3, "bc3"},
		{tsom::ClientAssetCookRegistry::TextureType::BC4, "bc4"},
		{tsom::ClientAssetCookRegistry::TextureType::BC5, "bc5"},
	})

	template<typename BasicJsonType>
	void to_json(BasicJsonType& j, const Nz::Color& color)
	{
		j = BasicJsonType{
			{"r", color.r},
			{"g", color.g},
			{"b", color.b}
		};

		if (!Nz::NumberEquals(color.a, 1.0f))
			j["a"] = color.a;
	}

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(tsom::ClientAssetCookRegistry::Texture, path, type)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(tsom::ClientAssetCookRegistry::BlockEntry,
		ambientOcclusionFallback, ambientOcclusionHeightTexture,
		baseColorFallback, baseColorTexture,
		metalnessFallback, normalMapTexture,
		roughnessFallback, roughnessMetalnessTexture
	)
}

namespace tsom
{
	void ClientAssetCookRegistry::AddBlock(std::string blockName, BlockEntry blockEntry)
	{
		NazaraAssertMsg(!m_blockEntries.contains(blockName), "block %s is already present", blockName.c_str());
		m_blockEntries.emplace(std::move(blockName), std::move(blockEntry));
	}

	auto ClientAssetCookRegistry::GetBlock(std::string_view blockName) const -> const BlockEntry&
	{
		return Nz::Retrieve(m_blockEntries, blockName);
	}

	bool ClientAssetCookRegistry::SaveToFile(const std::filesystem::path& path) const
	{
		nlohmann::ordered_json blockEntries;
		for (const auto& [blockName, blockEntry] : m_blockEntries)
			blockEntries[blockName] = blockEntry;

		nlohmann::ordered_json doc;
		doc["blocks"] = std::move(blockEntries);

		std::string content = doc.dump(1, '\t');

		return Nz::File::WriteWhole(path, content.data(), content.size());
	}

	std::optional<ClientAssetCookRegistry> ClientAssetCookRegistry::LoadFromContent(std::string_view content)
	{
		nlohmann::ordered_json doc = nlohmann::ordered_json::parse(content);

		ClientAssetCookRegistry cookRegistry;
		for (const auto& [blockName, blockEntryDoc] : doc["blocks"].items())
			cookRegistry.AddBlock(blockName, blockEntryDoc);

		return std::move(cookRegistry);
	}

	std::optional<ClientAssetCookRegistry> ClientAssetCookRegistry::LoadFromFile(const std::filesystem::path& path)
	{
		std::optional<std::vector<Nz::UInt8>> contentOpt = Nz::File::ReadWhole(path);
		if (!contentOpt)
			return std::nullopt;

		return LoadFromContent(std::string_view(reinterpret_cast<const char*>(contentOpt->data()), contentOpt->size()));
	}
}
