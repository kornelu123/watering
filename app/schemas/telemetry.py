from pydantic import BaseModel

class TelemetryCreate(BaseModel):
    device_id: str
    water_lvl: int
    battery_lvl: int
    moisture_lvl: int
    uptime: int

class TelemetryResponse(TelemetryCreate):
    id: int
    user_id: str
    timestamp: int
