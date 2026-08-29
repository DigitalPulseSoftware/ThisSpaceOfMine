ALTER TABLE "planet_entities" ADD COLUMN "connections" TEXT NOT NULL DEFAULT '{}' CHECK(json_valid("connections"));
