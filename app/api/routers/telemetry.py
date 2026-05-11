import logging
from fastapi import APIRouter, Depends, status, HTTPException, Query
from schemas.telemetry import TelemetryCreate, TelemetryStatusResponse, TelemetryPoint, DeviceResponse
from services.telemetry_service import TelemetryService
from api.dependencies import get_telemetry_service, verify_api_key
from core.security import get_current_user_id
from typing import List, Annotated

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/telemetry", tags=["Telemetry"])




@router.post("", response_model=TelemetryStatusResponse, status_code=status.HTTP_201_CREATED)
async def post_telemetry(
    data: TelemetryCreate,
    telemetry_service: TelemetryService = Depends(get_telemetry_service),
    api_key: str = Depends(verify_api_key)
):
    logger.info(f"Odebrano dane telemetryczne z urządzenia '{data.device_id}'")
    return await telemetry_service.save_telemetry(data)


router_user = APIRouter(prefix="", tags=["Devices"])


@router_user.get("/devices", response_model=List[DeviceResponse])
async def get_devices(
    telemetry_service: TelemetryService = Depends(get_telemetry_service),
    current_user_id: int = Depends(get_current_user_id)
):
    logger.info(f"Pobieranie listy urządzeń dla usera {current_user_id}")
    devices = await telemetry_service.get_devices_for_user(current_user_id)
    logger.info(f"Zwrócono {len(devices)} urządzeń dla usera {current_user_id}")
    return devices


@router_user.get("/telemetry/{device_id}", response_model=List[TelemetryPoint])
async def get_telemetry_history(
    device_id: str,
    user_id: Annotated[int, Depends(get_current_user_id)],
    service: Annotated[TelemetryService, Depends(get_telemetry_service)],
    range_key: str = Query(default="1d", pattern="^(1d|7d|30d)$"),
) -> list[dict]:
    try:
        return await service.get_telemetry_history(device_id, user_id, range_key)
    except PermissionError:
        raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail="Brak dostępu do urządzenia")
