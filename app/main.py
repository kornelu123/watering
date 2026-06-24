import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from core.redis import redis_manager
from core.database import db_manager
from core.logger import setup_logging
from api.routers import telemetry, devices, auth, settings, groups
from api.routers.telemetry import router_user as telemetry_user_router

setup_logging()
logger = logging.getLogger(__name__)

@asynccontextmanager
async def lifespan(app: FastAPI):
    await redis_manager.connect()
    await db_manager.connect()
    yield
    await redis_manager.disconnect()
    await db_manager.disconnect()

app = FastAPI(
    title="Plant Watering API",
    description="Projekt API do zarządzania urządzeniami podlewania roślin i zbierania danych telemetrycznych.",
    version="0.1.0",
    lifespan=lifespan
)

app.include_router(auth.router, prefix="/api/v1")
app.include_router(telemetry.router, prefix="/api/v1")
app.include_router(telemetry_user_router, prefix="/api/v1")
app.include_router(devices.router, prefix="/api/v1")
app.include_router(groups.router, prefix="/api/v1")
app.include_router(settings.router, prefix="/api/v1")

@app.get("/health", tags=["Health"])
async def health_check():
    return {"status": "ok"}
