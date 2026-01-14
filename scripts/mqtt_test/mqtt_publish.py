import json
import os
import time
from typing import Any

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
UUID = os.environ.get("MQTT_DEVICE_UUID", "TEST_POT_ADD")
PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "70474d24fe464c688066c0d899c27a09")


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
    msg_info.wait_for_publish()
    print(f"Message sent with response code: {msg_info.rc.name}")


def main() -> None:
    client = mqtt_client.Client(
        callback_api_version=mqtt_enums.CallbackAPIVersion.VERSION2,
        client_id=UUID,
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(UUID, PASSWORD)
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_start()

    try:
        telemetry = {
            "timestamp": time.time(),
            "data": {
                "lux": 1650,
                "tem": 50.1,
                "moi": 89,
                "pre": 1474.2
            }
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
        publish(client, f"devices/{UUID}/config/cmd", setup)
        time.sleep(0.5)
        publish(client, "devices/TEST_ID/setup", setup)
        # Returns good response code but ...
        # 1763315902: Denied PUBLISH from AABBCCDDEEFF (d0, q1, r0, m3, 'devices/TEST_ID/setup', ... (90 bytes))
        # Server logs tell us that this publish was denied
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
