# Mosquitto

## Starting up the server

```bash
docker-compose up mosquitto
```

This launches the MQTT server with exposed port `1883` and starts the agent
inside the same container.

There are three development accounts in the configuration files:

- Device
  - Login: `AABBCCDDEEFF`
  - Password: `device-password`
- Backend
  - Login: `backend`
  - Password: `backend-password`
- MQTT Agent (for dynamic user creation)
  - Login: `mqtt-agent`
  - Password: `mqtt-agent-password`

## Adding password manually

```bash
mosquitto_passwd -b ./mosquitto/config/dev_passwd <login> <password>
```

This command will add a new user to the `./config/dev_passwd` file.

> [!NOTE]
> `<login>` should be equal to the normalized MAC address.  
> Example: `AA:BB:CC:DD:EE:FF` = `AABBCCDDEEFF`

## Adding user dynamically

Send the payload below to topic `users/add`

```json
{
    "username": "<login>",
    "password": "<password>"
}
```

This will also add a new user to the `./config/dev_passwd` file.
