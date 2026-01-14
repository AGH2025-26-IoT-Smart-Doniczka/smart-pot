from pydantic import BaseModel


class WaterPlantRequest(BaseModel):
   duration: int  # Duration in seconds


class WateringStatusResponse(BaseModel):
    is_watering: bool


class PairingRequest(BaseModel):
    user_id: str


class AddUserRequest(BaseModel):
    username: str
    password: str