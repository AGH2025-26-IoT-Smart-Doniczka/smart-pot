# Backend

Python/FastAPI service that exposes Smart Pot HTTP APIs and MQTT integrations.

## Layout

- `app/main.py` – FastAPI application entry point.
- `app/api/` – HTTP routers grouped by feature (`auth`, `user`, `pots`).
- `app/domain/` – domain models and business logic shared between transports.
- `app/integrations/db/` – PostgreSQL client helpers.
- `app/integrations/repositories/` – persistence adapters used by the APIs.
- `app/integrations/mqtt/` – MQTT client responsible for publishing watering commands.
- `app/utils/` – helpers such as password hashing and JWT creation.

## Local development

```bash
cd backend
python -m venv .venv
source .venv/bin/activate  # Windows: .venv\Scripts\activate
pip install -e .[dev]
uvicorn app.main:app --reload
```

Set the following environment variables (or rely on defaults) before starting the server:

- `DB_HOST`, `DB_NAME`, `DB_USER`, `DB_PASSWORD`
- `MQTT_HOST`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`
- `SECRET_KEY` for JWT signing

## Tests

```bash
pytest
```