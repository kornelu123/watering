from typing import AsyncGenerator
from fastapi import Depends, Security, HTTPException, status
from fastapi.security import APIKeyHeader
from redis.asyncio import Redis

from core.config import settings
import asyncpg
from core.database import db_manager
from core.redis import redis_manager
from services.telemetry_service import TelemetryService
from services.command_service import CommandService

api_key_header = APIKeyHeader(name="X-API-Key")

def verify_api_key(api_key: str = Security(api_key_header)):
    if api_key != settings.API_KEY:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid API Key"
        )
    return api_key

async def get_redis_client() -> AsyncGenerator[Redis, None]:
    client = redis_manager.get_client()
    yield client

async def get_db_session() -> AsyncGenerator[asyncpg.Connection, None]:
    if db_manager.pool is None:
        raise RuntimeError("Pula bazy danych nie została zainicjalizowana.")
    async with db_manager.pool.acquire() as connection:
        yield connection

def get_telemetry_service(db_session=Depends(get_db_session)) -> TelemetryService:
    return TelemetryService(db_session=db_session)

def get_command_service(
    redis_client: Redis = Depends(get_redis_client),
    db_session: asyncpg.Connection = Depends(get_db_session)
) -> CommandService:
    return CommandService(redis_client=redis_client, db_session=db_session)
