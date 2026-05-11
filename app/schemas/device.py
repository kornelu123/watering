from pydantic import BaseModel
from typing import Optional

class DeviceModel(BaseModel):
    device_id: str
    user_id: str
    name: str

class WaterCommandRequest(BaseModel):
    duration_sec: Optional[int] = 10

class WateringTimeCommandRequest(BaseModel):
    duration_sec: int

class WateringThresholdCommandRequest(BaseModel):
    threshold_percent: int
