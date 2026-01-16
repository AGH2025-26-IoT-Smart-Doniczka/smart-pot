import json
import logging
from queue import Queue
from typing import Any, Tuple

from pydantic import ValidationError

from app.schemas.mqtt.pots import (
    TelemetryMqttMessage
)
from app.integrations.repositories.pots import measures_insert


TelemetryEvent = Tuple[str, Any]

logger = logging.getLogger(__name__)

telemetry_queue: Queue[TelemetryEvent] = Queue()


def telemetry_handler(topic: str, payload: dict) -> None:
    logger.info("ingress telemetry enqueued", extra={"topic": topic})
    telemetry_queue.put((topic, payload))

def _normalize_payload(payload: Any) -> dict:
    if isinstance(payload, dict):
        return payload

    if isinstance(payload, (bytes, bytearray)):
        payload = payload.decode()

    if isinstance(payload, str):
        return json.loads(payload)

    raise TypeError(f"Unsupported payload type: {type(payload)}")


def telemetry_worker() -> None:
    while True:
        topic, payload = telemetry_queue.get()

        try:
            raw_payload = _normalize_payload(payload)
        except (json.JSONDecodeError, UnicodeDecodeError, TypeError) as exc:
            logger.warning("invalid telemetry payload", extra={"topic": topic, "error": str(exc)})
            continue

        try:
            data = TelemetryMqttMessage(**raw_payload)
        except ValidationError as e:
            logger.warning("telemetry validation error", extra={"topic": topic, "error": str(e)})
            continue

        logger.info(
            "telemetry parsed",
            extra={
                "topic": topic,
                "timestamp": data.timestamp,
                "lux": data.data.lux,
                "moi": data.data.moi,
                "tem": data.data.tem,
                "pre": data.data.pre,
            },
        )

        pot_id = topic.split("/")[1]
        try:
            measures_insert(
                pot_id=pot_id,
                timestamp=data.timestamp,
                air_temp=data.data.tem,
                air_pressure=data.data.pre,
                soil_moisture=data.data.moi,
                illuminance=data.data.lux
            )
        except Exception as e:
            logger.error("telemetry insert failed", extra={"pot_id": pot_id, "error": str(e)})
        else:
            logger.info("telemetry stored", extra={"pot_id": pot_id})
