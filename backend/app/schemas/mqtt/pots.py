from typing import Any
from pydantic import BaseModel, conint, constr


class AddUserRequest(BaseModel):
    username: str
    password: str


class TelemetryData(BaseModel):
    lux: conint(ge=0)  # int >= 0
    tem: float
    moi: conint(ge=0, le=100)
    pre: conint(ge=0)


class TelemetryMqttMessage(BaseModel):
    ts: float
    data: TelemetryData


class ConfigChangeMqttRequest(BaseModel):
    lux: int
    moi: tuple[int, int]
    tem: tuple[float, float]
    mes: int
    sen: int
    wat: int


class LogsMqttMessage(BaseModel):
    ts: float
    lab: constr(min_length=1)
    lvl: conint(ge=1, le=4)
    data: str


class WaterPlantMqttRequest(BaseModel):
    dur: int  # Duration in seconds


class ActionMqttRequest(BaseModel):
    typ: constr(min_length=1)
    data: WaterPlantMqttRequest | dict[str, Any]
