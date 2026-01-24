import 'dart:async';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'ble_types.dart';

class _FlutterBlueBleDevice implements BleDevice {
  _FlutterBlueBleDevice(this._device);

  final BluetoothDevice _device;

  @override
  String get id => _device.remoteId.str;

  @override
  String get name => _device.platformName.isNotEmpty
      ? _device.platformName
      : _device.remoteId.str;

  @override
  Future<void> connect() async {
    await _device.connect(autoConnect: false, license: License.free);
  }

  @override
  Future<void> disconnect() async {
    await _device.disconnect();
  }

  @override
  Future<bool> get isConnected async => _device.isConnected;

  @override
  Future<void> createBond() async {
    await _device.createBond();
  }

  @override
  Future<bool> get isBonded async =>
      _device.bondState == BluetoothBondState.bonded;

  @override
  Future<void> writeCharacteristic(
    String serviceUuid,
    String characteristicUuid,
    List<int> value,
  ) async {
    final services = await _device.discoverServices();
    final service = services.firstWhere(
      (s) => s.uuid.toString() == serviceUuid,
      orElse: () => throw Exception(
        'Nie znaleziono serwisu konfiguracyjnego na urządzeniu',
      ),
    );

    final characteristic = service.characteristics.firstWhere(
      (c) => c.uuid.toString() == characteristicUuid,
      orElse: () => throw Exception(
        'Błąd zapisu charakterystyki: $characteristicUuid',
      ),
    );

    await characteristic.write(value);
  }
}

class _FlutterBlueBleAdapter implements BleAdapter {
  final Stream<List<BleScanResult>> _scanResults = FlutterBluePlus.scanResults
      .map(
        (results) => results
            .map(
              (res) => BleScanResult(
                device: _FlutterBlueBleDevice(res.device),
                rssi: res.rssi,
                advName: res.advertisementData.advName,
              ),
            )
            .toList(),
      );

  @override
  bool get isSupported => true;

  @override
  Stream<List<BleScanResult>> get scanResults => _scanResults;

  @override
  Future<void> startScan({Duration timeout = const Duration(seconds: 15)}) async {
    await FlutterBluePlus.startScan(timeout: timeout);
  }

  @override
  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
  }

  @override
  Future<List<BleDevice>> getConnectedDevices() async {
    final connected = FlutterBluePlus.connectedDevices;
    return connected.map((d) => _FlutterBlueBleDevice(d)).toList();
  }

  @override
  Future<List<BleDevice>> getBondedDevices() async {
    final bonded = await FlutterBluePlus.bondedDevices;
    return bonded.map((d) => _FlutterBlueBleDevice(d)).toList();
  }

  @override
  Future<BleDevice?> requestDevice({List<String> serviceUuids = const []}) async {
    return null;
  }
}

BleAdapter createBleAdapter() => _FlutterBlueBleAdapter();
