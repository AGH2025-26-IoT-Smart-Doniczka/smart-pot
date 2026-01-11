from datetime import datetime, timedelta, timezone
import jwt
import os


SECRET_KEY = os.getenv("SECRET_KEY", "smartpotsecretkey")
ALGORITHM = "HS256"


def create_access_token(user: dict):
    expire = datetime.now(timezone.utc) + timedelta(hours=1)
    payload = {
        "sub": str(user["user_id"]),
        "email": user["email"],
        "username": user["username"],
        "exp": int(expire.timestamp())
    }
    token = jwt.encode(payload, SECRET_KEY, algorithm=ALGORITHM)
    return token

