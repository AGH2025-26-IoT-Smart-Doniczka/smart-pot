import json
import os
from typing import Any

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
UUID = os.environ.get("MQTT_DEVICE_UUID", "backend")
PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "backend-password")


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

    payload = {
        "username": "AABBCCDDEEFF",
        "password": "device-password",
    }

    publish(client, "users/add", payload)

    client.loop(2)
    client.disconnect()


if __name__ == "__main__":
    main()
