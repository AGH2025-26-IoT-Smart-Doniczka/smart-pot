from psycopg2.extras import RealDictCursor
from passlib.hash import argon2

from ..db.client import get_connection


def verify_user_credentials(email: str, password: str) -> dict | None:
    conn = get_connection()

    try:
        with conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                cur.execute(
                    """
                    SELECT user_id, email, username, password_hash
                    FROM users
                    WHERE email = %s;
                    """,
                    (email,)
                )
                user = cur.fetchone()

        if not user:
            return None

        if not argon2.verify(password, user["password_hash"]):
            return None

        user.pop("password_hash", None)
        return user

    finally:
        conn.close()
