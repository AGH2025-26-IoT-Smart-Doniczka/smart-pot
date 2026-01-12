from fastapi import APIRouter, HTTPException, status

from ..schemas.pots import WaterPlantRequest
from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import WaterPlantMqttRequest

router = APIRouter()

@router.post("/{pot_id}/actions/water", status_code=status.HTTP_202_ACCEPTED)
def water_plant(pot_id: str, data: WaterPlantRequest):
    if data.duration <= 0:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Duration must be positive",
        )
    
    mqqt_client = MQTTClient()
    mqqt_client.connect()
    topic = f"devices/{pot_id}/watering/cmd"
    payload = WaterPlantMqttRequest(dur=data.duration).model_dump_json()

    mqqt_client.publish(topic, payload)
    mqqt_client.disconnect()

    return {"message": "Watering queued"}
