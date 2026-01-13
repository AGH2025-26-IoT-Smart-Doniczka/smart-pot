from fastapi import FastAPI

from app.api.user import router as user_router
from app.api.auth import router as auth_router
from app.api.pots import router as pots_router


app = FastAPI()

app.include_router(user_router, prefix="/user", tags=["user"])
app.include_router(auth_router, prefix="/auth", tags=["auth"])
app.include_router(pots_router, prefix="/pots", tags=["pots"])