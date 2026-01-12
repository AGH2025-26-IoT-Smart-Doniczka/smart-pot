from pydantic import BaseModel


class WaterPlantMqttRequest(BaseModel):
    dur: int  # Duration in seconds