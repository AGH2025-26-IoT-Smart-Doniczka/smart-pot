from fastapi import APIRouter, HTTPException, status

from ..utils.jwt_token import create_access_token
from ..utils.hash import hash_password
from ..schemas.auth import LoginRequest
from ..repositories.auth import verify_user_credentials


router = APIRouter()
    
@router.post("/login", status_code=status.HTTP_200_OK)
def login(data: LoginRequest):
    user, message = verify_user_credentials(data.email, data.password)
    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail={
                "message": "Sign-in failed.",
                "error": message
            }
        )
    token = create_access_token(user)

    return {
        "message": message,
        "user": {
            "email": user["email"],
            "username": user["username"],
            "id": str(user["user_id"])
        },
        "jwt": token
    }