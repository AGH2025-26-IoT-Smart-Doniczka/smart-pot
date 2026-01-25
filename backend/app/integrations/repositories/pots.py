from datetime import datetime
from typing import Any

from psycopg2 import IntegrityError
from psycopg2.extras import RealDictCursor

from ..db.client import get_connection
from ...schemas.roles import ConnectionRole


def pot_exists(pot_id: str) -> bool:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT 1
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                row = cur.fetchone()
                return row is not None
    finally:
        conn.close()


def user_exists(user_id: str) -> bool:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT 1
                    FROM users
                    WHERE user_id = %s;
                    """,
                    (user_id,),
                )
                row = cur.fetchone()
                return row is not None
    finally:
        conn.close()


def pot_has_owner(pot_id: str) -> tuple[Any, ...] | None:
    with get_connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                    SELECT user_id
                    FROM connections
                    WHERE pot_id = %s
                    AND role = %s
                    LIMIT 1;
                    """,
                (pot_id, ConnectionRole.OWNER.value),
            )
            return cur.fetchone()


def get_pot_owner_username(pot_id: str) -> str | None:
    with get_connection() as conn:
        with conn.cursor(cursor_factory=RealDictCursor) as cur:
            cur.execute(
                """
                    SELECT u.username
                    FROM connections c
                    JOIN users u ON c.user_id = u.user_id
                    WHERE c.pot_id = %s
                      AND c.role = %s
                    LIMIT 1;
                    """,
                (pot_id, ConnectionRole.OWNER.value),
            )
            row = cur.fetchone()
            if row:
                return row["username"]
            return None


def insert_connection(pot_id: str, user_id: str, has_owner: bool) -> dict[str, Any] | None:
    if not user_exists(user_id):
        raise ValueError(f"User with id {user_id} does not exist")

    if not pot_exists(pot_id):
        insert_pot(pot_id)

    role = ConnectionRole.OWNER.value if not has_owner else ConnectionRole.VIEWER.value

    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    INSERT INTO connections (
                        pot_id, user_id, role
                    )
                    VALUES (%s, %s, %s)
                    ON CONFLICT DO NOTHING
                    RETURNING *;
                    """,
                    (pot_id, user_id, role),
                )
                return cur.fetchone()
    finally:
        conn.close()


def insert_pot(pot_id: str) -> dict[str, Any] | None:
    try:
        conn = get_connection()
        try:
            with conn:
                with conn.cursor(cursor_factory=RealDictCursor) as cur:
                    cur.execute(
                        """
                        INSERT INTO pots (pot_id, pot_name)
                        VALUES (%s, %s)
                        ON CONFLICT DO NOTHING
                        RETURNING *;
                        """,
                        (pot_id, pot_id),
                    )
                    inserted_row = cur.fetchone()
                    print(f"[insert_pot] Inserted new pot with id={pot_id}")
                    return inserted_row
        finally:
            conn.close()

    except IntegrityError as e:
        raise e


def measures_insert(
    pot_id: str,
    timestamp: float,
    air_temp: float,
    air_pressure: float,
    soil_moisture: int,
    illuminance: int,
) -> dict[str, Any] | None:
    try:
        if not pot_exists(pot_id):
            raise ValueError(f"Pot with id {pot_id} does not exist")
        conn = get_connection()
        try:
            with conn:
                with conn.cursor(cursor_factory=RealDictCursor) as cur:
                    cur.execute(
                        """
                        INSERT INTO measures (
                            pot_id,
                            timestamp,
                            air_temp,
                            air_pressure,
                            soil_moisture,
                            illuminance
                        )
                        VALUES (
                            %s,
                            to_timestamp(%s),
                            %s, %s, %s, %s
                        )
                        RETURNING *;
                        """,
                        (pot_id, timestamp, air_temp, air_pressure, soil_moisture, illuminance),
                    )
                    inserted_row = cur.fetchone()
                    print(
                        f"[measures_insert] Inserted measures for pot_id={pot_id} at timestamp={timestamp}"
                    )
                    return inserted_row
        finally:
            conn.close()

    except IntegrityError as e:
        raise e


def get_history_measures(pot_id: str, count: int) -> list[dict[str, Any]]:
    if not pot_exists(pot_id):
        raise ValueError(f"Pot with id {pot_id} does not exist")
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                        SELECT
                            timestamp,
                            air_temp,
                            air_pressure,
                            soil_moisture,
                            illuminance
                        FROM measures
                        WHERE pot_id = %s
                        ORDER BY timestamp DESC
                        LIMIT %s;
                        """,
                    (pot_id, count),
                )
                result: list[dict[str, Any]] = []

                for row in cur.fetchall():
                    result.append(
                        {
                            "timestamp": row["timestamp"].isoformat()
                            if isinstance(row["timestamp"], datetime)
                            else row["timestamp"],
                            "data": {
                                "lux": row["illuminance"],
                                "tem": row["air_temp"],
                                "moi": row["soil_moisture"],
                                "pre": row["air_pressure"],
                            },
                        }
                    )

                return result
    finally:
        conn.close()


