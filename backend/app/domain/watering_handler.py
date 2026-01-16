import logging
from queue import Queue
from typing import Tuple

from app.schemas.mqtt.pots import (
    WateringStatusMqttResponse,
)
from app.integrations.repositories.pots import watering_update


logger = logging.getLogger(__name__)

WateringEvent = Tuple[str, dict]

watering_status_queue: Queue[WateringEvent] = Queue()


def watering_status_handler(topic: str, payload: dict) -> None:
    """
    MQTT ingress handler.
    MUST be fast and non-blocking.
    """
    logger.info("ingress watering status enqueued", extra={"topic": topic})
    watering_status_queue.put((topic, payload))


def watering_status_worker() -> None:
    while True:
        topic, payload = watering_status_queue.get()

        data = WateringStatusMqttResponse(**payload)
        pot_id = topic.split("/")[1]

        is_watering = bool(data.water)  # water = 1 -> is_watering = True

        logger.info(
            "watering status received",
            extra={
                "pot_id": pot_id,
                "is_watering": is_watering,
            },
        )

        watering_update(pot_id=pot_id, is_watering=is_watering)
