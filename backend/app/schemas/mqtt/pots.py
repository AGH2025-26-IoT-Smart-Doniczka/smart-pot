from pydantic import field_validator
from pydantic import BaseModel, conint, constr


class WaterPlantMqttRequest(BaseModel):
    dur: int  # Duration in seconds

class WateringStatusMqttResponse(BaseModel):
    water: int

    @field_validator("water")
    def check_water(cls, v):
        if v not in (0, 1):
            raise ValueError(f"water must be 0 or 1, got {v}")
        return v
    

class AddUserRequest(BaseModel):
    username: str
    password: str
    

class TelemetryData(BaseModel):
    lux: conint(ge=0)   # int >= 0
    tem: float
    moi: conint(ge=0)
    pre: float

class TelemetryMqttMessage(BaseModel):
    timestamp: float
    data: TelemetryData


class ConfigChangeMqttRequest(BaseModel):
	lux: int
	moi: list[int]
	tem: list[float] 
	sle: int