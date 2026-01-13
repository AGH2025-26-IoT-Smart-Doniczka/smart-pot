import os
import time
from typing import Any

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties, reasoncodes

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))

DEVICE_UUID = os.environ.get("MQTT_DEVICE_UUID", "AABBCCDDEEFF")
DEVICE_PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "device-password")
SESSION_EXPIRY = int(os.environ.get("MQTT_DEVICE_SESSION_EXPIRY", "86400"))

TOPIC_BASE = f"devices/{DEVICE_UUID}"


def on_connect(
    client: mqtt_client.Client,
    _userdata: Any,  # noqa: ANN401
    flags: mqtt_client.ConnectFlags,
    reason_code: reasoncodes.ReasonCode,
    _props: properties.Properties | None,
) -> None:
    print(
        f"Connected with result code {reason_code.packetType} "
        f"({reason_code.getName()})",
    )
    client.subscribe(f"{TOPIC_BASE}/watering/cmd", qos=1)
    client.subscribe(f"{TOPIC_BASE}/config", qos=1)
    client.subscribe(f"{TOPIC_BASE}/watering/status", qos=1)
    print(f"Session present: {flags.session_present}")


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
        client_id=DEVICE_UUID,
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(DEVICE_UUID, DEVICE_PASSWORD)
    client.on_connect = on_connect
    client.on_message = message_callback
    connect_props = properties.Properties(properties.PacketTypes.CONNECT)
    connect_props.SessionExpiryInterval = SESSION_EXPIRY

    client.connect(
        BROKER_HOST,
        BROKER_PORT,
        keepalive=60,
        clean_start=False,
        properties=connect_props,
    )

    client.loop_start()
    try:
        print("Listening... press Ctrl+C to detach (session stays on broker)")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nDisconnecting but keeping session alive...")
    finally:
        disconnect_props = properties.Properties(properties.PacketTypes.DISCONNECT)
        disconnect_props.SessionExpiryInterval = SESSION_EXPIRY
        client.disconnect(properties=disconnect_props)
        client.loop_stop()


if __name__ == "__main__":
    main()
