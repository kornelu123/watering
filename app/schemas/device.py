from pydantic import BaseModel
from typing import Optional
from datetime import datetime

class DeviceCreate(BaseModel):
    device_id: str
    name: str

class DeviceDetail(BaseModel):
    device_id: str
    name: str
    water_duration_sec: int
    moisture_threshold_percent: int
    created_at: datetime
    group_id: Optional[int] = None
    group_name: Optional[str] = None

class DeviceSettingsUpdate(BaseModel):
    name: Optional[str] = None
    water_duration_sec: Optional[int] = None
    moisture_threshold_percent: Optional[int] = None

class WaterCommandRequest(BaseModel):
    duration_sec: Optional[int] = 10

class WateringTimeCommandRequest(BaseModel):
    duration_sec: int

class WateringThresholdCommandRequest(BaseModel):
    threshold_percent: int

