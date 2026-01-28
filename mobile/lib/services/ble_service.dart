import 'dart:convert';

import 'package:smart_pot_mobile_app/services/ble_types.dart';
import 'package:smart_pot_mobile_app/services/ble_adapter.dart';

class BleService {
  static const String SERVICE_UUID = "5c73aa37-a268-40d1-b8d8-7f7a479490a2";
  static const String CHARACTERISTICS_PASS_UUID =
      "456f3c1e-3272-4a0c-9868-cebdd4fc0bee";
  static const String CHARACTERISTICS_SSID_UUID =
      "a3c35f05-51e9-4ad1-9b04-827243129788";

  static const String CHARACTERISTICS_MQTT_PASS =
      "5befb657-9ba7-4f37-8954-d8fc9ca0346c";

  static const String CHARACTERISTICS_RESET_UUID =
      "896a511e-7016-4a98-a001-dcc96b403ebd";

  static const String CHARACTERISTICS_CONFIG_UUID =
      '043df643-b3df-1dbd-0547-f926cee23429';

  Future<({String potId, bool isHardReset})> readResetCharacteristic(
    String deviceId,
  ) async {
    final connectedDevices = await bleAdapter.getConnectedDevices();
    BleDevice? device;
    try {
      device = connectedDevices.firstWhere((d) => d.id == deviceId);
    } catch (_) {
      throw Exception("Device $deviceId not connected");
    }

    final value = await device.readCharacteristic(
      SERVICE_UUID,
      CHARACTERISTICS_RESET_UUID,
    );

    if (value.length < 13) {
      throw Exception(
        "Invalid reset characteristic length: ${value.length}. Expected 13.",
      );
    }

    // Bytes 0-11: MAC Address (ASCII)
    final potIdBytes = value.sublist(0, 12);
    final potId = utf8.decode(potIdBytes);

    // Byte 12: Reset flag (0 = Hard Reset, 1 = No Reset)
    final resetFlag = value[12];
    final isHardReset = resetFlag == 0;

    return (potId: potId, isHardReset: isHardReset);
  }

  Future<void> writeConfiguration({
    required BleDevice device,
    required String ssid,
    required String wifiPass,
    String? mqttUser,
    String? mqttPass,
    Map<String, dynamic>? customConfig,
  }) async {
    try {
      await device.writeCharacteristic(
        SERVICE_UUID,
        CHARACTERISTICS_SSID_UUID,
        utf8.encode(ssid),
      );
      await device.writeCharacteristic(
        SERVICE_UUID,
        CHARACTERISTICS_PASS_UUID,
        utf8.encode(wifiPass),
      );

      if (mqttPass != null) {
        await device.writeCharacteristic(
          SERVICE_UUID,
          CHARACTERISTICS_MQTT_PASS,
          utf8.encode(mqttPass),
        );
      }

      List<int> configBytes;
      if (customConfig != null) {
        configBytes = utf8.encode(jsonEncode(customConfig));
      } else {
        configBytes = _generateDefaultConfig();
      }

      await device.writeCharacteristic(
        SERVICE_UUID,
        CHARACTERISTICS_CONFIG_UUID,
        configBytes,
      );
    } catch (e) {
      throw Exception('Błąd zapisu konfiguracji: $e');
    }
  }

  Future<bool> isPotConnected(String potId) async {
    final connectedDevices = await bleAdapter.getConnectedDevices();
    return connectedDevices.any((d) => d.id == potId);
  }

  Future<void> writeCharacteristic(
    String potId,
    String characteristicUuid,
    dynamic data,
  ) async {
    final connectedDevices = await bleAdapter.getConnectedDevices();
    BleDevice? device;
    try {
      device = connectedDevices.firstWhere((d) => d.id == potId);
    } catch (_) {
      throw Exception("Device $potId not connected");
    }

    List<int> bytes;
    if (data is List<int>) {
      bytes = data;
    } else if (data is String) {
      bytes = utf8.encode(data);
    } else {
      bytes = utf8.encode(jsonEncode(data));
    }

    await device.writeCharacteristic(
      SERVICE_UUID,
      characteristicUuid,
      bytes,
    );
  }

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
      tempLow & 0xFF, (tempLow >> 8) & 0xFF,
      tempHigh & 0xFF, (tempHigh >> 8) & 0xFF,
      sleepSec & 0xFF, (sleepSec >> 8) & 0xFF,
    ];
  }
}
