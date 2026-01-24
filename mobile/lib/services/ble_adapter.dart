import 'ble_types.dart';
import 'ble_adapter_stub.dart'
    if (dart.library.html) 'ble_adapter_web.dart'
    if (dart.library.io) 'ble_adapter_mobile.dart';

final BleAdapter bleAdapter = createBleAdapter();
