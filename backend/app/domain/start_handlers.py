import threading
from app.domain.watering_handler import watering_status_worker
from app.domain.telemetry_handler import telemetry_worker

def start_workers() -> None:
    threading.Thread(
        target=watering_status_worker,
        daemon=True,
    ).start()

    threading.Thread(
        target=telemetry_worker,
        daemon=True,
    ).start()