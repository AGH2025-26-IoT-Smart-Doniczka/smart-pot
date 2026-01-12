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
