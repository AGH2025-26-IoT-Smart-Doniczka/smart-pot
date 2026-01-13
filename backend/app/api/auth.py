from fastapi import APIRouter, HTTPException, status

from ..utils.jwt_token import create_access_token
from ..schemas.auth import LoginRequest
from ..integrations.repositories.auth import verify_user_credentials


router = APIRouter()
    
@router.post("/login", status_code=status.HTTP_200_OK)
def login(data: LoginRequest):
    try:
        user = verify_user_credentials(data.email, data.password)
    except Exception:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail={"message": "Internal server error"}
        )

    if not user:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail={
                "message": "Sign-in failed",
                "error": "Invalid credentials"
            }
        )

    token = create_access_token(user)

    return {
        "message": "Login successful",
        "user": {
            "id": str(user["user_id"]),
            "email": user["email"],
            "username": user["username"]
        },
        "access_token": token,
        "token_type": "Bearer"
    }
