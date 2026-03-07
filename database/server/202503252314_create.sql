CREATE TABLE "migration" (
	"name"	TEXT NOT NULL,
	PRIMARY KEY("name")
) WITHOUT ROWID,STRICT;

CREATE TABLE "planets" (
	"id"	INTEGER NOT NULL,
	"generator"	TEXT NOT NULL,
	"seed"	INTEGER NOT NULL,
	"chunk_count_x"	INTEGER NOT NULL CHECK("chunk_count_x" > 0),
	"chunk_count_y"	INTEGER NOT NULL CHECK("chunk_count_y" > 0),
	"chunk_count_z"	INTEGER NOT NULL CHECK("chunk_count_z" > 0),
	"corner_radius"	REAL NOT NULL DEFAULT 0 CHECK("corner_radius" >= 0),
	"gravity"	REAL NOT NULL,
	PRIMARY KEY("id")
) STRICT;

CREATE TABLE "planet_chunks" (
	"planet_id"	INTEGER NOT NULL,
	"position_x"	INTEGER NOT NULL,
	"position_y"	INTEGER NOT NULL,
	"position_z"	INTEGER NOT NULL,
	"version"	INTEGER NOT NULL,
	"chunk_data"	BLOB NOT NULL,
	"last_update"   TEXT NOT NULL,
	PRIMARY KEY("position_x","planet_id","position_y","position_z"),
	FOREIGN KEY("planet_id") REFERENCES "planets"("id") ON DELETE CASCADE
) WITHOUT ROWID,STRICT;

CREATE TABLE "planet_links" (
	"source_planet_id"	INTEGER NOT NULL,
	"destination_planet_id"	INTEGER NOT NULL,
	"position_x"	REAL NOT NULL,
	"position_y"	REAL NOT NULL,
	"position_z"	REAL NOT NULL,
	PRIMARY KEY("source_planet_id","destination_planet_id"),
	FOREIGN KEY("destination_planet_id") REFERENCES "planets"("id") ON DELETE CASCADE,
	FOREIGN KEY("source_planet_id") REFERENCES "planets"("id") ON DELETE CASCADE
) WITHOUT ROWID,STRICT;
