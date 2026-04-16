from typing import AsyncGenerator
from fastapi import Depends
from redis.asyncio import Redis

from core.redis import redis_manager
from services.telemetry_service import TelemetryService
from services.command_service import CommandService

async def get_redis_client() -> AsyncGenerator[Redis, None]:
    client = redis_manager.get_client()
    yield client

async def get_db_session():
    yield None

def get_telemetry_service(db_session=Depends(get_db_session)) -> TelemetryService:
    return TelemetryService(db_session=db_session)

def get_command_service(redis_client: Redis = Depends(get_redis_client)) -> CommandService:
    return CommandService(redis_client=redis_client)