def update_config(pot_id: str, data: dict[str, Any]) -> dict[str, Any] | None:
    if not pot_exists(pot_id):
        raise ValueError(f"Pot with id {pot_id} does not exist")

    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    UPDATE pots
                    SET
                        pot_name = COALESCE(%(pot_name)s, pot_name),
                        max_temperature = %(max_temp)s,
                        min_temperature = %(min_temp)s,
                        min_moisture = %(min_moisture)s,
                        max_moisture = %(max_moisture)s,
                        illuminance_type = %(illuminance)s,
                        measure_interval_sec = %(measure_interval_sec)s,
                        send_interval_sec = %(send_interval_sec)s,
                        watering_interval_sec = %(watering_interval_sec)s
                    WHERE pot_id = %(pot_id)s
                    RETURNING *;
                    """,
                    {
                        "pot_id": pot_id,
                        "pot_name": data.get("pot_name"),
                        "max_temp": data["max_temp"],
                        "min_temp": data["min_temp"],
                        "min_moisture": data["min_moisture"],
                        "max_moisture": data["max_moisture"],
                        "illuminance": data["illuminance"],
                        "measure_interval_sec": data["measure_interval_sec"],
                        "send_interval_sec": data["send_interval_sec"],
                        "watering_interval_sec": data.get("watering_interval_sec"),
                    },
                )
                return cur.fetchone()
    finally:
        conn.close()


def update_owner_connection(pot_id: str, new_owner_id: str) -> None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    UPDATE connections
                    SET role = %s
                    WHERE pot_id = %s;
                    """,
                    (ConnectionRole.VIEWER.value, pot_id),
                )

                cur.execute(
                    """
                    INSERT INTO connections (user_id, pot_id, role)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (user_id, pot_id)
                    DO UPDATE SET
                        role = EXCLUDED.role;
                    """,
                    (new_owner_id, pot_id, ConnectionRole.OWNER.value),
                )
    except Exception:
        conn.rollback()
        raise SystemError("Failed to update owner connection transaction")
    finally:
        conn.close()


def get_user_pots(user_id: str) -> list[dict[str, Any]]:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT
                        p.pot_id,
                        p.pot_name,
                        p.measure_interval_sec,
                        p.send_interval_sec,
                        p.watering_interval_sec,
                        p.max_temperature,
                        p.min_temperature,
                        p.min_moisture,
                        p.max_moisture,
                        p.illuminance_type,
                        c.user_id,
                        m.timestamp,
                        m.air_temp,
                        m.air_pressure,
                        m.soil_moisture,
                        m.illuminance
                    FROM connections c
                    JOIN pots p ON p.pot_id = c.pot_id
                    LEFT JOIN LATERAL (
                        SELECT
                            timestamp,
                            air_temp,
                            air_pressure,
                            soil_moisture,
                            illuminance
                        FROM measures
                        WHERE pot_id = p.pot_id
                        ORDER BY timestamp DESC
                        LIMIT 1
                    ) m ON TRUE
                    WHERE c.user_id = %s
                    ORDER BY p.pot_id;
                    """,
                    (user_id,),
                )
                rows = cur.fetchall()

                pots: list[dict[str, Any]] = []
                for row in rows:
                    last_measure = None
                    if row["timestamp"] is not None:
                        last_measure = {
                            "timestamp": row["timestamp"].isoformat()
                            if isinstance(row["timestamp"], datetime)
                            else row["timestamp"],
                            "air_temp": row["air_temp"],
                            "air_pressure": row["air_pressure"],
                            "soil_moisture": row["soil_moisture"],
                            "illuminance": row["illuminance"],
                        }

                    pots.append(
                        {
                            "pot_id": row["pot_id"],
                            "user_id": row["user_id"],
                            "name": row["pot_name"] or row["pot_id"],
                            "config": {
                                "pot_name": row["pot_name"] or row["pot_id"],
                                "measure_interval_sec": row["measure_interval_sec"],
                                "send_interval_sec": row["send_interval_sec"],
                                "watering_interval_sec": row["watering_interval_sec"],
                                "max_temp": row["max_temperature"],
                                "min_temp": row["min_temperature"],
                                "min_moisture": row["min_moisture"],
                                "max_moisture": row["max_moisture"],
                                "illuminance": row["illuminance_type"],
                            },
                            "last_measure": last_measure,
                        }
                    )

                return pots
    finally:
        conn.close()


def delete_owner_connection(pot_id: str, user_id: str) -> str:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT role
                    FROM connections
                    WHERE pot_id = %s
                      AND user_id = %s
                    LIMIT 1;
                    """,
                    (pot_id, user_id),
                )
                row = cur.fetchone()
                if row is None:
                    return "not_found"
                if row["role"] != ConnectionRole.OWNER.value:
                    return "forbidden"

                cur.execute(
                    """
                    DELETE FROM connections
                    WHERE pot_id = %s
                      AND user_id = %s;
                    """,
                    (pot_id, user_id),
                )
                return "deleted"
    finally:
        conn.close()
