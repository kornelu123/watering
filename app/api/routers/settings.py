import logging
from fastapi import APIRouter, Depends, status
from pydantic import BaseModel
from api.dependencies import get_db_session
from core.security import get_current_user_id
import asyncpg

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/settings", tags=["Settings"])


class UserSettings(BaseModel):
    battery_threshold_percent: int
    water_level_threshold_percent: int


@router.get("", response_model=UserSettings)
async def get_settings(
    db: asyncpg.Connection = Depends(get_db_session),
    current_user_id: int = Depends(get_current_user_id)
):
    row = await db.fetchrow(
        "SELECT battery_threshold_percent, water_level_threshold_percent FROM user_settings WHERE user_id = $1",
        current_user_id
    )
    if not row:
        logger.info(f"Brak ustawień dla usera {current_user_id}, zwracam domyślne")
        return UserSettings(battery_threshold_percent=20, water_level_threshold_percent=10)

    logger.info(f"Pobrano ustawienia usera {current_user_id}")
    return dict(row)


@router.put("", response_model=UserSettings)
async def update_settings(
    body: UserSettings,
    db: asyncpg.Connection = Depends(get_db_session),
    current_user_id: int = Depends(get_current_user_id)
):
    logger.info(f"Aktualizacja ustawień usera {current_user_id}: {body.model_dump()}")
    row = await db.fetchrow(
        """
        INSERT INTO user_settings (user_id, battery_threshold_percent, water_level_threshold_percent, updated_at)
        VALUES ($1, $2, $3, NOW())
        ON CONFLICT (user_id) DO UPDATE
            SET battery_threshold_percent = EXCLUDED.battery_threshold_percent,
                water_level_threshold_percent = EXCLUDED.water_level_threshold_percent,
                updated_at = NOW()
        RETURNING battery_threshold_percent, water_level_threshold_percent
        """,
        current_user_id, body.battery_threshold_percent, body.water_level_threshold_percent
    )
    logger.info(f"Zapisano ustawienia usera {current_user_id}")
    return dict(row)
