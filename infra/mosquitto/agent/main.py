import os
from typing import Any
import subprocess
import json

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums

from paho.mqtt import properties, reasoncodes

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
AGENT_LOGIN = os.environ.get("AGENT_LOGIN", "mqtt-agent")
AGENT_PASSWORD = os.environ.get("AGENT_PASSWORD", "mqtt-agent-password")

ACL_PATH: str = os.environ.get("ACL_PATH", "/mosquitto/config/acl")
PASSWD_PATH: str = os.environ.get("PASSWD_PATH", "/mosquitto/config/dev_passwd")

CONTROL_TOPIC: str = os.environ.get("CONTROL_TOPIC", "users/add")


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
    client.subscribe(CONTROL_TOPIC)


def on_message(
    _client: mqtt_client.Client,
    _userdata: Any,  # noqa: ANN401
    message: mqtt_client.MQTTMessage,
) -> None:
    try:
        data = json.loads(message.payload)
        username = data["username"]
        password = data["password"]

        subprocess.run(
            ["mosquitto_passwd", PASSWD_PATH, username],
            input=f"{password}\n{password}\n",
            text=True,
            check=True,
        )

        subprocess.run(["pkill", "-HUP", "mosquitto"], check=True)
    except Exception as e:
        print(f"Error: {e}")


def main():
    client = mqtt_client.Client(
        callback_api_version=mqtt_enums.CallbackAPIVersion.VERSION2,
        client_id=AGENT_LOGIN,
        protocol=mqtt_client.MQTTv5,
    )
    client.on_message = on_message
    client.on_connect = on_connect
    client.username_pw_set(AGENT_LOGIN, AGENT_PASSWORD)
    client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)
    client.loop_forever()


if __name__ == "__main__":
    main()
