
import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BleService{

  //Charakterystyki

  // różne UUID
  static const String SERVICE_UUID = "a2909447-7a7f-d8b8-d140-68a237aa735c";
  static const String CHARACTERISTICS_PASS_UUID = "ee0bfcd4-bdce-6898-0c4a-72321e3c6f45";
  static const String CHARACTERISTICS_SSID_UUID = "88971243-7282-049b-d14a-e951055fc3a3";

  static const String CHARACTERISTICS_USER_ID = "752ff574-058c-4ba3-8310-b6daa639ee4d";
  static const String CHARACTERISTICS_MQTT_USER = "e9c410d0-514f-4a62-a808-de5f71f8e747";
  static const String CHARACTERISTICS_MQTT_PASS = "5befb657-9ba7-4f37-8954-d8fc9ca0346c";

  //config
  static const String CHARACTERISTICS_CONFIG_UUID = '2934e2ce-26f9-4705-bd1d-dfb343f63d04';

  Future<void> writeConfiguration({
    required BluetoothDevice device,
    required String ssid,
    required String wifiPass,
    required String userId,
    String? mqttUser,
    String? mqttPass,
}) async {
    List<BluetoothService> services = await device.discoverServices();
    BluetoothService? service;

    try{
      service = services.firstWhere((s) => s.uuid.toString() == SERVICE_UUID);
    }catch(e){
      throw new Exception("Nie znaleziono serwisu konfiguracyjnego na urządzeniu");
    }

    // Funkcja pomocnicza do zapisu
    Future<void> write(String uuid, String value) async {
      try {
        var char = service!.characteristics.firstWhere((c) =>
        c.uuid.toString() == uuid);
        await char.write(utf8.encode(value));
      } catch (e) {
        throw new Exception("Błąd zapisu charakterystyki: ${uuid} : ${e}");
      }
    }

    await write(CHARACTERISTICS_USER_ID, userId);
    await write(CHARACTERISTICS_SSID_UUID, ssid);
    await write(CHARACTERISTICS_PASS_UUID, wifiPass);

    if(mqttPass != null){
      await write(CHARACTERISTICS_MQTT_PASS, mqttPass);
    }
    if(mqttUser != null){
      await write(CHARACTERISTICS_MQTT_USER, mqttUser);
    }

    Future<void> writeBytes(String uuid, List<int> bytes) async {
      try {
        var char = service!.characteristics.firstWhere((c) => c.uuid.toString() == uuid);
        await char.write(bytes);
      } catch (e) {
        throw new Exception("Błąd zapisu config: ${uuid} : ${e}");
      }
    }
    print("Wpisywanie configu");
    final config = _generateDefaultConfig();
    await writeBytes(CHARACTERISTICS_CONFIG_UUID, config);
  }

  // metoda wypisująca config "na sztywno"
  List<int> _generateDefaultConfig(){
    int tempLow = 2880;   //15 stopni
    int tempHigh = 3032; //30 stopni
    int sleepSec = 60;

    return [
      1, //srednie naswietlenie BYTE 0
      20, // 1 prog wigotnosci (20%) BYTE 1
      40, // 2 prog wilgotnosci (40%) BYTE 2
      60, // 3 prog wilgotnosci (60%) BYTE 3
      80, // 4 prog wilgotnosci (80%) BYTE 4

      // Byte 5-6: Temp Low (Little Endian)
      tempLow & 0xFF, (tempLow >> 8) & 0xFF,

      // Byte 7-8: Temp High (Little Endian)
      tempHigh & 0xFF, (tempHigh >> 8) & 0xFF,

      // Byte 9-10: Sleep duration (Little Endian)
      sleepSec & 0xFF, (sleepSec >> 8) & 0xFF,
    ];
  }
}