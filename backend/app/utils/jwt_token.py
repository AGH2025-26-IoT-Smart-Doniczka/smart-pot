from datetime import datetime, timedelta, timezone
import jwt
from jwt import InvalidTokenError
import os


SECRET_KEY = os.getenv("SECRET_KEY", "smartpotsecretkey")
ALGORITHM = "HS256"


def create_access_token(user: dict):
    expire = datetime.now(timezone.utc) + timedelta(hours=1)
    payload = {
        "sub": str(user["user_id"]),
        "email": user["email"],
        "username": user["username"],
        "exp": int(expire.timestamp()),
    }
    token = jwt.encode(payload, SECRET_KEY, algorithm=ALGORITHM)
    return token


def decode_access_token(token: str) -> dict:
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        return payload
    except InvalidTokenError as exc:
        raise exc
