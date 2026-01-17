FROM eclipse-mosquitto:2

RUN if [ -f /etc/alpine-release ]; then \
      apk add --no-cache python3 py3-pip; \
    else \
      apt-get update && \
      apt-get install -y python3 python3-pip python3-venv && \
      rm -rf /var/lib/apt/lists/*; \
    fi

RUN python3 -m venv /opt/venv && \
    /opt/venv/bin/pip install --no-cache-dir "paho-mqtt>=2.1.0"

COPY ./agent /mosquitto/agent
COPY ./entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
