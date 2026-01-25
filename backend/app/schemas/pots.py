from pydantic import BaseModel, field_validator, model_validator
from typing import Literal, Optional
from uuid import UUID
from .roles import ConnectionRole


class WaterPlantRequest(BaseModel):
   duration: int  # Duration in seconds


class WateringStatusResponse(BaseModel):
    is_watering: bool


class WateringStatusResponse(BaseModel):
    is_watering: bool


class PairingRequest(BaseModel):
    user_id: str


class ConfigChangeRequest(BaseModel):
    pot_name: Optional[str] = None
    measure_interval_sec: int
    send_interval_sec: int
    watering_interval_sec: Optional[int] = None
    watering_duration_sec: Optional[int] = None
    max_temp: float
    min_temp: float
    min_moisture: int
    max_moisture: int
    illuminance: Literal["low", "medium", "high"]

    @field_validator("pot_name")
    @classmethod
    def pot_name_non_empty(cls, v: Optional[str]) -> Optional[str]:
        if v is not None and not v.strip():
            raise ValueError("pot_name cannot be empty")
        return v

    @field_validator("measure_interval_sec", "send_interval_sec")
    @classmethod
    def interval_positive(cls, v: int) -> int:
        if v <= 0:
            raise ValueError("intervals must be > 0")
        return v

    @field_validator("watering_interval_sec")
    @classmethod
    def watering_interval_positive(cls, v: Optional[int]) -> Optional[int]:
        if v is not None and v <= 0:
            raise ValueError("watering_interval_sec must be > 0")
        return v

    @field_validator("watering_duration_sec")
    @classmethod
    def watering_duration_positive(cls, v: Optional[int]) -> Optional[int]:
        if v is not None and v <= 0:
            raise ValueError("watering_duration_sec must be > 0")
        if v is not None and v > 60:
            raise ValueError("watering_duration_sec must be <= 60")
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

    @field_validator("min_moisture", "max_moisture")
    @classmethod
    def moisture_in_range(cls, v: int) -> int:
        if v < 0 or v > 100:
            raise ValueError("Moisture must be between 0 and 100")
        return v

    @model_validator(mode="after")
    def check_moisture_order(self):
        if self.min_moisture > self.max_moisture:
            raise ValueError("min_moisture must be <= max_moisture")
        return self


class PotConfigResponse(BaseModel):
    pot_name: str
    measure_interval_sec: int
    send_interval_sec: int
    watering_interval_sec: Optional[int] = None
    watering_duration_sec: Optional[int] = None
    max_temp: float
    min_temp: float
    min_moisture: int
    max_moisture: int
    illuminance: Literal["low", "medium", "high"]


class PotListItemResponse(BaseModel):
    pot_id: str
    user_id: str
    role: ConnectionRole
    name: str
    last_measure: dict | None
    config: PotConfigResponse
    is_active: bool


class PotListResponse(BaseModel):
    pots: list[PotListItemResponse]


class PotRenameRequest(BaseModel):
    pot_name: Optional[str] = None

    @field_validator("pot_name")
    @classmethod
    def pot_name_non_empty(cls, v: Optional[str]) -> Optional[str]:
        if v is not None and not v.strip():
            raise ValueError("pot_name cannot be empty")
        return v


class PotConfigResponse(BaseModel):
    pot_name: str
    measure_interval_sec: int
    send_interval_sec: int
    watering_interval_sec: Optional[int] = None
    max_temp: float
    min_temp: float
    humidity: HumidityRange
    illuminance: Literal["low", "medium", "high"]


class PotListItemResponse(BaseModel):
    pot_id: str
    user_id: str
    name: str
    last_measure: dict | None
    config: PotConfigResponse


class PotListResponse(BaseModel):
    pots: list[PotListItemResponse]


class ChangeOwnerRequest(BaseModel):
    user_id: str


class ConnectionRoleRequest(BaseModel):
    user_id: UUID | None = None
    email: str | None = None
    role: str

    @field_validator("role")
    @classmethod
    def role_allowed(cls, v: str) -> str:
        return v.strip().upper()

    @model_validator(mode="after")
    def has_identifier(self):
        if self.user_id is None and (self.email is None or not self.email.strip()):
            raise ValueError("user_id or email is required")
        return self


class ConnectionDeleteRequest(BaseModel):
    user_id: UUID | None = None
    email: str | None = None

    @model_validator(mode="after")
    def has_identifier(self):
        if self.user_id is None and (self.email is None or not self.email.strip()):
            raise ValueError("user_id or email is required")
        return self
