from pydantic import BaseModel


class WaterPlantRequest(BaseModel):
    duration: int  # Duration in seconds
