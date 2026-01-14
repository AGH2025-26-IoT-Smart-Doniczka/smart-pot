import os
from decimal import Decimal
from uuid import uuid4

from fastapi import APIRouter, HTTPException, status, Query

from ..schemas.pots import (
    WaterPlantRequest, 
    PairingRequest,
    ConfigChangeRequest,
    ChangeOwnerRequest
)
from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import (
    WaterPlantMqttRequest,
    AddUserRequest
)
from ..integrations.repositories.pots import (
    get_watering_status,
    pot_exists,
    user_exists,
    pot_has_owner,
    get_pot_owner_username,
    insert_connection,
    get_history_measures,
    update_config,
    update_owner_connection
)
from ..domain.hard_reset_handler import wait_for_hard_reset

router = APIRouter()


def json_safe(obj):
    if isinstance(obj, Decimal):
        return float(obj)
    if isinstance(obj, dict):
        return {k: json_safe(v) for k, v in obj.items()}
    if isinstance(obj, list):
        return [json_safe(v) for v in obj]
    return obj


# split functionality into smaller functions later ( ﾉ ﾟｰﾟ)ﾉ
@router.post("/{pot_id}/pairing", status_code=status.HTTP_201_CREATED)
def pair_plant_with_user(pot_id: str, data: PairingRequest):
    has_owner = pot_has_owner(pot_id)
    result = {
            "role": "owner",
            "mqtt": {
                "username": pot_id,
                "password": ""
            }
    }
    print(f"Pot {pot_id} has owner: {has_owner}")
    if has_owner is not None and has_owner[0] == data.user_id:
        return result

    try:
        insert_connection(pot_id, data.user_id, has_owner)
    except ValueError as ve:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(ve)
        )

    if has_owner is not None:
        result["role"] = "user"
        return result
    
    mqtt_password = uuid4().hex
    client_id = f"backend-pairing-{uuid4().hex[:8]}"

    mqqt_client = MQTTClient(client_id=client_id, persistent_session=False)
    mqqt_client.connect()

    try:
        topic = "users/add"
        payload = AddUserRequest(
            username=pot_id,
            password=mqtt_password
        ).model_dump()
        print(f"Publishing to topic {topic} payload {payload}")

        mqqt_client.publish(topic, payload, qos=1, retain=False)

    finally:
        mqqt_client.disconnect()

    result["mqtt"]["password"] = mqtt_password
    return result


@router.get("/{pot_id}/measures")
def get_measures(pot_id: str, count: int = Query(10, ge=1, le=100)):
    try:
        measures = get_history_measures(pot_id, count)
    except ValueError as ve:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(ve)
        )
    return {
        "pot_id": pot_id,
        "count": count, 
        "measures" : measures
        }


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
    mqqt_client = MQTTClient(client_id=client_id, persistent_session=False)
    mqqt_client.connect()

    try:
        topic = f"devices/{pot_id}/watering/cmd"
        payload = WaterPlantMqttRequest(dur=data.duration).model_dump()
        mqqt_client.publish(
            topic,
            payload,
            qos=1,
        )
    finally:
        mqqt_client.disconnect()

    return {"message": "Watering queued"}


@router.get("/{pot_id}/actions/water/status", status_code=status.HTTP_200_OK)
def water_status(pot_id: str):
    try:
        is_watering = get_watering_status(pot_id)
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Cannot fetch watering status: {e}"
        )

    if is_watering:
        return {"is_watering": True}
    else:
        return {"is_watering": False}


@router.post("/{pot_id}/actions/config", status_code=status.HTTP_202_ACCEPTED)
def config_change(pot_id: str, data: ConfigChangeRequest):
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

    new_config = json_safe({
        "lux": updated["illuminance_type"],
        "moi": [
            updated["humidity_thresholds"]["very_low"],
            updated["humidity_thresholds"]["low"],
            updated["humidity_thresholds"]["high"],
            updated["humidity_thresholds"]["very_high"],
        ],
        "tem": [
            updated["min_temperature"],
            updated["max_temperature"],
        ],
        "sle": updated["measure_interval_sec"],
    })

    mqtt_client = MQTTClient(client_id=client_id, persistent_session=False)
    mqtt_client.connect()

    try:
        topic = f"devices/{pot_id}/config/cmd"
        mqtt_client.publish(topic, new_config, qos=1)
    finally:
        mqtt_client.disconnect()

    return {
        "pot_id": pot_id,
        "newConfig": new_config,
    }


@router.post("/{pot_id}/permissions/change-owner", status_code=status.HTTP_202_ACCEPTED)
def change_owner(pot_id: str, data: dict):
    new_user_id: str = data["user_id"]

    fail = {
        "changed": False,
        "reason": "",
        "owner": get_pot_owner_username(pot_id)
    }

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

    return {
		"changed": True,
		"newOwner": get_pot_owner_username(pot_id)
    }