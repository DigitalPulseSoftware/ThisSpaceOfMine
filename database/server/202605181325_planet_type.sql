ALTER TABLE "planets" ADD COLUMN "type" TEXT NOT NULL DEFAULT "round_cube_planet";
ALTER TABLE "planets" ADD COLUMN "properties" TEXT NOT NULL DEFAULT '{}' CHECK(json_valid("properties"));

UPDATE "planets" SET properties = json_object('CornerRadius', corner_radius, 'Gravity', gravity, 'Seed', seed);

ALTER TABLE "planets" DROP COLUMN "corner_radius";
ALTER TABLE "planets" DROP COLUMN "gravity";
ALTER TABLE "planets" DROP COLUMN "seed";
