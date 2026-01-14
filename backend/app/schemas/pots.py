from pydantic import BaseModel


class WaterPlantRequest(BaseModel):
    duration: int  # Duration in seconds


class PairingRequest(BaseModel):
    user_id: str


class AddUserRequest(BaseModel):
    username: str
    password: str