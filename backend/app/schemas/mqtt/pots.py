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
    moi: list[int]
    tem: list[float]
    mes: int
    sen: int
    wat: int | None


class LogsMqttMessage(BaseModel):
    ts: float
    lab: constr(min_length=1)
    lvl: conint(ge=1, le=4)
    data: str


class ActionMqttRequest(BaseModel):
    typ: constr(min_length=1)
    data: dict


class WaterPlantMqttRequest(BaseModel):
    dur: int  # Duration in seconds
