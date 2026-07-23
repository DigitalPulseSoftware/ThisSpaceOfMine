CREATE TABLE "planet_generator_cache" (
	"generator_name"     TEXT NOT NULL,
	"generator_hash"     TEXT NOT NULL,
	"chunk_x"            INTEGER NOT NULL,
	"chunk_y"            INTEGER NOT NULL,
	"chunk_z"            INTEGER NOT NULL,
	"chunk_data_version" INTEGER NOT NULL,
	"chunk_data"         BLOB NOT NULL,
	PRIMARY KEY("generator_name","chunk_x","chunk_y","chunk_z")
) STRICT;

