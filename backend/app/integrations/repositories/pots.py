from datetime import datetime
from uuid import uuid4
from typing import Any

from psycopg2 import IntegrityError
from psycopg2.extras import Json, RealDictCursor

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


def pot_is_active(pot_id: str) -> bool:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT is_active
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                row = cur.fetchone()
                return bool(row["is_active"]) if row else False
    finally:
        conn.close()


def get_pot_row(pot_id: str) -> dict[str, Any] | None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT *
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                return cur.fetchone()
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


def get_mqtt_password(pot_id: str) -> str | None:
    with get_connection() as conn:
        with conn.cursor(cursor_factory=RealDictCursor) as cur:
            cur.execute(
                """
                SELECT mqtt_password
                FROM pots
                WHERE pot_id = %s;
                """,
                (pot_id,),
            )
            row = cur.fetchone()
            return row["mqtt_password"] if row else None


def set_mqtt_password(pot_id: str, mqtt_password: str) -> None:
    with get_connection() as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                UPDATE pots
                SET mqtt_password = %s
                WHERE pot_id = %s;
                """,
                (mqtt_password, pot_id),
            )


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


def insert_pot(pot_id: str, mqtt_password: str | None = None) -> dict[str, Any] | None:
    try:
        conn = get_connection()
        try:
            with conn:
                with conn.cursor(cursor_factory=RealDictCursor) as cur:
                    cur.execute(
                        """
                        INSERT INTO pots (pot_id, pot_name, mqtt_password)
                        VALUES (%s, %s, %s)
                        ON CONFLICT DO NOTHING
                        RETURNING *;
                        """,
                        (pot_id, pot_id, mqtt_password),
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


def pot_logs_insert(
    pot_id: str,
    timestamp: float,
    label: str,
    payload: dict[str, Any],
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
                        INSERT INTO pot_logs (
                            pot_id,
                            timestamp,
                            label,
                            payload
                        )
                        VALUES (
                            %s,
                            to_timestamp(%s),
                            %s,
                            %s
                        )
                        RETURNING *;
                        """,
                        (pot_id, timestamp, label, Json(payload)),
                    )
                    inserted_row = cur.fetchone()
                    print(
                        f"[pot_logs_insert] Inserted log for pot_id={pot_id} at timestamp={timestamp}"
                    )
                    return inserted_row
        finally:
            conn.close()

    except IntegrityError as e:
        raise e


