import 'ble_types.dart';

class _UnsupportedBleAdapter implements BleAdapter {
  @override
  bool get isSupported => false;

  @override
  Stream<List<BleScanResult>> get scanResults => const Stream.empty();

  @override
  Future<void> startScan({Duration timeout = const Duration(seconds: 10)}) async {
    throw UnsupportedError('BLE not supported on this platform');
  }

  @override
  Future<void> stopScan() async {
    throw UnsupportedError('BLE not supported on this platform');
  }

  @override
  Future<List<BleDevice>> getConnectedDevices() async => [];

  @override
  Future<List<BleDevice>> getBondedDevices() async => [];

  @override
  Future<BleDevice?> requestDevice({List<String> serviceUuids = const []}) async {
    return null;
  }
}

BleAdapter createBleAdapter() => _UnsupportedBleAdapter();
