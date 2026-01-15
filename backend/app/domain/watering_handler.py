from queue import Queue
from typing import Tuple

from app.schemas.mqtt.pots import (
    WateringStatusMqttResponse,
)
from app.integrations.repositories.pots import watering_update


WateringEvent = Tuple[str, dict]

watering_status_queue: Queue[WateringEvent] = Queue()


def watering_status_handler(topic: str, payload: dict) -> None:
    """
    MQTT ingress handler.
    MUST be fast and non-blocking.
    """
    watering_status_queue.put((topic, payload))


def watering_status_worker() -> None:
    while True:
        topic, payload = watering_status_queue.get()

        data = WateringStatusMqttResponse(**payload)
        pot_id = topic.split("/")[1]

        print(
            f"[watering_status_worker] pot={pot_id} "
            f"finished={not data.water}"
        )

        is_watering = bool(data.water)  # water = 1 -> is_watering = True
        print(f"[watering_status_worker] pot={pot_id} is_watering={is_watering}")
        watering_update(pot_id=pot_id, is_watering=is_watering)
