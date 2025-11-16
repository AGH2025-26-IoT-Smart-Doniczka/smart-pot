# Mosquitto

## Starting up the server

```bash
docker-compose up mosquitto
```

This launches the MQTT server with exposed port `1883`.

There are two development accounts in configuration files:

- Device
  - Login: `AABBCCDDEEFF`
  - Password: `device-password`
- Backend
  - Login: `backend`
  - Password: `backend-password`

## Adding password manually

```bash
mosquitto_passwd -b ./mosquitto/config/dev_passwd <login> <password>
```

This command will add new user into the `./config/dev_passwd` file.

> [!NOTE]
> `<login>` should be equal to normalized MAC address.  
> Example: `AA:BB:CC:DD:EE:FF` = `AABBCCDDEEFF`
