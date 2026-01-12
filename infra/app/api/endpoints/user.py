from fastapi import APIRouter, HTTPException, status
from psycopg2 import IntegrityError

from ..schemas.user import RegisterRequest
from ..repositories.user import create_user
from ..utils.hash import hash_password
from ..utils.jwt_token import create_access_token


router = APIRouter()

@router.post("", status_code=status.HTTP_201_CREATED)
def register(data: RegisterRequest):
    password_hash = hash_password(data.password)

    try:
        user = create_user(
            email=data.email,
            username=data.username,
            password_hash=password_hash
        )
    except IntegrityError:
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail={
                "message": "User was not created",
                "error": "Email already in use"
            }
        )
    except Exception:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail={
                "message": "User was not created",
                "error": "Internal server error"
            }
        )

    token = create_access_token(user)

    return {
        "message": "User successfully created",
        "user": {
            "id": str(user["user_id"]),
            "email": user["email"],
            "username": user["username"]
        },
        "access_token": token,
        "token_type": "Bearer"
    }
