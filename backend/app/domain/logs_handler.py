import json
import logging
from queue import Queue
from typing import Any, Tuple

from pydantic import ValidationError

from app.schemas.mqtt.pots import LogsMqttMessage
from app.integrations.repositories.pots import pot_logs_insert

LogsEvent = Tuple[str, Any]

logger = logging.getLogger(__name__)

logs_queue: Queue[LogsEvent] = Queue()


def logs_handler(topic: str, payload: dict) -> None:
    logger.info("ingress logs enqueued", extra={"topic": topic})
    logs_queue.put((topic, payload))


def _normalize_payload(payload: Any) -> dict:
    if isinstance(payload, dict):
        return payload

    if isinstance(payload, (bytes, bytearray)):
        payload = payload.decode()

    if isinstance(payload, str):
        return json.loads(payload)

    raise TypeError(f"Unsupported payload type: {type(payload)}")


def logs_worker() -> None:
    while True:
        topic, payload = logs_queue.get()

        try:
            raw_payload = _normalize_payload(payload)
        except (json.JSONDecodeError, UnicodeDecodeError, TypeError) as exc:
            logger.warning("invalid logs payload", extra={"topic": topic, "error": str(exc)})
            continue

        try:
            data = LogsMqttMessage(**raw_payload)
        except ValidationError as e:
            logger.warning("logs validation error", extra={"topic": topic, "error": str(e)})
            continue

        pot_id = topic.split("/")[1]

        try:
            pot_logs_insert(
                pot_id=pot_id,
                timestamp=data.ts,
                label=data.lab,
                payload={"lvl": data.lvl, "data": data.data},
            )
        except Exception as e:
            logger.error("logs insert failed", extra={"pot_id": pot_id, "error": str(e)})
        else:
            logger.info("logs stored", extra={"pot_id": pot_id, "label": data.lab})
