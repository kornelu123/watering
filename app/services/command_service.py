import json
import logging
from redis.asyncio import Redis

logger = logging.getLogger(__name__)

class CommandService:
    def __init__(self, redis_client: Redis):
        self.redis = redis_client

    async def send_water_command(self, user_id: str, device_id: str, duration_sec: int) -> dict:
        logger.info(f"MOCK Autoryzacja SQL: sprawdzam czy device_id={device_id} należy do user_id={user_id}")
        
        payload = {
            "device_id": device_id,
            "cmd": "WATER",
            "duration_sec": duration_sec
        }
        
        await self.redis.publish("commands_channel", json.dumps(payload))
        logger.info(f"Opublikowano komendę z backendu: {payload}")
        
        return payload
