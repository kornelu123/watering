import logging
import asyncpg
from core.config import settings

logger = logging.getLogger(__name__)

class DatabaseManager:
    def __init__(self):
        self.pool = None

    async def connect(self):
        try:
            self.pool = await asyncpg.create_pool(
                settings.DATABASE_URL,
                min_size=2,
                max_size=10
            )
            logger.info("Połączono z bazą PostgreSQL (TimescaleDB) pomyślnie.")
        except Exception as e:
            logger.error(f"Błąd łączenia z bazą danych: {e}")
            raise e

    async def disconnect(self):
        if self.pool:
            await self.pool.close()
            logger.info("Zamknięto pulę połączeń z bazą danych.")

db_manager = DatabaseManager()
