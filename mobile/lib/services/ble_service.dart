import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BleService {
  //Charakterystyki
  // różne UUID
  // różne UUID (byte arrays reversed from firmware definitions)
  static const String SERVICE_UUID = "5c73aa37-a268-40d1-b8d8-7f7a479490a2";
  static const String CHARACTERISTICS_PASS_UUID =
      "456f3c1e-3272-4a0c-9868-cebdd4fc0bee"; //
  static const String CHARACTERISTICS_SSID_UUID =
      "a3c35f05-51e9-4ad1-9b04-827243129788"; //

  static const String CHARACTERISTICS_MQTT_PASS =
      "5befb657-9ba7-4f37-8954-d8fc9ca0346c";

  // config characteristic
  static const String CHARACTERISTICS_CONFIG_UUID =
      '043df643-b3df-1dbd-0547-f926cee23429'; //

  Future<void> writeConfiguration({
    required BluetoothDevice device,
    required String ssid,
    required String wifiPass,
    String? mqttUser,
    String? mqttPass,
  }) async {
    List<BluetoothService> services = await device.discoverServices();
    print(services.map((s) => s.uuid).toList());
    BluetoothService? service;

    try {
      service = services.firstWhere((s) => s.uuid.toString() == SERVICE_UUID);
    } catch (e) {
      throw new Exception(
        "Nie znaleziono serwisu konfiguracyjnego na urządzeniu",
      );
    }

    // Funkcja pomocnicza do zapisu
    Future<void> write(String uuid, String value) async {
      try {
        var char = service!.characteristics.firstWhere(
          (c) => c.uuid.toString() == uuid,
        );
        await char.write(utf8.encode(value));
      } catch (e) {
        throw new Exception("Błąd zapisu charakterystyki: ${uuid} : ${e}");
      }
    }

    await write(CHARACTERISTICS_SSID_UUID, ssid);
    await write(CHARACTERISTICS_PASS_UUID, wifiPass);

    if (mqttPass != null) {
      await write(CHARACTERISTICS_MQTT_PASS, mqttPass);
    }

    Future<void> writeBytes(String uuid, List<int> bytes) async {
      try {
        var char = service!.characteristics.firstWhere(
          (c) => c.uuid.toString() == uuid,
        );
        await char.write(bytes);
      } catch (e) {
        throw new Exception("Błąd zapisu config: ${uuid} : ${e}");
      }
    }

    print("Wpisywanie configu");
    final config = _generateDefaultConfig();
    print("Config bytes: $config");
    await writeBytes(CHARACTERISTICS_CONFIG_UUID, config);
  }

  // metoda wypisująca config "na sztywno"
  List<int> _generateDefaultConfig() {
    int tempLow = 2880; //15 stopni
    int tempHigh = 3032; //30 stopni
    int sleepSec = 60;

    return [
      1, //srednie naswietlenie BYTE 0
      20, // 1 prog wigotnosci (20%) BYTE 1
      40, // 2 prog wilgotnosci (40%) BYTE 2
      60, // 3 prog wilgotnosci (60%) BYTE 3
      80, // 4 prog wilgotnosci (80%) BYTE 4
      0, //buffer
      // Byte 5-6: Temp Low (Little Endian)
      tempLow & 0xFF, (tempLow >> 8) & 0xFF,

      // Byte 7-8: Temp High (Little Endian)
      tempHigh & 0xFF, (tempHigh >> 8) & 0xFF,

      // Byte 9-10: Sleep duration (Little Endian)
      sleepSec & 0xFF, (sleepSec >> 8) & 0xFF,
    ];
  }
}
