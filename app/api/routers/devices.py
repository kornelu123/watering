import logging
from typing import Annotated, List

from fastapi import APIRouter, Depends, status, HTTPException

from schemas.device import (
    DeviceCreate, DeviceDetail, DeviceSettingsUpdate,
    WaterCommandRequest, WateringTimeCommandRequest,
    WateringThresholdCommandRequest
)
from services.command_service import CommandService
from api.dependencies import get_command_service, get_db_session
from core.security import get_current_user_id
import asyncpg

logger = logging.getLogger(__name__)


router = APIRouter(prefix="/devices", tags=["Devices"])


@router.post(
    "",
    response_model=DeviceDetail,
    status_code=status.HTTP_201_CREATED,
    responses={409: {"description": "Urządzenie o tym ID już istnieje."}},
)
async def add_device(
    body: DeviceCreate,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Próba dodania urządzenia '{body.device_id}' przez usera {current_user_id}")
    existing = await db.fetchval(
        "SELECT 1 FROM devices WHERE device_id = $1", body.device_id
    )
    if existing:
        logger.warning(f"Odrzucono dodanie: urządzenie '{body.device_id}' już istnieje")
        raise HTTPException(status_code=409, detail="Urządzenie o tym ID już istnieje.")

    row = await db.fetchrow(
        """
        INSERT INTO devices (device_id, user_id, name, water_duration_sec, moisture_threshold_percent)
        VALUES ($1, $2, $3, 10, 30)
        RETURNING device_id, name, water_duration_sec, moisture_threshold_percent, created_at, NULL::INT AS group_id, NULL::TEXT AS group_name
        """,
        body.device_id, current_user_id, body.name
    )
    logger.info(f"Zarejestrowano urządzenie '{body.device_id}' (user {current_user_id}, nazwa: '{body.name}')")
    return dict(row)


@router.get(
    "/{device_id}",
    response_model=DeviceDetail,
    responses={404: {"description": "Urządzenie nie istnieje."}},
)
async def get_device(
    device_id: str,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    row = await db.fetchrow(
        """
        SELECT d.device_id, d.name, d.water_duration_sec, d.moisture_threshold_percent, d.created_at,
               d.group_id, g.name AS group_name
        FROM devices d
        LEFT JOIN device_groups g ON g.id = d.group_id AND g.user_id = d.user_id
        WHERE d.device_id = $1 AND d.user_id = $2
        """,
        device_id, current_user_id
    )
    if not row:
        logger.warning(f"Nie znaleziono urządzenia '{device_id}' dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Urządzenie nie istnieje.")
    logger.info(f"Pobrano szczegóły urządzenia '{device_id}' (user {current_user_id})")
    return dict(row)


@router.put(
    "/{device_id}/settings",
    response_model=DeviceDetail,
    responses={404: {"description": "Urządzenie nie istnieje."}},
)
async def update_device_settings(
    device_id: str,
    body: DeviceSettingsUpdate,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Aktualizacja ustawień urządzenia '{device_id}' przez usera {current_user_id}: {body.model_dump(exclude_none=True)}")
    row = await db.fetchrow(
        "SELECT * FROM devices WHERE device_id = $1 AND user_id = $2",
        device_id, current_user_id
    )
    if not row:
        logger.warning(f"Nie znaleziono urządzenia '{device_id}' dla usera {current_user_id} (settings update)")
        raise HTTPException(status_code=404, detail="Urządzenie nie istnieje.")

    new_name = body.name if body.name is not None else row["name"]
    new_duration = body.water_duration_sec if body.water_duration_sec is not None else row["water_duration_sec"]
    new_threshold = body.moisture_threshold_percent if body.moisture_threshold_percent is not None else row["moisture_threshold_percent"]

    updated = await db.fetchrow(
        """
        UPDATE devices
        SET name = $1, water_duration_sec = $2, moisture_threshold_percent = $3
        WHERE device_id = $4 AND user_id = $5
        RETURNING device_id, name, water_duration_sec, moisture_threshold_percent, created_at, group_id
        """,
        new_name, new_duration, new_threshold, device_id, current_user_id
    )
    logger.info(f"Zaktualizowano ustawienia urządzenia '{device_id}' w bazie danych")

    if body.water_duration_sec is not None:
        await command_service.send_watering_time_command(
            user_id=str(current_user_id), device_id=device_id, time_ms=new_duration * 1000
        )

    if body.moisture_threshold_percent is not None:
        await command_service.send_watering_threshold_command(
            user_id=str(current_user_id), device_id=device_id, threshold_percent=new_threshold
        )

    return dict(updated)


@router.post("/{device_id}/water", status_code=status.HTTP_200_OK)
async def trigger_water(
    device_id: str,
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
    command: WaterCommandRequest | None = None,
):
    duration_sec = command.duration_sec if command and command.duration_sec is not None else 10
    logger.info(f"Żądanie natychmiastowego podlewania: urządzenie '{device_id}', czas {duration_sec}s (user {current_user_id})")
    result = await command_service.send_water_command(
        user_id=str(current_user_id), device_id=device_id, time_ms=duration_sec * 1000
    )
    return {"status": "command_published", "data": result}


@router.post("/{device_id}/watering_time", status_code=status.HTTP_200_OK)
async def trigger_watering_time(
    device_id: str,
    command: WateringTimeCommandRequest,
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Żądanie ustawienia czasu podlewania: urządzenie '{device_id}', czas {command.duration_sec}s (user {current_user_id})")
    result = await command_service.send_watering_time_command(
        user_id=str(current_user_id), device_id=device_id, time_ms=command.duration_sec * 1000
    )
    return {"status": "command_published", "data": result}


@router.post("/{device_id}/watering_threshold", status_code=status.HTTP_200_OK)
async def trigger_watering_threshold(
    device_id: str,
    command: WateringThresholdCommandRequest,
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Żądanie ustawienia progu wilgotności: urządzenie '{device_id}', próg {command.threshold_percent}% (user {current_user_id})")
    result = await command_service.send_watering_threshold_command(
        user_id=str(current_user_id), device_id=device_id, threshold_percent=command.threshold_percent
    )
    return {"status": "command_published", "data": result}


@router.post("/{device_id}/reboot", status_code=status.HTTP_200_OK)
async def trigger_reboot(
    device_id: str,
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Żądanie restartu urządzenia '{device_id}' (user {current_user_id})")
    result = await command_service.send_reboot_command(
        user_id=str(current_user_id), device_id=device_id
    )
    return {"status": "command_published", "data": result}
