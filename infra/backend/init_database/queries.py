queries = [
    """
    CREATE EXTENSION IF NOT EXISTS "pgcrypto";
    """,
    """
    CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
    """,
    """
    CREATE TABLE IF NOT EXISTS users (
        user_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
        email TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        username TEXT NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS pots (
        pot_id TEXT PRIMARY KEY,
        measure_interval_sec INTEGER DEFAULT 300,
        max_temperature NUMERIC(4,1) DEFAULT 30.0,
        min_temperature NUMERIC(4,1) DEFAULT 10.0,
        humidity_thresholds JSONB DEFAULT '{
                "very_low": 10,
                "low": 35,
                "high": 60,
                "very_high": 90
            }'::jsonb,
        illuminance_type INTEGER DEFAULT 1,
        is_watering BOOLEAN DEFAULT FALSE
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS connections (
        user_id UUID NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
        pot_id TEXT NOT NULL REFERENCES pots(pot_id) ON DELETE CASCADE,
        is_active BOOLEAN DEFAULT TRUE,
        is_admin BOOLEAN DEFAULT TRUE,
        is_owner BOOLEAN DEFAULT TRUE,
        PRIMARY KEY (user_id, pot_id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS measures (
        pot_id TEXT NOT NULL REFERENCES pots(pot_id) ON DELETE CASCADE,
        timestamp TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
        air_temp NUMERIC(4,1),
        air_pressure INTEGER,
        soil_moisture NUMERIC(4,1),
        illuminance INTEGER,
        PRIMARY KEY (pot_id, timestamp)
    )
    """,
    """
    CREATE UNIQUE INDEX IF NOT EXISTS one_owner_per_pot
    ON connections (pot_id)
    WHERE is_owner = TRUE;
    """,
]
