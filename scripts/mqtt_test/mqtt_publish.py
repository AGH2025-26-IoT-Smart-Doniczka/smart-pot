import json
import os
import time
from typing import Any

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
UUID = os.environ.get("MQTT_DEVICE_UUID", "AABBCCDDEEFF")
PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "device-password")


def publish(
    client: mqtt_client.Client,
    topic: str,
    payload: dict[str, Any],
) -> None:
    msg_info: mqtt_client.MQTTMessageInfo = client.publish(
        topic,
        json.dumps(payload),
        qos=1,
        retain=False,
    )
    print(f"Message sent with response code: {msg_info.rc.name}")


def main() -> None:
    client = mqtt_client.Client(
        callback_api_version=mqtt_enums.CallbackAPIVersion.VERSION2,
        client_id=UUID,
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(UUID, PASSWORD)
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)

    telemetry = {
        "type": "telemetry",
        "version": 1,
        "data": {
            "potId": "pot-1",
            "airTemp": 23.4,
            "airHumidity": 55.1,
            "airPressure": 1012.3,
            "soilMoisture": 0.42,
            "illuminance": 1200.0,
        },
    }

    setup = {
        "type": "pairing",
        "version": 1,
        "data": {
            "potId": "pot-1",
            "email": "user@example.com",
        },
    }

    publish(client, f"devices/{UUID}/telemetry", telemetry)
    time.sleep(0.5)
    publish(client, f"devices/{UUID}/setup", setup)
    time.sleep(0.5)
    publish(client, "devices/TEST_ID/setup", setup)
    # Returns good response code but ...
    # 1763315902: Denied PUBLISH from AABBCCDDEEFF (d0, q1, r0, m3, 'devices/TEST_ID/setup', ... (90 bytes))
    # Server logs tell us that this publish was denied

    client.loop(2)
    client.disconnect()


if __name__ == "__main__":
    main()
