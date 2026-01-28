import 'dart:async';
import 'dart:js_util' as js_util;
import 'dart:typed_data';

import 'package:web/web.dart' as web;

import 'ble_types.dart';

class _WebBleDevice implements BleDevice {
  _WebBleDevice(this._device);

  final dynamic _device;
  dynamic _server;

  @override
  String get id => js_util.getProperty(_device, 'id') as String? ?? '';

  @override
  String get name =>
      js_util.getProperty(_device, 'name') as String? ?? id;

  @override
  Future<void> connect() async {
    final gatt = js_util.getProperty(_device, 'gatt');
    _server = await js_util.promiseToFuture(
      js_util.callMethod(gatt, 'connect', const []),
    );
  }

  @override
  Future<void> disconnect() async {
    final gatt = js_util.getProperty(_device, 'gatt');
    js_util.callMethod(gatt, 'disconnect', const []);
  }

  @override
  Future<bool> get isConnected async {
    final gatt = js_util.getProperty(_device, 'gatt');
    final connected = js_util.getProperty(gatt, 'connected');
    return connected == true;
  }

  @override
  Future<void> createBond() async {
    return;
  }

  @override
  Future<bool> get isBonded async => false;

  @override
  Future<void> writeCharacteristic(
    String serviceUuid,
    String characteristicUuid,
    List<int> value,
  ) async {
    if (!await isConnected) {
      await connect();
    }

    final server = _server ?? js_util.getProperty(_device, 'gatt');
    final service = await js_util.promiseToFuture(
      js_util.callMethod(server, 'getPrimaryService', [serviceUuid]),
    );
    final characteristic = await js_util.promiseToFuture(
      js_util.callMethod(service, 'getCharacteristic', [characteristicUuid]),
    );

    final bytes = Uint8List.fromList(value);
    await js_util.promiseToFuture(
      js_util.callMethod(characteristic, 'writeValue', [bytes]),
    );
  }

  @override
  Future<List<int>> readCharacteristic(
    String serviceUuid,
    String characteristicUuid,
  ) async {
    if (!await isConnected) {
      await connect();
    }

    final server = _server ?? js_util.getProperty(_device, 'gatt');
    final service = await js_util.promiseToFuture(
      js_util.callMethod(server, 'getPrimaryService', [serviceUuid]),
    );
    final characteristic = await js_util.promiseToFuture(
      js_util.callMethod(service, 'getCharacteristic', [characteristicUuid]),
    );

    final dataView = await js_util.promiseToFuture(
      js_util.callMethod(characteristic, 'readValue', []),
    );
    
    // Convert DataView to List<int>
    final buffer = js_util.getProperty(dataView, 'buffer');
    final uint8Array = Uint8List.view(buffer);
    return uint8Array.toList();
  }
}

class _WebBleAdapter implements BleAdapter {
  final StreamController<List<BleScanResult>> _scanController =
      StreamController.broadcast();
  final List<BleScanResult> _knownResults = [];

  @override
  bool get isSupported =>
      js_util.hasProperty(web.window.navigator, 'bluetooth');

  @override
  Stream<List<BleScanResult>> get scanResults => _scanController.stream;

  @override
  Future<void> startScan({Duration timeout = const Duration(seconds: 15)}) async {
    return;
  }

  @override
  Future<void> stopScan() async {
    return;
  }

  @override
  Future<List<BleDevice>> getConnectedDevices() async => [];

  @override
  Future<List<BleDevice>> getBondedDevices() async => [];

  @override
  Future<BleDevice?> requestDevice({List<String> serviceUuids = const []}) async {
    if (!isSupported) return null;

    final options = <String, dynamic>{};
    if (serviceUuids.isNotEmpty) {
      options['filters'] = [
        {'services': serviceUuids},
      ];
      options['optionalServices'] = serviceUuids;
    } else {
      options['acceptAllDevices'] = true;
    }

    try {
      final device = await js_util.promiseToFuture(
        js_util.callMethod(
          js_util.getProperty(web.window.navigator, 'bluetooth'),
          'requestDevice',
          [js_util.jsify(options)],
        ),
      );

      if (device == null) return null;

      final wrapped = _WebBleDevice(device);
      final result = BleScanResult(
        device: wrapped,
        rssi: 0,
        advName: wrapped.name,
      );

      _knownResults.removeWhere((r) => r.device.id == wrapped.id);
      _knownResults.add(result);
      _scanController.add(List<BleScanResult>.from(_knownResults));

      return wrapped;
    } catch (_) {
      return null;
    }
  }
}

BleAdapter createBleAdapter() => _WebBleAdapter();
