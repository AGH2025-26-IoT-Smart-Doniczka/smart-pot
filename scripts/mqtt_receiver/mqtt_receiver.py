import json
import os
from datetime import datetime, timezone
from typing import Any

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties, reasoncodes

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
USERNAME = os.environ.get("MQTT_USER", "mqtt-receiver")
PASSWORD = os.environ.get("MQTT_PASSWORD", "mqtt-receiver-password")


def on_connect(
    client: mqtt_client.Client,
    _userdata: Any,  # noqa: ANN401
    _flags: mqtt_client.ConnectFlags,
    reason_code: reasoncodes.ReasonCode,
    _props: properties.Properties | None,
) -> None:
    print(
        f"Connected with result code {reason_code.packetType} "
        f"({reason_code.getName()})",
    )
    client.subscribe("#", qos=1)


def _format_payload(payload: bytes) -> str:
    text = payload.decode(errors="replace")
    try:
        parsed = json.loads(text)
    except json.JSONDecodeError:
        return text
    return json.dumps(parsed, indent=2, ensure_ascii=True)


def message_callback(
    _client: mqtt_client.Client,
    _userdata: Any,  # noqa: ANN401
    message: mqtt_client.MQTTMessage,
) -> None:
    if message.topic.startswith("$SYS/"):
        return
    timestamp = datetime.now(timezone.utc).isoformat()
    print(f"{timestamp} {message.topic}")
    print(_format_payload(message.payload))


def main() -> None:
    client = mqtt_client.Client(
        callback_api_version=enums.CallbackAPIVersion.VERSION2,
        client_id=USERNAME,
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_message = message_callback
    connect_props = properties.Properties(properties.PacketTypes.CONNECT)
    connect_props.SessionExpiryInterval = 0xFFFFFFFF
    client.connect(
        BROKER_HOST,
        BROKER_PORT,
        keepalive=60,
        clean_start=False,  # False -> Read queued msg, True -> Ignore queued msg
        properties=connect_props,
    )
    client.loop_forever()


if __name__ == "__main__":
    main()
