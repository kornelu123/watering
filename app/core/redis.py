import logging
import ssl
from redis.asyncio import Redis, ConnectionPool
from core.config import settings

logger = logging.getLogger(__name__)

class RedisManager:
    def __init__(self):
        self.pool = None
        self.client = None

    async def connect(self):
        kwargs = {"decode_responses": True}
        if settings.REDIS_URL.startswith("rediss://"):
            kwargs["ssl_cert_reqs"] = "required"
            kwargs["ssl_ca_certs"] = "/app/certs/ca.crt"
            
        self.pool = ConnectionPool.from_url(
            settings.REDIS_URL,
            **kwargs
        )
        self.client = Redis(connection_pool=self.pool)
        
        await self.client.ping()
        logger.info("Połączono z Redis (TLS + hasło) pomyślnie.")

    async def disconnect(self):
        if self.client:
            await self.client.close()
            logger.info("Zamknięto połączenie z Redis.")
            
    def get_client(self) -> Redis:
        if not self.client:
            raise RuntimeError("Redis client is not initialized.")
        return self.client

redis_manager = RedisManager()
