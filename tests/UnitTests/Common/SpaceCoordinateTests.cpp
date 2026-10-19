#include <CommonLib/Utility/SpaceTransformations.hpp>
#include <Nazara/Core/ApplicationBase.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace tsom;

TEST_CASE("Space transformations", "[SpaceTransformations]")
{
	SECTION("Testing space transformations")
	{
		auto TestLocalTransformation = [](const Nz::Vector3d& globalCoordinates, const Nz::Vector3i32& expectedCellCoordinates, const Nz::Vector3f& expectedLocalCoordinates)
		{
			auto [cellCoordinates, localCoordinates] = tsom::SpaceTransformations::ToLocalCoordinates(globalCoordinates);
			INFO("When testing with global coordinates " << globalCoordinates);
			CHECK(cellCoordinates == expectedCellCoordinates);
			CHECK(localCoordinates == expectedLocalCoordinates);

			CHECK(tsom::SpaceTransformations::ToGlobalCoordinates(cellCoordinates, localCoordinates) == globalCoordinates);
		};

		TestLocalTransformation({ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 });
		TestLocalTransformation({ 1000, 0, 0 }, { 0, 0, 0 }, { 1000, 0, 0 });
		TestLocalTransformation({ -1000, 0, 0 }, { 0, 0, 0 }, { -1000, 0, 0 });
		TestLocalTransformation({ 4097, 0, 0 }, { 1, 0, 0 }, { -4095, 0, 0 });
		TestLocalTransformation({ -4097, 0, 0 }, { -1, 0, 0 }, { 4095, 0, 0 });
		TestLocalTransformation({ 4097 + 8192, -4097, 0 }, { 2, -1, 0 }, { -4095, 4095, 0 });
		TestLocalTransformation({ -4097 - 8192, 4097, 0 }, { -2, 1, 0 }, { 4095, -4095, 0 });

		TestLocalTransformation({
			tsom::Constants::SpaceCellSize * 5.0 + 1.0,
			tsom::Constants::SpaceCellSize * -40.0 - 1.0,
			0.0 },
			{ 5, -40, 0 }, { 1, -1, 0 });

	}
}
