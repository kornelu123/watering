import json
import logging
import asyncpg
from redis.asyncio import Redis
from fastapi import HTTPException, status

logger = logging.getLogger(__name__)

class CommandService:
    def __init__(self, redis_client: Redis, db_session: asyncpg.Connection = None):
        self.redis = redis_client
        self.db = db_session

    async def _verify_device_access(self, user_id: str, device_id: str):
        if not self.db:
            raise RuntimeError("Brak połączenia z bazą danych")
            
        try:
            user_id_int = int(user_id)
        except ValueError:
            raise HTTPException(status_code=400, detail="Nieprawidłowy format user_id (oczekiwano liczby).")
            
        query = "SELECT 1 FROM devices WHERE device_id = $1 AND user_id = $2"
        record = await self.db.fetchrow(query, device_id, user_id_int)
        
        if not record:
            logger.warning(f"Odrzucono komendę: urządzenie {device_id} nie należy do usera {user_id_int}")
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN, 
                detail="Urządzenie nie istnieje lub nie masz do niego dostępu."
            )

    async def send_water_command(self, user_id: str, device_id: str, time_ms: int) -> dict:
        await self._verify_device_access(user_id, device_id)
            
        payload = {
            "device_id": device_id,
            "cmd": "WATER",
            "time_ms": time_ms
        }
        
        await self.redis.publish("commands_channel", json.dumps(payload))
        logger.info(f"Opublikowano komendę z backendu do urządzenia {device_id}: {payload}")
        return payload

    async def send_watering_time_command(self, user_id: str, device_id: str, time_ms: int) -> dict:
        await self._verify_device_access(user_id, device_id)
            
        payload = {
            "device_id": device_id,
            "cmd": "WATERING_TIME",
            "time_ms": time_ms
        }
        
        await self.redis.publish("commands_channel", json.dumps(payload))
        logger.info(f"Opublikowano komendę z backendu do urządzenia {device_id}: {payload}")
        return payload

    async def send_watering_threshold_command(self, user_id: str, device_id: str, threshold_percent: int) -> dict:
        await self._verify_device_access(user_id, device_id)
        
        # Map 0-100% to 0-65535 (uint16_t range)
        threshold_value = int((max(0, min(100, threshold_percent)) / 100.0) * 65535)
            
        payload = {
            "device_id": device_id,
            "cmd": "WATERING_THRESHOLD",
            "threshold": threshold_value
        }
        
        await self.redis.publish("commands_channel", json.dumps(payload))
        logger.info(f"Opublikowano komendę z backendu do urządzenia {device_id}: {payload}")
        return payload

    async def send_reboot_command(self, user_id: str, device_id: str) -> dict:
        await self._verify_device_access(user_id, device_id)
            
        payload = {
            "device_id": device_id,
            "cmd": "REBOOT"
        }
        
        await self.redis.publish("commands_channel", json.dumps(payload))
        logger.info(f"Opublikowano komendę z backendu do urządzenia {device_id}: {payload}")
        return payload
