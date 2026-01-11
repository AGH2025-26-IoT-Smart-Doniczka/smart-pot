from ..db.client import get_connection
from psycopg2.extras import RealDictCursor


def email_exists(email: str) -> bool:
    conn = get_connection()
    cur = conn.cursor()

    cur.execute(
        "SELECT 1 FROM users WHERE email = %s LIMIT 1;",
        (email,)
    )

    exists = cur.fetchone() is not None

    cur.close()
    conn.close()

    return exists


def create_user(email: str, username: str, password_hash: str, salt: str) -> dict:
    conn = get_connection()
    cur = conn.cursor(cursor_factory=RealDictCursor)
    try:
        cur.execute(
            """
            INSERT INTO users (email, password_hash, salt, username)
            VALUES (%s, %s, %s, %s)
            RETURNING user_id, email, username;
            """,
            (email, password_hash, salt, username)
        )
        user = cur.fetchone()
        conn.commit()
        return user
    finally:
        cur.close()
        conn.close()