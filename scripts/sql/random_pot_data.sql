INSERT INTO measures (
    pot_id,
    timestamp,
    air_temp,
    air_pressure,
    soil_moisture,
    illuminance
  )
  SELECT
    'DEF' AS pot_id,
    (date_trunc('day', now()) - (n || ' days')::interval) + interval '12 hours' AS timestamp,
    round((20 + random() * 6)::numeric, 1) AS air_temp,           -- 20.0–26.0 °C
    (1000 + random() * 20)::int AS air_pressure,                  -- 1000–1020 hPa
    (30 + random() * 35)::int AS soil_moisture,                    -- 30–65 %
    (3000 + random() * 9000)::int AS illuminance                   -- 3000–12000 lx
  FROM generate_series(0, 29) AS n;