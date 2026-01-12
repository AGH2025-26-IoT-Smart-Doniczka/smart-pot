FROM python:3.13

WORKDIR /app

COPY infra/backend/requirements.txt ./requirements.txt
RUN pip install --no-cache-dir -r requirements.txt

COPY backend ./backend
COPY infra/backend/init_database ./init_database
COPY infra/backend/start.sh ./start.sh

RUN chmod +x start.sh

ENV PYTHONPATH="/app/backend:${PYTHONPATH}"

CMD ["./start.sh"]