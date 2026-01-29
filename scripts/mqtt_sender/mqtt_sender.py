import argparse
import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import paho.mqtt.client as mqtt_client
from paho.mqtt import enums, properties

BROKER_HOST = os.environ.get("MQTT_HOST", "localhost")
BROKER_PORT = int(os.environ.get("MQTT_PORT", "1883"))
USERNAME = os.environ.get("MQTT_USER", "mqtt-sender")
PASSWORD = os.environ.get("MQTT_PASSWORD", "mqtt-sender-password")

EVENTS_DIR = Path(__file__).parent / "events"


def _format_payload(payload: bytes) -> str:
    text = payload.decode(errors="replace")
    try:
        parsed = json.loads(text)
    except json.JSONDecodeError:
        return text
    return json.dumps(parsed, indent=2, ensure_ascii=True)


def _log_message(topic: str, payload: bytes) -> None:
    timestamp = datetime.now(timezone.utc).isoformat()
    print(f"{timestamp} {topic}")
    print(_format_payload(payload))


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Publish a single MQTT message.")
    parser.add_argument("--event", help="Name of a preset event in events/<name>.json")
    parser.add_argument("--topic", help="MQTT topic to publish")
    parser.add_argument("--payload", help="Raw JSON string payload")
    parser.add_argument("--qos", type=int, default=1, help="QoS level (default: 1)")
    parser.add_argument(
        "--retain",
        type=str,
        default="false",
        help="Retain flag (true/false, default: false)",
    )
    args = parser.parse_args()

    has_event = args.event is not None
    has_direct = args.topic is not None or args.payload is not None
    if has_event and has_direct:
        parser.error("Use either --event or --topic/--payload, not both.")
    if has_event:
        return args
    if args.topic is None or args.payload is None:
        parser.error("--topic and --payload are required when --event is not used.")
    return args


def _parse_retain(value: str) -> bool:
    normalized = value.strip().lower()
    if normalized in {"true", "1", "yes", "y"}:
        return True
    if normalized in {"false", "0", "no", "n"}:
        return False
    raise ValueError(f"Invalid retain value: {value}")


def _load_event(name: str) -> tuple[str, dict[str, Any]]:
    path = EVENTS_DIR / f"{name}.json"
    if not path.exists():
        raise FileNotFoundError(f"Event file not found: {path}")
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("Event file must contain an object")
    topic = data.get("topic")
    payload = data.get("payload")
    if not isinstance(topic, str):
        raise ValueError("Event file 'topic' must be a string")
    if not isinstance(payload, (dict, str)):
        raise ValueError("Event file 'payload' must be an object or a string")
    return topic, payload


def _load_payload(payload_text: str) -> dict[str, Any]:
    parsed = json.loads(payload_text)
    if not isinstance(parsed, dict):
        raise ValueError("Payload must be a JSON object")
    return parsed


def main() -> None:
    args = _parse_args()

    if args.event:
        topic, payload = _load_event(args.event)
    else:
        topic = args.topic
        payload = _load_payload(args.payload)

    retain = _parse_retain(args.retain)

    client = mqtt_client.Client(
        callback_api_version=enums.CallbackAPIVersion.VERSION2,
        client_id=USERNAME,
        protocol=mqtt_client.MQTTv5,
    )
    client.username_pw_set(USERNAME, PASSWORD)

    connect_props = properties.Properties(properties.PacketTypes.CONNECT)
    connect_props.SessionExpiryInterval = 0xFFFFFFFF
    client.connect(
        BROKER_HOST,
        BROKER_PORT,
        keepalive=60,
        clean_start=True,
        properties=connect_props,
    )

    client.loop_start()
    try:
        payload_text = json.dumps(payload)
        result = client.publish(topic, payload_text, qos=args.qos, retain=retain)
        result.wait_for_publish()
        _log_message(topic, payload_text.encode())
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
