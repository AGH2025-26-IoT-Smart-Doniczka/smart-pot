import 'dart:async';

class BleScanResult {
  BleScanResult({
    required this.device,
    required this.rssi,
    required this.advName,
  });

  final BleDevice device;
  final int rssi;
  final String advName;
}

abstract class BleDevice {
  String get id;
  String get name;

  Future<void> connect();
  Future<void> disconnect();
  Future<bool> get isConnected;

  Future<void> createBond();
  Future<bool> get isBonded;

  Future<void> writeCharacteristic(
    String serviceUuid,
    String characteristicUuid,
    List<int> value,
  );

  Future<List<int>> readCharacteristic(
    String serviceUuid,
    String characteristicUuid,
  );
}

abstract class BleAdapter {
  bool get isSupported;

  Stream<List<BleScanResult>> get scanResults;

  Future<void> startScan({Duration timeout});
  Future<void> stopScan();

  Future<List<BleDevice>> getConnectedDevices();
  Future<List<BleDevice>> getBondedDevices();

  Future<BleDevice?> requestDevice({List<String> serviceUuids});
}
