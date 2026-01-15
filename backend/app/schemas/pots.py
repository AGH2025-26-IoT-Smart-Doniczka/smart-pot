from pydantic import BaseModel, field_validator, model_validator
from typing import Literal

class WaterPlantRequest(BaseModel):
   duration: int  # Duration in seconds


class WateringStatusResponse(BaseModel):
    is_watering: bool


class PairingRequest(BaseModel):
    user_id: str


class HumidityRange(BaseModel):
    min: int
    opt_min: int
    opt_max: int
    max: int

    @field_validator("min", "opt_min", "opt_max", "max")
    @classmethod
    def humidity_non_negative(cls, v: int) -> int:
        if v < 0 or v > 100:
            raise ValueError("Humidity must be between 0 and 100")
        return v

    @model_validator(mode="after")
    def check_order(self):
        if not (self.min <= self.opt_min <= self.opt_max <= self.max):
            raise ValueError(
                "Humidity values must satisfy: min ≤ opt_min ≤ opt_max ≤ max"
            )
        return self

class ConfigChangeRequest(BaseModel):
    sleep_interval_sec: int
    max_temp: float
    min_temp: float
    humidity: HumidityRange
    illuminance: Literal["low", "medium", "high"]

    @field_validator("sleep_interval_sec")
    @classmethod
    def sleep_interval_positive(cls, v: int) -> int:
        if v <= 0:
            raise ValueError("sleep_interval_sec must be > 0")
        return v

    @field_validator("min_temp", "max_temp")
    @classmethod
    def temperature_reasonable(cls, v: float) -> float:
        if v < -40 or v > 100:
            raise ValueError("Temperature out of realistic range")
        return v

    @model_validator(mode="after")
    def check_temperature_order(self):
        if self.min_temp >= self.max_temp:
            raise ValueError("min_temp must be < max_temp")
        return self


class ChangeOwnerRequest(BaseModel):
    user_id: str