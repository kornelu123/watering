import json
import logging
from typing import Annotated, List

import asyncpg
from fastapi import APIRouter, Depends, HTTPException, status

from api.dependencies import get_db_session, get_command_service
from core.security import get_current_user_id
from schemas.group import DeviceAssignment, GroupCreate, GroupDetail, GroupUpdate
from services.command_service import CommandService

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/groups", tags=["Groups"])


async def _fetch_group_row(db: asyncpg.Connection, group_id: int, user_id: int):
    return await db.fetchrow(
        """
        SELECT
            g.id,
            g.name,
            g.moisture_threshold_percent,
            g.watering_duration_sec,
            g.created_at,
            COALESCE(
                json_agg(json_build_object('device_id', d.device_id, 'device_name', d.name))
                FILTER (WHERE d.device_id IS NOT NULL),
                '[]'::json
            )::text AS devices
        FROM device_groups g
        LEFT JOIN devices d ON d.group_id = g.id AND d.user_id = g.user_id
        WHERE g.id = $1 AND g.user_id = $2
        GROUP BY g.id, g.name, g.moisture_threshold_percent, g.watering_duration_sec, g.created_at
        """,
        group_id,
        user_id,
    )


async def _fetch_device_row(db: asyncpg.Connection, device_id: str, user_id: int):
    return await db.fetchrow(
        "SELECT device_id, group_id FROM devices WHERE device_id = $1 AND user_id = $2",
        device_id,
        user_id,
    )


def _row_to_group_detail(row):
    row_dict = dict(row)
    devices_raw = row_dict.get("devices", [])

    if isinstance(devices_raw, str):
        try:
            devices_raw = json.loads(devices_raw)
        except json.JSONDecodeError:
            devices_raw = []

    if devices_raw is None:
        devices_raw = []

    if isinstance(devices_raw, dict):
        devices_raw = [devices_raw]

    row_dict["devices"] = [DeviceAssignment(**device) for device in devices_raw]
    return GroupDetail(**row_dict)


async def _fetch_group_devices(db: asyncpg.Connection, group_id: int, user_id: int) -> List[str]:
    """Fetch list of device IDs in a group."""
    rows = await db.fetch(
        "SELECT device_id FROM devices WHERE group_id = $1 AND user_id = $2",
        group_id,
        user_id,
    )
    return [row["device_id"] for row in rows]


async def _publish_group_settings_to_devices(
    db: asyncpg.Connection,
    command_service: CommandService,
    group_id: int,
    user_id: int,
    moisture_threshold: int,
    watering_duration: int,
):
    """Apply group settings to all devices in the group and publish commands."""
    device_ids = await _fetch_group_devices(db, group_id, user_id)
    
    if not device_ids:
        logger.info(f"Grupa {group_id} nie ma żadnych urządzeń, pominięcie publikowania komend")
        return
    
    logger.info(f"Publikowanie ustawień grupy {group_id} do {len(device_ids)} urządzeń")
    
    # Update device settings in database
    await db.execute(
        """
        UPDATE devices 
        SET moisture_threshold_percent = $1, water_duration_sec = $2
        WHERE group_id = $3 AND user_id = $4
        """,
        moisture_threshold,
        watering_duration,
        group_id,
        user_id,
    )
    
    # Publish commands to devices
    for device_id in device_ids:
        try:
            await command_service.send_watering_time_command(
                user_id=str(user_id),
                device_id=device_id,
                time_ms=watering_duration * 1000
            )
            await command_service.send_watering_threshold_command(
                user_id=str(user_id),
                device_id=device_id,
                threshold_percent=moisture_threshold
            )
            logger.info(f"Opublikowano ustawienia grupy do urządzenia {device_id}")
        except Exception:
            logger.exception(f"Błąd publikowania ustawień grupy do urządzenia {device_id}")
            raise


