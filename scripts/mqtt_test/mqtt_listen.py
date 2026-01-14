import os
from typing import Any

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties, reasoncodes

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
USERNAME = os.environ.get("MQTT_USER", "backend")
PASSWORD = os.environ.get("MQTT_PASSWORD", "backend-password")


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
    client.subscribe("devices/+/telemetry", qos=1)
    client.subscribe("devices/+/setup", qos=1)


def message_callback(
    _client: mqtt_client.Client,
    _userdata: Any,  # noqa: ANN401
    message: mqtt_client.MQTTMessage,
) -> None:
    payload = message.payload.decode(errors="replace")
    print(f"{message.topic}: {payload}")


def main() -> None:
    client = mqtt_client.Client(
        callback_api_version=enums.CallbackAPIVersion.VERSION2,
        client_id="backend",
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(USERNAME, PASSWORD)
    client.on_connect = on_connect
    client.on_message = message_callback
    connect_props = properties.Properties(properties.PacketTypes.CONNECT)
    connect_props.SessionExpiryInterval = 0xFFFFFFFF
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60, clean_start=False, properties=connect_props)
    client.loop_forever()


if __name__ == "__main__":
    main()
