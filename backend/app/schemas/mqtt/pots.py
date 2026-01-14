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
    

class TelemetryData(BaseModel):
    lux: conint(ge=0)   # int >= 0
    tem: float
    moi: conint(ge=0)
    pre: float

class TelemetryMqttMessage(BaseModel):
    potId: constr(min_length=12, max_length=12)  # dokładnie 12 znaków
    timestamp: float
    data: TelemetryData
