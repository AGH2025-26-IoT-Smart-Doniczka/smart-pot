import os
from uuid import uuid4

from fastapi import APIRouter, HTTPException, status

from ..schemas.pots import WaterPlantRequest
from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import WaterPlantMqttRequest
from ..integrations.repositories.pots import get_watering_status

router = APIRouter()

@router.post("/{pot_id}/actions/water", status_code=status.HTTP_202_ACCEPTED)
def water_plant(pot_id: str, data: WaterPlantRequest):
    if data.duration <= 0:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Duration must be positive",
        )

    client_id = f"backend-water-{uuid4().hex[:10]}"
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
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Cannot fetch watering status: {e}"
        )

    if is_watering:
        return {"status": "Watering", "finished": False}
    else:
        return {"status": "Finished watering", "finished": True}