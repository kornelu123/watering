import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from core.redis import redis_manager
from api.routers import telemetry, devices

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@asynccontextmanager
async def lifespan(app: FastAPI):
    await redis_manager.connect()
    yield
    await redis_manager.disconnect()

app = FastAPI(
    title="Plant Watering API",
    description="Projekt API do zarządzania urządzeniami podlewania roślin i zbierania danych telemetrycznych.",
    version="0.1.0",
    lifespan=lifespan
)

app.include_router(telemetry.router, prefix="/api/v1")
app.include_router(devices.router, prefix="/api/v1")

@app.get("/health", tags=["Health"])
async def health_check():
    return {"status": "ok"}
