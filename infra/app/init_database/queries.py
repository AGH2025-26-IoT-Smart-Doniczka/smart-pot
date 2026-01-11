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
        salt TEXT NOT NULL,
        username TEXT NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS connections (
        user_id UUID NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
        pot_id UUID UNIQUE NOT NULL DEFAULT uuid_generate_v1(),
        active BOOLEAN DEFAULT TRUE,
        PRIMARY KEY (user_id, pot_id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS measures (
        pot_id UUID NOT NULL REFERENCES connections(pot_id) ON DELETE CASCADE,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        air_temp REAL,
        air_pressure INTEGER,
        soil_moisture REAL,
        illuminance INTEGER,
        PRIMARY KEY (pot_id, timestamp)
    )
    """,
]
