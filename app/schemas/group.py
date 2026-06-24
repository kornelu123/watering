from datetime import datetime
from typing import Optional, List

from pydantic import BaseModel


class DeviceAssignment(BaseModel):
    device_id: str
    device_name: str


class GroupCreate(BaseModel):
    name: str
    moisture_threshold_percent: int = 30
    watering_duration_sec: int = 10
    device_id: Optional[str] = None


class GroupUpdate(BaseModel):
    name: Optional[str] = None
    moisture_threshold_percent: Optional[int] = None
    watering_duration_sec: Optional[int] = None


class GroupDetail(BaseModel):
    id: int
    name: str
    moisture_threshold_percent: int
    watering_duration_sec: int
    created_at: datetime
    devices: List[DeviceAssignment] = []