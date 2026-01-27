queries = [
    """
    CREATE EXTENSION IF NOT EXISTS "pgcrypto";
    """,
    """
    CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
    """,
    """
    CREATE TABLE IF NOT EXISTS users (
        user_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
        email TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        username TEXT NOT NULL
    )
    """,
    """
    DO $$
    BEGIN
        IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'connection_role') THEN
            CREATE TYPE connection_role AS ENUM ('OWNER', 'EDITOR', 'VIEWER');
        END IF;
    END $$;
    """,
    """
    CREATE TABLE IF NOT EXISTS pots (
        pot_id TEXT PRIMARY KEY,
        pot_name TEXT,
        measure_interval_sec INTEGER NOT NULL DEFAULT 300,
        send_interval_sec INTEGER NOT NULL DEFAULT 300,
        watering_interval_sec INTEGER,
        watering_duration_sec INTEGER,
        min_temperature NUMERIC(4,1) DEFAULT 10.0,
        max_temperature NUMERIC(4,1) DEFAULT 30.0,
        min_moisture INTEGER DEFAULT 0,
        max_moisture INTEGER DEFAULT 100,
        illuminance_type INTEGER DEFAULT 1
    )
    """,
    """
    ALTER TABLE pots
    ADD COLUMN IF NOT EXISTS mqtt_password TEXT;
    """,
    """
    ALTER TABLE pots
    ADD COLUMN IF NOT EXISTS watering_duration_sec INTEGER;
    """,
    """
    CREATE TABLE IF NOT EXISTS connections (
        user_id UUID NOT NULL REFERENCES users(user_id),
        pot_id TEXT NOT NULL REFERENCES pots(pot_id),
        role connection_role DEFAULT 'VIEWER',
        PRIMARY KEY (user_id, pot_id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS measures (
        pot_id TEXT NOT NULL REFERENCES pots(pot_id),
        timestamp TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
        air_temp REAL,
        air_pressure INTEGER,
        soil_moisture INTEGER,
        illuminance INTEGER,
        PRIMARY KEY (pot_id, timestamp)
    )
    """,
    """
    CREATE UNIQUE INDEX IF NOT EXISTS one_owner_per_pot
    ON connections (pot_id)
    WHERE role = 'OWNER';
    """,
    """
    CREATE TABLE IF NOT EXISTS pot_logs (
        id SERIAL PRIMARY KEY,
        pot_id TEXT NOT NULL REFERENCES pots(pot_id),
		timestamp TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
        label TEXT,
        payload JSONB
    )
    """,
]
