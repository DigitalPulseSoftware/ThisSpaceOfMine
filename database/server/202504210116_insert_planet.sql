INSERT INTO "planets" (id, generator, seed, chunk_count_x, chunk_count_y, chunk_count_z, corner_radius, gravity)
SELECT 1,
       'alice',
       42,
       5,
       5,
       5,
       16,
       9.8
WHERE NOT EXISTS (SELECT 1
                  FROM planets
                  WHERE id = 1);
