import json
import os
import threading
from typing import Any, Callable

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties, reasoncodes


MAX_SESSION_EXPIRY = 0xFFFFFFFF


class MQTTClient:
    def __init__(
        self,
        *,
        client_id: str | None = None,
        persistent_session: bool = False,
        session_expiry_interval: int | None = None,
    ) -> None:
        self.broker_host = os.environ.get("MQTT_HOST", "mosquitto")
        self.broker_port = int(os.environ.get("MQTT_PORT", "1883"))
        self.username = os.environ.get("MQTT_USER", "backend")
        self.password = os.environ.get("MQTT_PASSWORD", "backend-password")

        self.client_id = client_id or os.environ.get(
            "MQTT_CLIENT_ID", "backend-service"
        )

        self.persistent_session = persistent_session

        default_expiry = int(
            os.environ.get("MQTT_SESSION_EXPIRY", str(MAX_SESSION_EXPIRY))
        )
        requested_expiry = session_expiry_interval or default_expiry
        self.session_expiry_interval = max(
            0, min(requested_expiry, MAX_SESSION_EXPIRY)
        )

        self.client = mqtt_client.Client(
            callback_api_version=enums.CallbackAPIVersion.VERSION2,
            client_id=self.client_id,
            protocol=mqtt_client.MQTTv5,
        )

        self.client.username_pw_set(self.username, self.password)

        self.client.reconnect_delay_set(min_delay=1, max_delay=30)

        self.connected_event = threading.Event()
        self.disconnected_event = threading.Event()

        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

        self.handlers: dict[str, tuple[Callable[[str, dict], None], int]] = {}

    # ---------------------------------------------------------------------

    def connect(self, timeout: int = 10) -> None:
        connect_props = None
        clean_start = True

        if self.persistent_session:
            connect_props = properties.Properties(
                properties.PacketTypes.CONNECT
            )
            connect_props.SessionExpiryInterval = (
                self.session_expiry_interval
            )
            clean_start = False

        self.client.connect(
            self.broker_host,
            self.broker_port,
            keepalive=60,
            clean_start=clean_start,
            properties=connect_props,
        )
        self.client.loop_start()

        if not self.connected_event.wait(timeout):
            raise TimeoutError("MQTT connect timeout")

    def disconnect(self, *, clear_session: bool = False) -> None:
        disconnect_props = None

        if self.persistent_session:
            disconnect_props = properties.Properties(
                properties.PacketTypes.DISCONNECT
            )
            disconnect_props.SessionExpiryInterval = (
                0 if clear_session else self.session_expiry_interval
            )

        self.client.disconnect(properties=disconnect_props)
        self.client.loop_stop()

    # ---------------------------------------------------------------------

    def subscribe(
        self,
        topic: str,
        handler: Callable[[str, dict], None],
        qos: int = 1,
    ) -> None:
        qos = max(0, min(qos, 2))
        self.handlers[topic] = (handler, qos)

        if self.connected_event.is_set():
            self.client.subscribe(topic, qos=qos)

    def publish(
        self,
        topic: str,
        payload: dict | str,
        qos: int = 1,
        retain: bool = False,
    ) -> None:
        if not self.connected_event.is_set():
            raise RuntimeError("MQTT not connected")

        body = payload if isinstance(payload, str) else json.dumps(payload)

        msg = self.client.publish(
            topic,
            body,
            qos=qos,
            retain=retain,
            properties=None,
        )
        msg.wait_for_publish()

        if msg.rc != mqtt_client.MQTT_ERR_SUCCESS:
            raise RuntimeError(f"Publish failed: {msg.rc}")

    # ---------------------------------------------------------------------
    # callbacks
    # ---------------------------------------------------------------------

    def _on_connect(
        self,
        client: mqtt_client.Client,
        _userdata: Any,
        _flags: mqtt_client.ConnectFlags,
        reason_code: reasoncodes.ReasonCode,
        _props: properties.Properties | None,
    ) -> None:
        print(f"MQTT connected: {reason_code.getName()}")

        if reason_code.is_failure:
            return

        self.connected_event.set()
        self.disconnected_event.clear()

        for topic, (_handler, qos) in self.handlers.items():
            client.subscribe(topic, qos=qos)

    def _on_disconnect(self, client, userdata, reason_code, properties=None, *args):
        print("MQTT disconnected")
        print("Reason code:", reason_code)
        print("Properties:", properties)
        self.connected_event.clear()
        self.disconnected_event.set()

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

        for pattern, (handler, _) in self.handlers.items():
            if mqtt_client.topic_matches_sub(pattern, message.topic):
                handler(message.topic, payload)
                return

        print(f"Unhandled topic: {message.topic}")
