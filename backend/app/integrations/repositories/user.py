from psycopg2 import IntegrityError
from psycopg2.extras import RealDictCursor

from ..db.client import get_connection


def create_user(email: str, username: str, password_hash: str) -> dict:
    conn = get_connection()

    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    INSERT INTO users (email, password_hash, username)
                    VALUES (%s, %s, %s)
                    RETURNING user_id, email, username;
                    """,
                    (email, password_hash, username)
                )
                return cur.fetchone()

    except IntegrityError:
        raise

    finally:
        conn.close()


def get_user_id_by_email(email: str) -> str | None:
    conn = get_connection()
    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT user_id
                    FROM users
                    WHERE lower(email) = lower(%s)
                    LIMIT 1;
                    """,
                    (email,),
                )
                row = cur.fetchone()
                return row["user_id"] if row else None
    finally:
        conn.close()
