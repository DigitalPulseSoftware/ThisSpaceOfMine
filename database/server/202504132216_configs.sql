CREATE TABLE "configs" (
	"name"	TEXT NOT NULL,
	"value"	TEXT NOT NULL CHECK(json_valid("value")),
	PRIMARY KEY("name")
) WITHOUT ROWID,STRICT;

INSERT INTO configs(name, value) VALUES("default_spawnpoint", json_object('planet_id', 1, 'position', json_object('x', 0.0, 'y', 100.0, 'z', 5.0)));
