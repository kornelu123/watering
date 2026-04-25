import logging
import time
from schemas.telemetry import TelemetryCreate

logger = logging.getLogger(__name__)

class TelemetryService:
    def __init__(self, db_session=None):
        self.db = db_session

    async def save_telemetry(self, telemetry: TelemetryCreate) -> dict:
        mock_user_id = "user_123" 
        current_timestamp = int(time.time())
        
        logger.info(f"MOCK DB Zapis: urządzenie '{telemetry.device_id}' "
                    f"(user_id: {mock_user_id}), dane: {telemetry.model_dump()}")
        
        return {
            "id": 1,
            "user_id": mock_user_id,
            "timestamp": current_timestamp,
            **telemetry.model_dump()
        }
