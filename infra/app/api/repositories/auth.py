from psycopg2.extras import RealDictCursor
from passlib.hash import argon2

from ..db.client import get_connection


def verify_user_credentials(email: str, password: str) -> dict | None:
    conn = get_connection()
    cur = conn.cursor(cursor_factory=RealDictCursor)

    cur.execute("SELECT * FROM users WHERE email = %s;", (email,))
    user = cur.fetchone()
    cur.close()
    conn.close()

    if not user:
        return None, "User not found"

    if argon2.verify(password, user["password_hash"]):
        return user, "User successfully logged in"
    else:
        return None, "Incorrect password"
