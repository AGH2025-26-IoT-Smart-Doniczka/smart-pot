#!/bin/bash
set -e

export PYTHONPATH="/app/backend:${PYTHONPATH}"

echo "Initializing database..."
python ./init_database/init_db.py

echo "Starting FastAPI server..."
exec uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
