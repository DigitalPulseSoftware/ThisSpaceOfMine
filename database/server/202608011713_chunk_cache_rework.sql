DROP TABLE "planet_generator_cache";
ALTER TABLE "planet_chunks" ADD COLUMN "cache_state" INTEGER NOT NULL DEFAULT 0;
