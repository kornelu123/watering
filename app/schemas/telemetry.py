from pydantic import BaseModel
from datetime import datetime
from typing import Optional

class TelemetryCreate(BaseModel):
    device_id: str
    water_lvl: int
    battery_lvl: int
    moisture_lvl: int
    uptime: int

class TelemetryStatusResponse(BaseModel):
    status: str

class TelemetryPoint(BaseModel):
    time: datetime
    moisture_lvl: float
    battery_lvl: float
    water_lvl: float

class DeviceResponse(BaseModel):
    device_id: str
    name: str
    water_duration_sec: int
    created_at: datetime
    group_id: Optional[int] = None
    group_name: Optional[str] = None
    moisture_lvl: Optional[float] = None
    battery_lvl: Optional[float] = None
    water_lvl: Optional[float] = None
    last_seen: Optional[datetime] = None
