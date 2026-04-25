import logging
from redis.asyncio import Redis, ConnectionPool
from core.config import settings

logger = logging.getLogger(__name__)

class RedisManager:
    def __init__(self):
        self.pool = None
        self.client = None

    async def connect(self):
        self.pool = ConnectionPool.from_url(settings.REDIS_URL, decode_responses=True)
        self.client = Redis(connection_pool=self.pool)
        logger.info("Zainicjowano pulę połączeń z Redis.")

    async def disconnect(self):
        if self.client:
            await self.client.close()
            logger.info("Zamknięto połączenie z Redis.")
            
    def get_client(self) -> Redis:
        if not self.client:
            raise RuntimeError("Redis client is not initialized.")
        return self.client

redis_manager = RedisManager()
