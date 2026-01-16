from uuid import uuid4

from ..integrations.mqtt.MQTTClient import MQTTClient
from ..schemas.mqtt.pots import AddUserRequest

def get_new_mqtt_password(user_id: str, pot_id: str) -> str:
    mqtt_password = uuid4().hex
    client_id = f"backend-pairing-{uuid.uuid4().hex[:8]}"

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

    return mqtt_password