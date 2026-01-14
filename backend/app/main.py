import os
import threading
import asyncio
from contextlib import asynccontextmanager

from fastapi import FastAPI

from app.api.user import router as user_router
from app.api.auth import router as auth_router
from app.api.pots import router as pots_router

from app.integrations.mqtt.MQTTClient import MQTTClient
from app.domain.watering_handler import watering_status_handler
from app.domain.telemetry_handler import telemetry_handler
from app.domain.start_handlers import start_workers
from app.domain.hard_reset_handler import hard_reset_handler

mqtt_client = MQTTClient(
    client_id=os.environ.get("MQTT_CLIENT_ID", "backend-service"),
    persistent_session=True,
    session_expiry_interval=0xFFFFFFFF,
)

@asynccontextmanager
async def lifespan(app: FastAPI):
    print("Starting application...")

    start_workers()

    loop = asyncio.get_running_loop()
    await loop.run_in_executor(None, mqtt_client.connect)

    await loop.run_in_executor(None, lambda: mqtt_client.subscribe("devices/+/watering/status", watering_status_handler, qos=1))
    await loop.run_in_executor(None, lambda: mqtt_client.subscribe("devices/+/telemetry", telemetry_handler, qos=1))
    await loop.run_in_executor(None, lambda: mqtt_client.subscribe("devices/+/hard-reset", hard_reset_handler, qos=1))

    yield

    print("Stopping application...")
    await loop.run_in_executor(None, mqtt_client.disconnect)

app = FastAPI(lifespan=lifespan)

app.include_router(user_router, prefix="/user", tags=["user"])
app.include_router(auth_router, prefix="/auth", tags=["auth"])
app.include_router(pots_router, prefix="/pots", tags=["pots"])

@app.get("/health")
def health():
    return {"status": "ok"}
