from uuid import uuid4

def get_new_mqtt_password(pot_id: str) -> str:
    _ = pot_id
    return uuid4().hex
