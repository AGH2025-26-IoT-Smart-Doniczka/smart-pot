from queue import Queue
from typing import Tuple
import json
from pydantic import ValidationError

from app.schemas.mqtt.pots import (
    TelemetryMqttMessage
)
from app.integrations.repositories.pots import telemetry_insert


TelemetryEvent = Tuple[str, dict]

telemetry_queue: Queue[TelemetryEvent] = Queue()


def telemetry_handler(topic: str, payload: dict) -> None:
    telemetry_queue.put((topic, payload))

def telemetry_worker() -> None:
    while True:
        topic, payload = telemetry_queue.get()

        try:
            data = TelemetryMqttMessage(**json.loads(payload))
        except ValidationError as e:
            print("Błąd walidacji:", e)
        else:
            print("Parsed timestamp:", data.timestamp)
            print("Pot ID:", data.potId)
            print("Lux:", data.data.lux)
        pot_id = topic.split("/")[1]

        telemetry_insert(
            pot_id=pot_id,
            timestamp=data.timestamp,
            air_temp=data.data.tem,
            air_pressure=data.data.pre,
            soil_moisture=data.data.moi,
            illuminance=data.data.lux
        )