def get_user_pot_logs(user_id: str, count: int) -> list[dict[str, Any]]:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT
                        l.id,
                        l.pot_id,
                        l.timestamp,
                        l.label,
                        l.payload,
                        p.pot_name
                    FROM pot_logs l
                    JOIN connections c ON c.pot_id = l.pot_id
                    JOIN pots p ON p.pot_id = l.pot_id
                    WHERE c.user_id = %s
                    ORDER BY l.timestamp DESC
                    LIMIT %s;
                    """,
                    (user_id, count),
                )
                rows = cur.fetchall()
                logs: list[dict[str, Any]] = []
                for row in rows:
                    logs.append(
                        {
                            "id": row["id"],
                            "pot_id": row["pot_id"],
                            "pot_name": row["pot_name"] or row["pot_id"],
                            "timestamp": row["timestamp"].isoformat()
                            if isinstance(row["timestamp"], datetime)
                            else row["timestamp"],
                            "label": row["label"],
                            "payload": row["payload"],
                        }
                    )
                return logs
    finally:
        conn.close()


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


def get_history_measures_aggregated(
    pot_id: str,
    start_ts: datetime,
    end_ts: datetime,
    bucket_seconds: int,
) -> list[dict[str, Any]]:
    if not pot_exists(pot_id):
        raise ValueError(f"Pot with id {pot_id} does not exist")
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                        SELECT
                            to_timestamp(
                                floor(extract(epoch from timestamp) / %s) * %s
                            ) AS bucket_ts,
                            avg(air_temp) AS air_temp_avg,
                            min(air_temp) AS air_temp_min,
                            max(air_temp) AS air_temp_max,
                            avg(air_pressure) AS air_pressure_avg,
                            min(air_pressure) AS air_pressure_min,
                            max(air_pressure) AS air_pressure_max,
                            avg(soil_moisture) AS soil_moisture_avg,
                            min(soil_moisture) AS soil_moisture_min,
                            max(soil_moisture) AS soil_moisture_max,
                            avg(illuminance) AS illuminance_avg,
                            min(illuminance) AS illuminance_min,
                            max(illuminance) AS illuminance_max
                        FROM measures
                        WHERE pot_id = %s
                          AND timestamp >= %s
                          AND timestamp <= %s
                        GROUP BY bucket_ts
                        ORDER BY bucket_ts ASC;
                        """,
                    (bucket_seconds, bucket_seconds, pot_id, start_ts, end_ts),
                )
                result: list[dict[str, Any]] = []
                for row in cur.fetchall():
                    bucket_ts = row["bucket_ts"]
                    result.append(
                        {
                            "timestamp": bucket_ts.isoformat()
                            if isinstance(bucket_ts, datetime)
                            else bucket_ts,
                            "data": {
                                "air_temp": {
                                    "avg": row["air_temp_avg"],
                                    "min": row["air_temp_min"],
                                    "max": row["air_temp_max"],
                                },
                                "air_pressure": {
                                    "avg": row["air_pressure_avg"],
                                    "min": row["air_pressure_min"],
                                    "max": row["air_pressure_max"],
                                },
                                "soil_moisture": {
                                    "avg": row["soil_moisture_avg"],
                                    "min": row["soil_moisture_min"],
                                    "max": row["soil_moisture_max"],
                                },
                                "illuminance": {
                                    "avg": row["illuminance_avg"],
                                    "min": row["illuminance_min"],
                                    "max": row["illuminance_max"],
                                },
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
                        watering_interval_sec = %(watering_interval_sec)s,
                        watering_duration_sec = %(watering_duration_sec)s
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
                        "watering_duration_sec": data.get("watering_duration_sec"),
                    },
                )
                return cur.fetchone()
    finally:
        conn.close()


def update_pot_name(pot_id: str, pot_name: str | None) -> dict[str, Any] | None:
    if not pot_exists(pot_id):
        raise ValueError(f"Pot with id {pot_id} does not exist")
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    UPDATE pots
                    SET pot_name = %s
                    WHERE pot_id = %s
                    RETURNING *;
                    """,
                    (pot_name, pot_id),
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
                        p.is_active,
                        p.pot_name,
                        p.measure_interval_sec,
                        p.send_interval_sec,
                        p.watering_interval_sec,
                        p.watering_duration_sec,
                        p.max_temperature,
                        p.min_temperature,
                        p.min_moisture,
                        p.max_moisture,
                        p.illuminance_type,
                        c.user_id,
                        c.role,
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
                            "role": row["role"],
                            "name": row["pot_name"] or row["pot_id"],
                            "is_active": row["is_active"],
                            "config": {
                                "pot_name": row["pot_name"] or row["pot_id"],
                                "measure_interval_sec": row["measure_interval_sec"],
                                "send_interval_sec": row["send_interval_sec"],
                                "watering_interval_sec": row["watering_interval_sec"],
                                "watering_duration_sec": row["watering_duration_sec"],
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


def user_has_write_role(pot_id: str, user_id: str) -> bool:
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
                    return False
                return row["role"] in (
                    ConnectionRole.OWNER.value,
                    ConnectionRole.EDITOR.value,
                )
    finally:
        conn.close()


def get_connection_role(pot_id: str, user_id: str) -> str | None:
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
                return row["role"] if row else None
    finally:
        conn.close()


def list_connections(pot_id: str) -> list[dict[str, Any]]:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT c.user_id, c.role, u.email
                    FROM connections c
                    LEFT JOIN users u ON u.user_id = c.user_id
                    WHERE pot_id = %s
                    ORDER BY role DESC, user_id;
                    """,
                    (pot_id,),
                )
                return cur.fetchall()
    finally:
        conn.close()


def upsert_connection_role(pot_id: str, user_id: str, role: str) -> str:
    if role not in (ConnectionRole.VIEWER.value, ConnectionRole.EDITOR.value):
        raise ValueError("Role must be VIEWER or EDITOR")

    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    INSERT INTO connections (user_id, pot_id, role)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (user_id, pot_id)
                    DO UPDATE SET role = EXCLUDED.role;
                    """,
                    (user_id, pot_id, role),
                )
        return "upserted"
    finally:
        conn.close()


def delete_connection(pot_id: str, user_id: str) -> str:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    DELETE FROM connections
                    WHERE pot_id = %s
                      AND user_id = %s
                    RETURNING role;
                    """,
                    (pot_id, user_id),
                )
                row = cur.fetchone()
                return row["role"] if row else ""
    finally:
        conn.close()


def delete_connections_for_pot(pot_id: str) -> None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    DELETE FROM connections
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
    finally:
        conn.close()


