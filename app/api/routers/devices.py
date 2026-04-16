from fastapi import APIRouter, Depends, status
from typing import Optional
from schemas.device import WaterCommandRequest
from services.command_service import CommandService
from api.dependencies import get_command_service

router = APIRouter(prefix="/users/{user_id}/devices", tags=["Devices"])

@router.post("/{device_id}/water", status_code=status.HTTP_200_OK)
async def trigger_water(
    user_id: str,
    device_id: str,
    command: Optional[WaterCommandRequest] = None,
    command_service: CommandService = Depends(get_command_service)
):
    duration = command.duration_sec if command and command.duration_sec is not None else 10
    
    result = await command_service.send_water_command(
        user_id=user_id,
        device_id=device_id,
        duration_sec=duration
    )
    
    return {"status": "command_published", "data": result}
