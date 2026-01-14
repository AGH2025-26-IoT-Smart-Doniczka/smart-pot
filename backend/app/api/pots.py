import os
from uuid import uuid4

from fastapi import APIRouter, HTTPException, status

from ..schemas.pots import WaterPlantRequest, PairingRequest, AddUserRequest
from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import WaterPlantMqttRequest
from ..integrations.repositories.pots import (
    get_watering_status,
    pot_exists,
    pot_has_owner, 
    insert_connection
)

router = APIRouter()


@router.post("/{pot_id}/pairing", status_code=status.HTTP_201_CREATED)
def pair_plant_with_user(pot_id: str, data: PairingRequest):
    has_owner = pot_has_owner(pot_id)
    print(f"Pot {pot_id} has owner: {has_owner}")
    if has_owner is not None and has_owner[0] == data.user_id:
        return {
            "role": "owner",
            "mqtt": {
                "username": pot_id,
                "password": ""
            }
        }

    try:
        insert_connection(pot_id, data.user_id, has_owner)
    except ValueError as ve:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=str(ve)
        )

    if has_owner is not None:
        return {
            "role": "user",
            "mqtt": {
                "username": pot_id,
                "password": ""
            }
        }

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

    return {
            "role": "owner",
            "mqtt": {
                "username": pot_id,
                "password": mqtt_password
            }
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
        return {"status": "Watering", "finished": False}
    else:
        return {"status": "Finished watering", "finished": True}