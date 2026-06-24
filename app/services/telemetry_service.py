import logging
from datetime import datetime, timezone, timedelta
import asyncpg
from schemas.telemetry import TelemetryCreate

logger = logging.getLogger(__name__)

_RANGE_TO_VIEW: dict[str, str] = {
    "24h": "telemetry_15m",
    "7d":  "telemetry_1h",
    "30d": "telemetry_1d",
}

_RANGE_MAP: dict[str, timedelta] = {
    "24h": timedelta(hours=24),
    "7d":  timedelta(days=7),
    "30d": timedelta(days=30),
}

_AGG_QUERY = """
    SELECT
        bucket       AS time,
        avg_moisture AS moisture_lvl,
        avg_battery  AS battery_lvl,
        avg_water    AS water_lvl
    FROM {view}
    WHERE device_id = $1 AND bucket >= $2
    ORDER BY time ASC
"""


class TelemetryService:
    def __init__(self, db_session: asyncpg.Connection = None):
        self.db = db_session

    async def save_telemetry(self, telemetry: TelemetryCreate) -> dict:
        if not self.db:
            raise RuntimeError("Brak połączenia z bazą danych")
            
        current_time = datetime.now(timezone.utc)
        
        query = """
            INSERT INTO telemetry (time, device_id, water_lvl, battery_lvl, moisture_lvl, uptime)
            VALUES ($1, $2, $3, $4, $5, $6)
        """
        
        try:
            await self.db.execute(
                query,
                current_time,
                telemetry.device_id,
                telemetry.water_lvl,
                telemetry.battery_lvl,
                telemetry.moisture_lvl,
                telemetry.uptime
            )
            logger.info(f"Zapisano telemetrię z urządzenia {telemetry.device_id} do TimescaleDB.")
        except asyncpg.exceptions.ForeignKeyViolationError:
            logger.warning(f"Odrzucono telemetrię: urządzenie {telemetry.device_id} nie istnieje w bazie.")
            raise ValueError(f"Urządzenie o ID {telemetry.device_id} nie jest zarejestrowane.")
        except Exception as e:
            logger.error(f"Błąd DB podczas zapisu telemetrii: {e}")
            raise e

        return {"status": "ok"}

    async def get_devices_for_user(self, user_id: int) -> list[dict]:
        """Returns devices owned by user, enriched with the latest telemetry reading."""
        logger.info(f"Pobieranie urządzeń dla usera {user_id}")
        query = """
            SELECT
                d.device_id,
                d.name,
                d.water_duration_sec,
                d.created_at,
                d.group_id,
                g.name AS group_name,
                ROUND((t.moisture_lvl / 65535.0) * 100, 1) AS moisture_lvl,
                ROUND((t.battery_lvl  / 65535.0) * 100, 1) AS battery_lvl,
                ROUND((t.water_lvl    / 65535.0) * 100, 1) AS water_lvl,
                t.time AS last_seen
            FROM devices d
            LEFT JOIN device_groups g ON g.id = d.group_id AND g.user_id = d.user_id
            LEFT JOIN LATERAL (
                SELECT moisture_lvl, battery_lvl, water_lvl, time
                FROM telemetry
                WHERE device_id = d.device_id
                ORDER BY time DESC
                LIMIT 1
            ) t ON true
            WHERE d.user_id = $1
            ORDER BY d.created_at DESC
        """
        rows = await self.db.fetch(query, user_id)
        devices = [dict(r) for r in rows]
        logger.info(f"Zwrócono {len(devices)} urządzeń dla usera {user_id}")
        return devices

    async def get_telemetry_history(
        self, device_id: str, user_id: int, range_key: str
    ) -> list[dict]:
        logger.info(
            f"Pobieranie historii telemetrii: urządzenie '{device_id}', "
            f"zakres={range_key} (user {user_id})"
        )

        ownership = await self.db.fetchval(
            "SELECT 1 FROM devices WHERE device_id = $1 AND user_id = $2",
            device_id, user_id,
        )
        if not ownership:
            logger.warning(
                f"Odmowa dostępu do historii telemetrii urządzenia '{device_id}' "
                f"dla usera {user_id}"
            )
            raise PermissionError("Brak dostępu do urządzenia")

        since = datetime.now(timezone.utc) - _RANGE_MAP[range_key]
        view = _RANGE_TO_VIEW[range_key]

        rows = await self.db.fetch(_AGG_QUERY.format(view=view), device_id, since)
        points = [dict(r) for r in rows]

        logger.info(f"Zwrócono {len(points)} rekordów z '{view}' dla urządzenia '{device_id}'")
        return points