@router.get("", response_model=List[GroupDetail])
async def list_groups(
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Pobieranie listy grup dla usera {current_user_id}")
    rows = await db.fetch(
        """
        SELECT
            g.id,
            g.name,
            g.moisture_threshold_percent,
            g.watering_duration_sec,
            g.created_at,
            COALESCE(
                json_agg(json_build_object('device_id', d.device_id, 'device_name', d.name))
                FILTER (WHERE d.device_id IS NOT NULL),
                '[]'::json
            )::text AS devices
        FROM device_groups g
        LEFT JOIN devices d ON d.group_id = g.id AND d.user_id = g.user_id
        WHERE g.user_id = $1
        GROUP BY g.id, g.name, g.moisture_threshold_percent, g.watering_duration_sec, g.created_at
        ORDER BY g.created_at DESC
        """,
        current_user_id,
    )
    result = [_row_to_group_detail(row) for row in rows]
    logger.info(f"Zwrócono {len(result)} grup dla usera {current_user_id}")
    return result


@router.post(
    "",
    response_model=GroupDetail,
    status_code=status.HTTP_201_CREATED,
    responses={
        400: {"description": "Nazwa grupy nie może być pusta."},
        404: {"description": "Urządzenie nie istnieje."},
        409: {"description": "Grupa o takiej nazwie już istnieje."},
    },
)
async def create_group(
    body: GroupCreate,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(
        f"Próba tworzenia grupy: nazwa='{body.name}', próg={body.moisture_threshold_percent}%, czas={body.watering_duration_sec}s (user {current_user_id})"
    )
    group_name = body.name.strip()
    if not group_name:
        logger.warning(f"Odrzucono tworzenie grupy: nazwa grupy nie może być pusta (user {current_user_id})")
        raise HTTPException(status_code=400, detail="Nazwa grupy nie może być pusta.")

    try:
        async with db.transaction():
            group_id = await db.fetchval(
                """
                INSERT INTO device_groups (user_id, name, moisture_threshold_percent, watering_duration_sec)
                VALUES ($1, $2, $3, $4)
                RETURNING id
                """,
                current_user_id,
                group_name,
                body.moisture_threshold_percent,
                body.watering_duration_sec,
            )

            if body.device_id:
                device = await _fetch_device_row(db, body.device_id, current_user_id)
                if not device:
                    logger.warning(
                        f"Odrzucono tworzenie grupy: urządzenie '{body.device_id}' nie istnieje (user {current_user_id})"
                    )
                    raise HTTPException(status_code=404, detail="Urządzenie nie istnieje.")
                await db.execute(
                    "UPDATE devices SET group_id = $1 WHERE device_id = $2 AND user_id = $3",
                    group_id,
                    body.device_id,
                    current_user_id,
                )
                logger.info(f"Przypisano urządzenie '{body.device_id}' do nowej grupy {group_id}")
                
                # Apply group settings to device and publish commands
                await command_service.send_watering_time_command(
                    user_id=str(current_user_id),
                    device_id=body.device_id,
                    time_ms=body.watering_duration_sec * 1000
                )
                await command_service.send_watering_threshold_command(
                    user_id=str(current_user_id),
                    device_id=body.device_id,
                    threshold_percent=body.moisture_threshold_percent
                )
                logger.info(f"Opublikowano ustawienia grupy do urządzenia {body.device_id}")
                
    except asyncpg.UniqueViolationError:
        logger.warning(f"Odrzucono tworzenie grupy: grupa '{group_name}' już istnieje dla usera {current_user_id}")
        raise HTTPException(status_code=409, detail="Grupa o takiej nazwie już istnieje.")

    row = await _fetch_group_row(db, group_id, current_user_id)
    device_count = 1 if body.device_id else 0
    logger.info(f"Utworzono grupę '{group_name}' (id={group_id}, urządzeń: {device_count}) (user {current_user_id})")
    return _row_to_group_detail(row)


@router.put(
    "/{group_id}",
    response_model=GroupDetail,
    responses={
        404: {"description": "Grupa nie istnieje."},
        400: {"description": "Nazwa grupy nie może być pusta."},
        409: {"description": "Grupa o takiej nazwie już istnieje."},
    },
)
async def update_group(
    group_id: int,
    body: GroupUpdate,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Aktualizacja grupy {group_id} (user {current_user_id}): {body.model_dump(exclude_none=True)}")
    existing = await db.fetchrow(
        "SELECT * FROM device_groups WHERE id = $1 AND user_id = $2",
        group_id,
        current_user_id,
    )
    if not existing:
        logger.warning(f"Nie znaleziono grupy {group_id} dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Grupa nie istnieje.")

    new_name = body.name.strip() if body.name is not None else existing["name"]
    if not new_name:
        logger.warning(
            f"Odrzucono aktualizację grupy {group_id}: nazwa grupy nie może być pusta (user {current_user_id})"
        )
        raise HTTPException(status_code=400, detail="Nazwa grupy nie może być pusta.")

    new_moisture = (
        body.moisture_threshold_percent
        if body.moisture_threshold_percent is not None
        else existing["moisture_threshold_percent"]
    )
    new_duration = (
        body.watering_duration_sec
        if body.watering_duration_sec is not None
        else existing["watering_duration_sec"]
    )

    # Check if settings changed (to determine if we need to publish commands)
    settings_changed = (
        body.moisture_threshold_percent is not None or
        body.watering_duration_sec is not None
    )

    try:
        async with db.transaction():
            await db.execute(
                """
                UPDATE device_groups
                SET name = $1,
                    moisture_threshold_percent = $2,
                    watering_duration_sec = $3
                WHERE id = $4 AND user_id = $5
                """,
                new_name,
                new_moisture,
                new_duration,
                group_id,
                current_user_id,
            )
            
            # If settings changed, apply them to all devices in the group
            if settings_changed:
                await _publish_group_settings_to_devices(
                    db,
                    command_service,
                    group_id,
                    current_user_id,
                    new_moisture,
                    new_duration,
                )
                
    except asyncpg.UniqueViolationError:
        logger.warning(
            f"Odrzucono aktualizację grupy {group_id}: nazwa '{new_name}' już istnieje dla usera {current_user_id}"
        )
        raise HTTPException(status_code=409, detail="Grupa o takiej nazwie już istnieje.")

    row = await _fetch_group_row(db, group_id, current_user_id)
    logger.info(f"Zaktualizowano grupę '{new_name}' (id={group_id}) (user {current_user_id})")
    return _row_to_group_detail(row)


@router.delete(
    "/{group_id}",
    status_code=status.HTTP_204_NO_CONTENT,
    responses={404: {"description": "Grupa nie istnieje."}},
)
async def delete_group(
    group_id: int,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Próba usunięcia grupy {group_id} (user {current_user_id})")
    result = await db.execute(
        "DELETE FROM device_groups WHERE id = $1 AND user_id = $2",
        group_id,
        current_user_id,
    )
    if result == "DELETE 0":
        logger.warning(f"Nie znaleziono grupy {group_id} do usunięcia dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Grupa nie istnieje.")
    logger.info(f"Usunięto grupę {group_id} (user {current_user_id})")


@router.post(
    "/{group_id}/devices/{device_id}",
    response_model=GroupDetail,
    responses={404: {"description": "Grupa nie istnieje."}},
)
async def assign_device_to_group(
    group_id: int,
    device_id: str,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    command_service: Annotated[CommandService, Depends(get_command_service)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Próba przypisania urządzenia '{device_id}' do grupy {group_id} (user {current_user_id})")
    group = await db.fetchrow(
        "SELECT id, moisture_threshold_percent, watering_duration_sec FROM device_groups WHERE id = $1 AND user_id = $2",
        group_id,
        current_user_id,
    )
    if not group:
        logger.warning(f"Nie znaleziono grupy {group_id} (przypisanie urządzenia) dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Grupa nie istnieje.")

    device = await _fetch_device_row(db, device_id, current_user_id)
    if not device:
        logger.warning(f"Nie znaleziono urządzenia '{device_id}' (przypisanie do grupy {group_id}) dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Urządzenie nie istnieje.")

    async with db.transaction():
        await db.execute(
            "UPDATE devices SET group_id = $1 WHERE device_id = $2 AND user_id = $3",
            group_id,
            device_id,
            current_user_id,
        )
        logger.info(f"Przypisano urządzenie '{device_id}' do grupy {group_id} w bazie danych")
        
        # Apply group settings to device and publish commands
        await command_service.send_watering_time_command(
            user_id=str(current_user_id),
            device_id=device_id,
            time_ms=group["watering_duration_sec"] * 1000
        )
        await command_service.send_watering_threshold_command(
            user_id=str(current_user_id),
            device_id=device_id,
            threshold_percent=group["moisture_threshold_percent"]
        )
        
        # Update device settings in database
        await db.execute(
            """
            UPDATE devices 
            SET moisture_threshold_percent = $1, water_duration_sec = $2
            WHERE device_id = $3 AND user_id = $4
            """,
            group["moisture_threshold_percent"],
            group["watering_duration_sec"],
            device_id,
            current_user_id,
        )
        
        logger.info(f"Opublikowano ustawienia grupy do urządzenia {device_id}")
    
    row = await _fetch_group_row(db, group_id, current_user_id)
    logger.info(f"Przypisano urządzenie '{device_id}' do grupy {group_id} (user {current_user_id})")
    return _row_to_group_detail(row)


@router.delete(
    "/{group_id}/devices/{device_id}",
    response_model=GroupDetail,
    responses={404: {"description": "Grupa nie istnieje."}, 409: {"description": "Urządzenie nie jest przypisane do tej grupy."}},
)
async def remove_device_from_group(
    group_id: int,
    device_id: str,
    db: Annotated[asyncpg.Connection, Depends(get_db_session)],
    current_user_id: Annotated[int, Depends(get_current_user_id)],
):
    logger.info(f"Próba usunięcia urządzenia '{device_id}' z grupy {group_id} (user {current_user_id})")
    group = await db.fetchrow(
        "SELECT id FROM device_groups WHERE id = $1 AND user_id = $2",
        group_id,
        current_user_id,
    )
    if not group:
        logger.warning(f"Nie znaleziono grupy {group_id} (usunięcie urządzenia) dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Grupa nie istnieje.")

    device = await _fetch_device_row(db, device_id, current_user_id)
    if not device:
        logger.warning(f"Nie znaleziono urządzenia '{device_id}' (usunięcie z grupy {group_id}) dla usera {current_user_id}")
        raise HTTPException(status_code=404, detail="Urządzenie nie istnieje.")
    if device["group_id"] != group_id:
        logger.warning(f"Urządzenie '{device_id}' nie jest przypisane do grupy {group_id} (user {current_user_id})")
        raise HTTPException(status_code=409, detail="Urządzenie nie jest przypisane do tej grupy.")

    await db.execute(
        "UPDATE devices SET group_id = NULL WHERE device_id = $1 AND user_id = $2",
        device_id,
        current_user_id,
    )
    row = await _fetch_group_row(db, group_id, current_user_id)
    logger.info(f"Usunięto urządzenie '{device_id}' z grupy {group_id} (user {current_user_id})")
    return _row_to_group_detail(row)