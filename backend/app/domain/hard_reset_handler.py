import time
from queue import Empty, Queue
from threading import Lock
from typing import Dict


hard_reset_queues: Dict[str, Queue[dict]] = {}
hard_reset_lock = Lock()


def _get_queue(pot_id: str) -> Queue[dict]:
    with hard_reset_lock:
        queue = hard_reset_queues.get(pot_id)
        if queue is None:
            queue = Queue()
            hard_reset_queues[pot_id] = queue
        return queue


def _drain_queue(queue: Queue[dict]) -> None:
    while True:
        try:
            queue.get_nowait()
        except Empty:
            return


def hard_reset_handler(topic: str, payload: dict):
    pot_id = topic.split("/")[1]
    queue = _get_queue(pot_id)
    queue.put({"pot_id": pot_id, "timestamp": payload.get("timestamp")})


def wait_for_hard_reset(pot_id: str, timeout: float = 180.0) -> bool:
    queue = _get_queue(pot_id)
    _drain_queue(queue)  # drop stale events, user interaction is required after API call

    start = time.time()
    while True:
        elapsed = time.time() - start
        if elapsed >= timeout:
            break

        remaining = timeout - elapsed
        try:
            msg = queue.get(timeout=min(1.0, remaining))
            if msg.get("pot_id") == pot_id:
                return True
        except Empty:
            continue
    return False