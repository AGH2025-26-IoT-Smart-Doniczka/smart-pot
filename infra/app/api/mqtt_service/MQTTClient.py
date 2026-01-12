import json
import os
from typing import Any, Callable

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties, reasoncodes


class MQTTClient:
    def __init__(self) -> None:
        self.broker_host = os.environ.get("MQTT_HOST", "mosquitto")
        self.broker_port = int(os.environ.get("MQTT_PORT", "1883"))
        self.username = os.environ.get("MQTT_USER", "backend")
        self.password = os.environ.get("MQTT_PASSWORD", "backend-password")
        self.client_id = "AABBCCDDEEFF"

        self.client = mqtt_client.Client(
            callback_api_version=enums.CallbackAPIVersion.VERSION2,
            client_id=self.client_id,
            protocol=mqtt_client.MQTTv5,
        )

        self.client.username_pw_set(self.username, self.password)

        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

        self.handlers: dict[str, Callable[[str, dict], None]] = {}


    def connect(self) -> None:
        self.client.connect(self.broker_host, self.broker_port, keepalive=60)
        self.client.loop_start()

    def disconnect(self) -> None:
        self.client.loop_stop()
        self.client.disconnect()
        print("MQTT disconnected")


    def listen(self) -> None:
        """Blocking listen (use in worker / service)."""
        self.client.loop_forever()

    def subscribe(self, topic: str, handler: Callable[[str, dict], None]) -> None:
        self.handlers[topic] = handler
        self.client.subscribe(topic)
        print(f"Subscribed to {topic}")

    def _on_connect(
        self,
        client: mqtt_client.Client,
        _userdata: Any,
        _flags: mqtt_client.ConnectFlags,
        reason_code: reasoncodes.ReasonCode,
        _props: properties.Properties | None,
    ) -> None:
        print(f"MQTT connected: {reason_code.getName()}")

        for topic in self.handlers:
            client.subscribe(topic)

    def _on_message(
        self,
        _client: mqtt_client.Client,
        _userdata: Any,
        message: mqtt_client.MQTTMessage,
    ) -> None:
        try:
            payload = json.loads(message.payload.decode())
        except json.JSONDecodeError:
            print(f"Invalid JSON on {message.topic}")
            return

        for pattern, handler in self.handlers.items():
            if mqtt_client.topic_matches_sub(pattern, message.topic):
                handler(message.topic, payload)
                return

        print(f"Unhandled topic: {message.topic}")


    def publish(self, topic: str, payload: dict) -> None:
        msg = self.client.publish(topic, json.dumps(payload), qos=1)
        msg.wait_for_publish()
        print(f"Published to {topic} with result: {msg.rc.name}")
