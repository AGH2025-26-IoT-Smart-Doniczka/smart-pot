"""Utility script that emulates an ESP watering controller over MQTT."""

from __future__ import annotations

import json
import os
import time
from collections import deque
from queue import Empty, Queue
from threading import Event
from typing import Any, Deque, Tuple

import paho.mqtt.client as mqtt_client
import paho.mqtt.enums as mqtt_enums
from paho.mqtt import properties as mqtt_properties

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))

DEVICE_UUID = os.environ.get("MQTT_DEVICE_UUID", "TEST_POT_ADD")
DEVICE_PASSWORD = os.environ.get("MQTT_DEVICE_PASSWORD", "27427f5e3f834b1db8a16bf913e9acdf")

COMMAND_TOPIC = f"devices/{DEVICE_UUID}/watering/cmd"
STATUS_TOPIC = f"devices/{DEVICE_UUID}/watering/status"

WAKE_INTERVAL = int(os.environ.get("MQTT_WAKE_INTERVAL", "10"))
CONNECT_TIMEOUT = int(os.environ.get("MQTT_CONNECT_TIMEOUT", "15"))
SESSION_EXPIRY = int(os.environ.get("MQTT_DEVICE_SESSION_EXPIRY", "86400"))

def publish_status(
    client: mqtt_client.Client,
    connected_event: Event,
    *,
    stat: str,
    fin: int,
) -> bool:
    if not connected_event.wait(CONNECT_TIMEOUT):
        print(f"Publish skipped ({stat}, fin={fin}) - broker offline")
        return False

    payload = json.dumps({"stat": stat, "fin": fin})
    info = client.publish(STATUS_TOPIC, payload, qos=1, retain=False)
    try:
        info.wait_for_publish()
    except RuntimeError as exc:
        print(f"Publish failed ({stat}, fin={fin}): {exc}")
        return False

    if info.rc != mqtt_client.MQTT_ERR_SUCCESS:
        print(f"Publish error ({stat}, fin={fin}) rc={info.rc}")
        return False
    else:
        print(f"Published status ({stat}, fin={fin}) -> rc={info.rc}")
        return True


def main() -> None:
    command_queue: Queue[dict[str, Any]] = Queue()
    pending_status: Deque[Tuple[str, int]] = deque()
    connected_event = Event()

    def flush_pending(reason: str) -> None:
        if not pending_status:
            return

        print(f"[{reason}] Flushing {len(pending_status)} pending status messages...")
        while pending_status:
            stat, fin = pending_status[0]
            if publish_status(
                client,
                connected_event,
                stat=stat,
                fin=fin,
            ):
                pending_status.popleft()
                print(
                    f"[{reason}] Pending status ({stat}, fin={fin}) delivered"
                )
            else:
                print("Broker still unavailable; keeping pending statuses")
                break

    def on_connect(
        client: mqtt_client.Client,
        _userdata: Any,
        _flags: mqtt_client.ConnectFlags,
        _reason_code: mqtt_client.ReasonCode,
        _props: mqtt_client.Properties | None,
    ) -> None:
        print("Connected to MQTT broker, subscribing to command topic")
        client.subscribe(COMMAND_TOPIC, qos=1)
        connected_event.set()
        flush_pending("Connect")

    def on_disconnect(
        _client: mqtt_client.Client,
        _userdata: Any,
        _reason_code: mqtt_client.ReasonCode | int,
        _props: mqtt_client.Properties | None,
        *_extra: Any,
    ) -> None:
        print("Disconnected from broker, waiting for reconnect...")
        connected_event.clear()

    def on_message(
        _client: mqtt_client.Client,
        _userdata: Any,
        message: mqtt_client.MQTTMessage,
    ) -> None:
        if message.topic != COMMAND_TOPIC:
            return

        try:
            payload = json.loads(message.payload.decode("utf-8"))
        except json.JSONDecodeError:
            print(f"Ignoring invalid JSON payload on {message.topic}")
            return

        command_queue.put(payload)
        print(f"Received command payload: {payload}")

    client = mqtt_client.Client(
        callback_api_version=mqtt_enums.CallbackAPIVersion.VERSION2,
        client_id=f"{DEVICE_UUID}-watering-test",
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(DEVICE_UUID, DEVICE_PASSWORD)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.reconnect_delay_set(min_delay=1, max_delay=30)

    connect_props = mqtt_properties.Properties(mqtt_properties.PacketTypes.CONNECT)
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
        if not connected_event.wait(CONNECT_TIMEOUT):
            raise TimeoutError("MQTT broker did not confirm connection")

        print("Session established, starting wake/sleep loop...")

        while True:
            print("\n[Wake] Checking for new commands...")
            handled_any = False

            flush_pending("Wake")

            while True:
                try:
                    payload = command_queue.get_nowait()
                except Empty:
                    break

                handled_any = True

                if "dur" not in payload:
                    print("Command payload missing 'dur' field, skipping")
                    continue

                duration = int(payload["dur"])
                print(f"Command duration: {duration} seconds")

                if not publish_status(
                    client,
                    connected_event,
                    stat="Starting",
                    fin=0,
                ):
                    pending_status.append(("Starting", 0))
                    print("Queued 'Starting' status for retry")

                for second in range(1, duration + 1):
                    print(f"Watering... {second}/{duration} seconds")
                    time.sleep(1)

                if not publish_status(
                    client,
                    connected_event,
                    stat="Finished",
                    fin=1,
                ):
                    pending_status.append(("Finished", 1))
                    print("Queued 'Finished' status for retry")
                print("Command finished; ready for next message")

            if not handled_any:
                print("No new commands this cycle")

            print(f"[Sleep] Going idle for {WAKE_INTERVAL} seconds")
            time.sleep(WAKE_INTERVAL)

    finally:
        client.loop_stop()
        disconnect_props = mqtt_properties.Properties(
            mqtt_properties.PacketTypes.DISCONNECT
        )
        disconnect_props.SessionExpiryInterval = SESSION_EXPIRY
        client.disconnect(properties=disconnect_props)
        print("MQTT client disconnected")


if __name__ == "__main__":
    main()