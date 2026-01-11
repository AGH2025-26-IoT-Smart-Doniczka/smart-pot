from fastapi import FastAPI
from api.endpoints.user import router as user_router
from api.endpoints.auth import router as auth_router


app = FastAPI()

app.include_router(user_router, prefix="/user", tags=["user"])
app.include_router(auth_router, prefix="/auth", tags=["auth"])