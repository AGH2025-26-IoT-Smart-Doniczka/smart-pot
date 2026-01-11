from fastapi import APIRouter, HTTPException, status

from ..schemas.user import RegisterRequest
from ..repositories.user import email_exists, create_user
from ..utils.hash import hash_password
from ..utils.jwt_token import create_access_token


router = APIRouter()

@router.post("/", status_code=status.HTTP_201_CREATED)
def register(data: RegisterRequest):
    if email_exists(data.email):
        raise HTTPException(
            status_code=status.HTTP_409_CONFLICT,
            detail={
                "message": "User was not created.",
                "error": "Email already in use."
            }
        )

    password_hash, salt = hash_password(data.password)
    user = create_user(data.email, data.username, password_hash, salt)
    token = create_access_token(user)

    return {
        "message": "User successfully created",
        "user": {
            "email": user["email"],
            "username": user["username"],
            "id": str(user["user_id"])
        },
        "jwt": token
    }