# Smart Pot

This repository contains multiple parts of our Smart Pot project.
Everything is still in **development** and not ready for production use.

## MQTT Server

This server uses the [Eclipse Mosquitto](https://hub.docker.com/_/eclipse-mosquitto) Docker image.

It is configured to have three topics per ESP device:

1. ESP $\rightarrow$ Server
   1. `devices/<uuid>/telemetry`

        This topic is used to transfer sensor telemetry data to the server.

   2. `devices/<uuid>/setup`

        This topic is used to send setup config to the server. Setup configuration can include, for example, a request to pair the device with a client account.

2. Server $\rightarrow$ ESP
   1. `devices/<uuid>/config`

        This topic is used to remotely configure the device. The ESP changes its internal configuration based on values sent to this topic.
