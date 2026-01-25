import os
from decimal import Decimal
from uuid import uuid4

from fastapi import APIRouter, HTTPException, status, Query, Header, Response
from jwt import InvalidTokenError

from ..schemas.pots import (
    WaterPlantRequest,
    PairingRequest,
    ConfigChangeRequest,
    ChangeOwnerRequest,
    PotListResponse,
)
from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import WaterPlantMqttRequest, AddUserRequest
from ..integrations.repositories.pots import (
    pot_exists,
    user_exists,
    pot_has_owner,
    get_pot_owner_username,
    insert_connection,
    get_history_measures,
    update_config,
    update_owner_connection,
    get_user_pots,
    delete_owner_connection,
    user_has_write_role,
)
from ..domain.hard_reset_handler import wait_for_hard_reset
from ..utils.jwt_token import decode_access_token
from ..utils.mqtt_password import get_new_mqtt_password

router = APIRouter()


def json_safe(obj):
    if isinstance(obj, Decimal):
        return float(obj)
    if isinstance(obj, dict):
        return {k: json_safe(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [json_safe(v) for v in obj]
    return obj


def get_user_id_from_auth(authorization: str | None) -> str:
    if not authorization:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Missing Authorization header",
        )

    if not authorization.startswith("Bearer "):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid Authorization header",
        )

    token = authorization.split(" ", 1)[1].strip()
    if not token:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid Authorization header",
        )

    try:
        payload = decode_access_token(token)
    except InvalidTokenError:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid token",
        )

    user_id = payload.get("sub")
    if not user_id:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid token",
        )

    return user_id


@router.get("", status_code=status.HTTP_200_OK)
def list_user_pots(authorization: str | None = Header(default=None)):
    user_id = get_user_id_from_auth(authorization)

    pots = get_user_pots(user_id)

    ILLUMINANCE_REVERSE = {
        0: "low",
        1: "medium",
        2: "high",
    }

    mapped = []
    for pot in pots:
        cfg = pot.get("config", {})
        mapped.append(
            {
                **pot,
                "config": {
                    "pot_name": cfg.get("pot_name") or pot.get("name") or pot.get("pot_id"),
                    "measure_interval_sec": cfg.get("measure_interval_sec") or 0,
                    "send_interval_sec": cfg.get("send_interval_sec") or 0,
                    "watering_interval_sec": cfg.get("watering_interval_sec"),
                    "max_temp": cfg.get("max_temp"),
                    "min_temp": cfg.get("min_temp"),
                    "min_moisture": cfg.get("min_moisture") or 0,
                    "max_moisture": cfg.get("max_moisture") or 0,
                    "illuminance": ILLUMINANCE_REVERSE.get(cfg.get("illuminance"), "medium"),
                },
            }
        )

    print(mapped)
    response = PotListResponse(pots=mapped)
    return response.model_dump()


@router.delete("/{pot_id}/pairing", status_code=status.HTTP_204_NO_CONTENT)
def unpair_pot(pot_id: str, authorization: str | None = Header(default=None)):
    user_id = get_user_id_from_auth(authorization)

    result = delete_owner_connection(pot_id=pot_id, user_id=user_id)
    if result == "forbidden":
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="User is not allowed to unpair this pot",
        )
    if result == "not_found":
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Pot not found or not paired",
        )

    return Response(status_code=status.HTTP_204_NO_CONTENT)


@router.post("/{pot_id}/pairing", status_code=status.HTTP_201_CREATED)
def pair_plant_with_user(pot_id: str, data: PairingRequest):
    has_owner = pot_has_owner(pot_id)
    mqtt_password = get_new_mqtt_password()
    result = {"role": "owner", "mqtt": {"username": pot_id, "password": mqtt_password}}
    print(f"Pot {pot_id} has owner: {has_owner}")
    if has_owner is not None and has_owner[0] == data.user_id:
        return result

    try:
        insert_connection(pot_id, data.user_id, has_owner)
    except ValueError as ve:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=str(ve))

    if has_owner is not None:
        result["mqtt"]["password"] = ""
        result["role"] = "user"
        return JSONResponse(result, status_code=status.HTTP_200_OK)

    result["mqtt"]["password"] = mqtt_password
    return JSONResponse(result, status_code=status.HTTP_200_OK)


