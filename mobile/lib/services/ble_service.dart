
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
      try{
        var char = service!.characteristics.firstWhere((c) => c.uuid.toString() == uuid);
        await char.write(utf8.encode(value));
      }catch(e){
        throw new Exception("Błąd zapisu charakterystyki: ${uuid} : ${e}");
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
    }
  }
}