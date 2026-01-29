import threading
from app.domain.logs_handler import logs_worker
from app.domain.telemetry_handler import telemetry_worker

def start_workers() -> None:
    threading.Thread(
        target=telemetry_worker,
        daemon=True,
    ).start()

    threading.Thread(
        target=logs_worker,
        daemon=True,
    ).start()
