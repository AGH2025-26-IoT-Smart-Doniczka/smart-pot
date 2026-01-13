from psycopg2 import IntegrityError
from psycopg2.extras import RealDictCursor

from ..db.client import get_connection


def pot_exists(pot_id: str) -> bool:
    try:
        conn = get_connection()
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT 1
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,)
                )
                row = cur.fetchone()
                return row is not None

    finally:
        conn.close()


def insert_pot(pot_id: str) -> dict | None:
    try:
        conn = get_connection()
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    INSERT INTO pots (pot_id)
                    VALUES (%s)
                    RETURNING *;
                    """,
                    (pot_id,)
                )
                inserted_row = cur.fetchone()
                print(f"[insert_pot] Inserted new pot with id={pot_id}")
                return inserted_row

    except IntegrityError as e:
        raise e

    finally:
        conn.close()


def watering_update(pot_id: str, is_watering: bool) -> dict | None:
    try:
        conn = get_connection()
        if not pot_exists(pot_id):
            insert_pot(pot_id)
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    UPDATE pots
                    SET is_watering = %s
                    WHERE pot_id = %s
                    RETURNING *;
                    """,
                    (is_watering, pot_id)
                )
                updated_row = cur.fetchone()
                return updated_row

    except IntegrityError as e:
        raise e

    finally:
        conn.close()


def get_watering_status(pot_id: str) -> bool:
    try:
        conn = get_connection()
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT is_watering
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,)
                )
                row = cur.fetchone()
                if row is None:
                    raise ValueError(f"Pot with id {pot_id} not found")
                return row["is_watering"]

    finally:
        conn.close()


def telemetry_insert(pot_id: str, timestamp: float, air_temp: float, air_pressure: float, soil_moisture: int, illuminance: int) -> dict | None:
    try:
        conn = get_connection()
        if not pot_exists(pot_id):
            insert_pot(pot_id)
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    INSERT INTO telemetry (pot_id, timestamp, air_temp, air_pressure, soil_moisture, illuminance)
                    VALUES (%s, %s, %s, %s, %s, %s)
                    RETURNING *;
                    """,
                    (pot_id, timestamp, air_temp, air_pressure, soil_moisture, illuminance)
                )
                inserted_row = cur.fetchone()
                return inserted_row

    except IntegrityError as e:
        raise e

    finally:
        conn.close()