import json
import os
from typing import Any

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums


BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))

DEVICE_UUID = os.environ.get("MQTT_DEVICE_UUID", "AABBCCDDEEFF")
DEVICE_PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "device-password")


def publish_status(client: mqtt_client.Client, payload: dict[str, Any]) -> None:
    topic = f"devices/{DEVICE_UUID}/status"
    msg_info = client.publish(topic, json.dumps(payload), qos=1, retain=True)
    msg_info.wait_for_publish()
    print(f"Status sent to {topic} -> {msg_info.rc.name}")


def main() -> None:
    client = mqtt_client.Client(
        callback_api_version=mqtt_enums.CallbackAPIVersion.VERSION2,
        client_id=f"{DEVICE_UUID}-status",
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(DEVICE_UUID, DEVICE_PASSWORD)
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_start()

    try:
        payload = {
            "stat": os.environ.get("MQTT_TEST_STATUS", "in_progress"),
            "fin": int(os.environ.get("MQTT_TEST_STATUS_FIN", "0")),
        }
        publish_status(client, payload)
        client.loop(1)
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
