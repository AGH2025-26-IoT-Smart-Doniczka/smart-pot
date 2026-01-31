import json
import logging
from queue import Queue
from typing import Any, Tuple

from pydantic import ValidationError

from app.schemas.mqtt.pots import ConfigChangeMqttRequest, LogsMqttMessage
from app.integrations.repositories.pots import archive_pot, get_pot_row, pot_logs_insert, update_config

LogsEvent = Tuple[str, Any]

logger = logging.getLogger(__name__)

logs_queue: Queue[LogsEvent] = Queue()


def logs_handler(topic: str, payload: dict) -> None:
    logger.info("ingress logs enqueued", extra={"topic": topic})
    logs_queue.put((topic, payload))


def _normalize_payload(payload: Any) -> dict:
    if isinstance(payload, dict):
        return payload

    if isinstance(payload, (bytes, bytearray)):
        payload = payload.decode()

    if isinstance(payload, str):
        return json.loads(payload)

    raise TypeError(f"Unsupported payload type: {type(payload)}")


def logs_worker() -> None:
    while True:
        topic, payload = logs_queue.get()

        try:
            raw_payload = _normalize_payload(payload)
        except (json.JSONDecodeError, UnicodeDecodeError, TypeError) as exc:
            logger.warning("invalid logs payload", extra={"topic": topic, "error": str(exc)})
            continue

        try:
            data = LogsMqttMessage(**raw_payload)
        except ValidationError as e:
            logger.warning("logs validation error", extra={"topic": topic, "error": str(e)})
            continue

        pot_id = topic.split("/")[1]

        if data.lab == "config":
            try:
                raw_cfg = json.loads(data.data)
                cfg = ConfigChangeMqttRequest(**raw_cfg)
            except (json.JSONDecodeError, ValidationError, TypeError) as exc:
                logger.warning("config log invalid", extra={"pot_id": pot_id, "error": str(exc)})
            else:
                current = get_pot_row(pot_id)
                if current is None:
                    logger.warning("config log pot missing", extra={"pot_id": pot_id})
                else:
                    try:
                        update_config(
                            pot_id=pot_id,
                            data={
                                "pot_name": current.get("pot_name"),
                                "max_temp": cfg.tem[1],
                                "min_temp": cfg.tem[0],
                                "min_moisture": cfg.moi[0],
                                "max_moisture": cfg.moi[1],
                                "illuminance": current.get("illuminance_type", "low"),
                                "measure_interval_sec": cfg.mes,
                                "send_interval_sec": cfg.sen,
                                "watering_interval_sec": cfg.wai if cfg.wai and cfg.wai > 0 else None,
                                "watering_duration_sec": cfg.wat if cfg.wat and cfg.wat > 0 else None,
                            },
                        )
                    except Exception as exc:
                        logger.error("config log update failed", extra={"pot_id": pot_id, "error": str(exc)})
                    else:
                        logger.info("config updated from log", extra={"pot_id": pot_id})

        try:
            pot_logs_insert(
                pot_id=pot_id,
                timestamp=data.ts,
                label=data.lab,
                payload={"lvl": data.lvl, "data": data.data},
            )
        except Exception as e:
            logger.error("logs insert failed", extra={"pot_id": pot_id, "error": str(e)})
        else:
            logger.info("logs stored", extra={"pot_id": pot_id, "label": data.lab})

        if data.lab == "factory_reset":
            try:
                archived = archive_pot(pot_id)
            except Exception as e:
                logger.error("archive pot failed", extra={"pot_id": pot_id, "error": str(e)})
            else:
                if archived is None:
                    logger.info("archive pot skipped (already inactive?)", extra={"pot_id": pot_id})
                else:
                    logger.info("pot archived after factory_reset log", extra={"pot_id": pot_id})