@router.get("/{pot_id}/measures")
def get_measures(pot_id: str, count: int = Query(10, ge=1, le=100)):
    try:
        measures = get_history_measures(pot_id, count)
    except ValueError as ve:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=str(ve))
    return {"pot_id": pot_id, "count": count, "measures": measures}


@router.post("/{pot_id}/actions/water", status_code=status.HTTP_202_ACCEPTED)
def water_plant(pot_id: str, data: WaterPlantRequest):
    if data.duration <= 0:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Duration must be positive",
        )

    if not pot_exists(pot_id):
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Pot not found",
        )

    client_id = f"backend-water-{uuid4().hex[:8]}"
    mqtt_client = MQTTClient(client_id=client_id, persistent_session=False)
    mqtt_client.connect()

    try:
        topic = f"devices/{pot_id}/watering/cmd"
        payload = WaterPlantMqttRequest(dur=data.duration).model_dump()
        mqtt_client.publish(
            topic,
            payload,
            qos=1,
        )
    finally:
        mqtt_client.disconnect()

    return {"message": "Watering queued"}


@router.post("/{pot_id}/actions/config", status_code=status.HTTP_202_ACCEPTED)
def config_change(
    pot_id: str,
    data: ConfigChangeRequest,
    authorization: str | None = Header(default=None),
):
    user_id = get_user_id_from_auth(authorization)
    if not user_has_write_role(pot_id, user_id):
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="User is not allowed to change configuration",
        )

    ILLUMINANCE_MAP = {
        "low": 0,
        "medium": 1,
        "high": 2,
    }

    payload = data.model_dump()

    payload["illuminance"] = ILLUMINANCE_MAP[payload["illuminance"]]

    try:
        updated = update_config(pot_id, payload)
    except ValueError as ve:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(ve),
        )

    if not updated:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail="Pot not found",
        )

    client_id = f"backend-config-{uuid4().hex[:8]}"

    new_config = json_safe(
        {
            "lux": updated["illuminance_type"],
            "moi": [
                updated["min_moisture"],
                updated["max_moisture"],
            ],
            "tem": [
                updated["min_temperature"],
                updated["max_temperature"],
            ],
            "mes": updated["measure_interval_sec"],
            "sen": updated["send_interval_sec"],
            "wat": updated["watering_interval_sec"],
        }
    )

    mqtt_client = MQTTClient(client_id=client_id, persistent_session=False)
    mqtt_client.connect()

    try:
        topic = f"devices/{pot_id}/config/cmd"
        mqtt_client.publish(topic, new_config, qos=1, retain=True)
    finally:
        mqtt_client.disconnect()

    return {
        "pot_id": pot_id,
        "newConfig": new_config,
    }


@router.post("/{pot_id}/permissions/change-owner", status_code=status.HTTP_202_ACCEPTED)
def change_owner(pot_id: str, data: dict):
    new_user_id: str = data["user_id"]

    fail = {"changed": False, "reason": "", "owner": get_pot_owner_username(pot_id)}

    if not pot_exists(pot_id):
        fail["reason"] = "Pot not found"
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=fail)
    if not user_exists(new_user_id):
        fail["reason"] = "User not found"
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail=fail)

    if not wait_for_hard_reset(pot_id, timeout=180):
        fail["reason"] = "Hard reset not received from device"
        raise HTTPException(status_code=status.HTTP_408_REQUEST_TIMEOUT, detail=fail)

    try:
        update_owner_connection(pot_id=pot_id, new_owner_id=new_user_id)
    except SystemError as e:
        fail["reason"] = "Database error during owner change"
        raise HTTPException(status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail=fail)

    return {"changed": True, "newOwner": get_pot_owner_username(pot_id)}
