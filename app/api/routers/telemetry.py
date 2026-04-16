from fastapi import APIRouter, Depends, status
from schemas.telemetry import TelemetryCreate, TelemetryResponse
from services.telemetry_service import TelemetryService
from api.dependencies import get_telemetry_service

router = APIRouter(prefix="/telemetry", tags=["Telemetry"])

@router.post("", response_model=TelemetryResponse, status_code=status.HTTP_201_CREATED)
async def post_telemetry(
    data: TelemetryCreate,
    telemetry_service: TelemetryService = Depends(get_telemetry_service)
):
    saved_data = await telemetry_service.save_telemetry(data)
    return saved_data
