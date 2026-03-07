CREATE TABLE "planet_entities" (
	"id"	INTEGER NOT NULL,
	"unique_id"	TEXT NOT NULL,
	"planet_id"	INTEGER NOT NULL,
	"class_name"	TEXT NOT NULL,
	"class_version"	INTEGER NOT NULL,
	"position_x"	REAL NOT NULL,
	"position_y"	REAL NOT NULL,
	"position_z"	REAL NOT NULL,
	"rotation_x"	REAL NOT NULL,
	"rotation_y"	REAL NOT NULL,
	"rotation_z"	REAL NOT NULL,
	"rotation_w"	REAL NOT NULL,
	"properties"	TEXT NOT NULL DEFAULT '{}' CHECK(json_valid("properties")),
	"last_update"	TEXT NOT NULL,
	PRIMARY KEY("id"),
    UNIQUE("unique_id"),
	FOREIGN KEY("planet_id") REFERENCES "planets"("id") ON DELETE CASCADE
) STRICT