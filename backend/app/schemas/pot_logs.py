from pydantic import BaseModel
from typing import Any, Optional


class PotLog(BaseModel):
    id: int
    pot_id: str
    label: Optional[str] = None
    payload: Optional[dict[str, Any]] = None
