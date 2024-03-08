#include <CommonLib/Chunk.hpp>
#include <CommonLib/ChunkContainer.hpp>
#include <CommonLib/Planet.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace tsom;

TEST_CASE("Block positions", "[Chunks]")
{
	constexpr int ChunkSize = int(Planet::ChunkSize);
	constexpr int HalfChunkSize = ChunkSize / 2;

	Planet planet(1.f, 0.f, 9.81f);

	SECTION("Testing block indices")
	{
		CHECK(planet.GetBlockIndices({  0, 0, 0 },  { 0, 0, 0 }) == BlockIndices(-HalfChunkSize,     -HalfChunkSize,     -HalfChunkSize));
		CHECK(planet.GetBlockIndices({ -1, 0, 0 },  { 0, 0, 0 }) == BlockIndices(-HalfChunkSize * 3, -HalfChunkSize,     -HalfChunkSize));
		CHECK(planet.GetBlockIndices({  1, 0, 0 },  { 0, 0, 0 }) == BlockIndices( HalfChunkSize,     -HalfChunkSize,     -HalfChunkSize));
		CHECK(planet.GetBlockIndices({  0, -1, 0 }, { 0, 0, 0 }) == BlockIndices(-HalfChunkSize,     -HalfChunkSize * 3, -HalfChunkSize));
		CHECK(planet.GetBlockIndices({  0, 1, 0 },  { 0, 0, 0 }) == BlockIndices(-HalfChunkSize,      HalfChunkSize,     -HalfChunkSize));
		CHECK(planet.GetBlockIndices({  0, 0, -1 }, { 0, 0, 0 }) == BlockIndices(-HalfChunkSize,     -HalfChunkSize,     -HalfChunkSize * 3));
		CHECK(planet.GetBlockIndices({  0, 0, 1 },  { 0, 0, 0 }) == BlockIndices(-HalfChunkSize,     -HalfChunkSize,      HalfChunkSize));

		SECTION("Converting back and forth")
		{
			auto Test = [&](const ChunkIndices& chunkIndices, const Nz::Vector3ui& blockIndices)
			{
				BlockIndices globalIndices = planet.GetBlockIndices(chunkIndices, blockIndices);

				Nz::Vector3ui localIndices;
				INFO("Chunk indices: " << chunkIndices << ", block indices: " << blockIndices);
				CHECK(planet.GetChunkIndicesByBlockIndices(globalIndices, &localIndices) == chunkIndices);
				CHECK(localIndices == blockIndices);
			};

			Test({ 1, 0, 0 }, { 1, 0, 0 });
			Test({ -1, 0, 0 }, { 0, 0, 0 });
			Test({ -1, 0, 0 }, { 0, 0, 1 });
			Test({ -1, 0, 0 }, { 0, 1, 0 });
			Test({ -1, 42, 2 }, { 14, 10, 12 });
			Test({ 3, 42, -2 }, { 17, 10, Planet::ChunkSize - 1 });
			Test({ 3, 42, -2 }, { Planet::ChunkSize - 1, Planet::ChunkSize - 1, Planet::ChunkSize - 1 });
		}
	}
}
