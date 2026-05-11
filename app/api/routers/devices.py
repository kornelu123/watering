from fastapi import APIRouter, Depends, status
from typing import Optional
from schemas.device import WaterCommandRequest, WateringTimeCommandRequest, WateringThresholdCommandRequest
from services.command_service import CommandService
from api.dependencies import get_command_service, verify_api_key

router = APIRouter(prefix="/users/{user_id}/devices", tags=["Devices"])

@router.post("/{device_id}/water", status_code=status.HTTP_200_OK)
async def trigger_water(
    user_id: str,
    device_id: str,
    command: Optional[WaterCommandRequest] = None,
    command_service: CommandService = Depends(get_command_service),
    api_key: str = Depends(verify_api_key)
):
    duration_sec = command.duration_sec if command and command.duration_sec is not None else 10
    
    result = await command_service.send_water_command(
        user_id=user_id,
        device_id=device_id,
        time_ms=duration_sec * 1000
    )
    
    return {"status": "command_published", "data": result}

@router.post("/{device_id}/watering_time", status_code=status.HTTP_200_OK)
async def trigger_watering_time(
    user_id: str,
    device_id: str,
    command: WateringTimeCommandRequest,
    command_service: CommandService = Depends(get_command_service),
    api_key: str = Depends(verify_api_key)
):
    result = await command_service.send_watering_time_command(
        user_id=user_id,
        device_id=device_id,
        time_ms=command.duration_sec * 1000
    )
    
    return {"status": "command_published", "data": result}

@router.post("/{device_id}/watering_threshold", status_code=status.HTTP_200_OK)
async def trigger_watering_threshold(
    user_id: str,
    device_id: str,
    command: WateringThresholdCommandRequest,
    command_service: CommandService = Depends(get_command_service),
    api_key: str = Depends(verify_api_key)
):
    result = await command_service.send_watering_threshold_command(
        user_id=user_id,
        device_id=device_id,
        threshold_percent=command.threshold_percent
    )
    
    return {"status": "command_published", "data": result}

@router.post("/{device_id}/reboot", status_code=status.HTTP_200_OK)
async def trigger_reboot(
    user_id: str,
    device_id: str,
    command_service: CommandService = Depends(get_command_service),
    api_key: str = Depends(verify_api_key)
):
    result = await command_service.send_reboot_command(
        user_id=user_id,
        device_id=device_id
    )
    
    return {"status": "command_published", "data": result}