def delete_measures_for_pot(pot_id: str) -> None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    DELETE FROM measures
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
    finally:
        conn.close()


def reset_pot_after_hard_reset(pot_id: str) -> bool:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    UPDATE pots
                    SET
                        pot_name = NULL,
                        measure_interval_sec = DEFAULT,
                        send_interval_sec = DEFAULT,
                        watering_interval_sec = NULL,
                        watering_duration_sec = NULL,
                        min_temperature = DEFAULT,
                        max_temperature = DEFAULT,
                        min_moisture = DEFAULT,
                        max_moisture = DEFAULT,
                        illuminance_type = DEFAULT
                    WHERE pot_id = %s
                    RETURNING pot_id;
                    """,
                    (pot_id,),
                )
                return cur.fetchone() is not None
    finally:
        conn.close()


def archive_pot(pot_id: str) -> dict[str, Any] | None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT *
                    FROM pots
                    WHERE pot_id = %s
                      AND is_active = TRUE;
                    """,
                    (pot_id,),
                )
                row = cur.fetchone()
                if row is None:
                    return None

                new_pot_id = str(uuid4())
                cur.execute(
                    """
                    UPDATE pots
                    SET pot_id = %s,
                        is_active = FALSE
                    WHERE pot_id = %s;
                    """,
                    (new_pot_id, pot_id),
                )
                row["archived_pot_id"] = new_pot_id
                return row
    finally:
        conn.close()


def set_owner_with_previous_editor(pot_id: str, new_owner_id: str) -> None:
    if not user_exists(new_owner_id):
        raise ValueError(f"User with id {new_owner_id} does not exist")

    conn = get_connection()
    try:
        with conn:
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
                row = cur.fetchone()
                prev_owner_id = row[0] if row else None

                if prev_owner_id and prev_owner_id != new_owner_id:
                    cur.execute(
                        """
                        UPDATE connections
                        SET role = %s
                        WHERE pot_id = %s
                          AND user_id = %s;
                        """,
                        (ConnectionRole.EDITOR.value, pot_id, prev_owner_id),
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
    finally:
        conn.close()


def apply_hard_reset(pot_id: str, new_owner_id: str, mqtt_password: str) -> None:
    if not user_exists(new_owner_id):
        raise ValueError(f"User with id {new_owner_id} does not exist")

    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    SELECT mqtt_password
                    FROM pots
                    WHERE pot_id = %s
                      AND is_active = TRUE;
                    """,
                    (pot_id,),
                )
                row = cur.fetchone()
                if row is not None:
                    new_archived_id = str(uuid4())
                    cur.execute(
                        """
                        UPDATE pots
                        SET pot_id = %s,
                            is_active = FALSE
                        WHERE pot_id = %s;
                        """,
                        (new_archived_id, pot_id),
                    )

                cur.execute(
                    """
                    INSERT INTO pots (pot_id, pot_name, mqtt_password, is_active)
                    VALUES (%s, %s, %s, TRUE)
                    ON CONFLICT (pot_id)
                    DO UPDATE SET
                        pot_name = NULL,
                        measure_interval_sec = DEFAULT,
                        send_interval_sec = DEFAULT,
                        watering_interval_sec = NULL,
                        watering_duration_sec = NULL,
                        min_temperature = DEFAULT,
                        max_temperature = DEFAULT,
                        min_moisture = DEFAULT,
                        max_moisture = DEFAULT,
                        illuminance_type = DEFAULT,
                        mqtt_password = EXCLUDED.mqtt_password,
                        is_active = TRUE;
                    """,
                    (pot_id, pot_id, mqtt_password),
                )
                rows = cur.fetchall()

                cur.execute(
                    """
                    INSERT INTO connections (user_id, pot_id, role)
                    VALUES (%s, %s, %s)
                    ON CONFLICT (user_id, pot_id)
                    DO UPDATE SET role = EXCLUDED.role;
                    """,
                    (new_owner_id, pot_id, ConnectionRole.OWNER.value),
                )
    finally:
        conn.close()


def delete_pot(pot_id: str) -> bool:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor() as cur:
                cur.execute(
                    """
                    SELECT 1
                    FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                exists = cur.fetchone() is not None
                if not exists:
                    return False

                cur.execute(
                    """
                    DELETE FROM measures
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                cur.execute(
                    """
                    DELETE FROM pot_logs
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                cur.execute(
                    """
                    DELETE FROM connections
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                cur.execute(
                    """
                    DELETE FROM pots
                    WHERE pot_id = %s;
                    """,
                    (pot_id,),
                )
                return True
    finally:
        conn.close()
