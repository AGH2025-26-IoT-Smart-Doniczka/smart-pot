import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';

class DeviceScanScreen extends StatefulWidget {
  // final Function(BluetoothDevice) onDeviceSelected;
  const DeviceScanScreen({super.key, required this.onDeviceSelected});

  //dodatkowy callback
  final Function(BluetoothDevice) onDeviceSelected;

  @override
  State<DeviceScanScreen> createState() => _DeviceScanScreenState();
}

class _DeviceScanScreenState extends State<DeviceScanScreen> {
  StreamSubscription<List<ScanResult>>? _scanSub;
  bool _permissionsGranted = false;
  bool _permissionPermanentlyDenied = false;

  @override
  void initState() {
    super.initState();

    _scanSub = FlutterBluePlus.scanResults.listen(
      (results) => debugPrint('scanResults update: ${results.length} items'),
      onError: (err) => debugPrint('scanResults error: $err'),
    );

    _ensurePermissionsAndScan();
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    FlutterBluePlus.stopScan();
    super.dispose();
  }

  Future<void> _ensurePermissionsAndScan() async {
    final permissionResult = await _requestPermissions();
    if (!mounted) return;

    setState(() {
      _permissionsGranted = permissionResult.granted;
      _permissionPermanentlyDenied = permissionResult.permanentlyDenied;
    });

    if (permissionResult.granted) {
      await _startScanning();
    }
  }

  Future<({bool granted, bool permanentlyDenied})> _requestPermissions() async {
    final permissions = [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.locationWhenInUse,
    ];

    if (await _needsLegacyBluetoothPermission()) {
      permissions.add(Permission.bluetooth);
    }

    final statuses = await permissions.request();
    final allGranted = statuses.values.every((status) => status.isGranted);
    final permanentlyDenied = statuses.values.any(
      (status) => status.isPermanentlyDenied,
    );

    if (!allGranted) {
      debugPrint('Permissions denied: $statuses');
    }

    return (granted: allGranted, permanentlyDenied: permanentlyDenied);
  }

  Future<void> _startScanning() async {
    try {
      final state = await FlutterBluePlus.adapterState.first;
      debugPrint('Adapter state: $state');
      final isScanning = await FlutterBluePlus.isScanning.first;
      debugPrint('isScanning before start: $isScanning');

      if (isScanning) {
        await FlutterBluePlus.stopScan();
        debugPrint('Existing scan stopped');
      }

      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
      debugPrint('Scan started');
    } catch (e) {
      debugPrint('Error while scanning: $e');
    }
  }

  Future<bool> _needsLegacyBluetoothPermission() async {
    final info = await DeviceInfoPlugin().androidInfo;
    return info.version.sdkInt <= 32; // Android 12L i starsze
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          ElevatedButton.icon(
            onPressed: _ensurePermissionsAndScan,
            label: Text("Scan again"),
            icon: Icon(Icons.refresh),
          ),
          const SizedBox(height: 10),
          Expanded(
            child: !_permissionsGranted
                ? _buildPermissionInfo()
                : StreamBuilder<List<ScanResult>>(
                    stream: FlutterBluePlus.scanResults,
                    initialData: [],
                    builder: (context, snapshot) {
                      final results = List<ScanResult>.from(
                        snapshot.data ?? const [],
                      );
                      debugPrint('builder results count: ${results.length}');

                      results.sort(
                        (a, b) => b.rssi.compareTo(a.rssi),
                      ); //sortowanie po sile sygnału

                      if (results.isEmpty) {
                        return const Center(child: Text("No devices found"));
                      }

                      return ListView.separated(
                        separatorBuilder: (c, i) => const Divider(),
                        itemCount: results.length,
                        itemBuilder: (c, i) {
                          final result = results[i];
                          return _buildDeviceTile(result);
                        },
                      );
                    },
                  ),
          ),
        ],
      ),
      //do Symulacji
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () {
          final fakeDevice = BluetoothDevice(
            remoteId: const DeviceIdentifier("00:00:00:00:00:00"),
          );
          widget.onDeviceSelected(fakeDevice);
        },
        label: const Text("SYMULACJA"),
        icon: const Icon(Icons.bug_report),
        backgroundColor: Colors.orange,
      ),
    );
  }

  Widget _buildDeviceTile(ScanResult res) {
    final advName = res.advertisementData.advName;
    final name = res.device.platformName.isNotEmpty
        ? res.device.platformName
        : advName.isNotEmpty
        ? advName
        : res.device.remoteId.str; // fallback jeśli brak nazwy

    return ListTile(
      title: Text(name, style: const TextStyle(fontWeight: FontWeight.bold)),
      subtitle: Text(res.device.remoteId.str),
      trailing: ElevatedButton(
        onPressed: () {
          widget.onDeviceSelected(res.device);
        },
        child: Text("Połącz"),
      ),
    );
  }

  Widget _buildPermissionInfo() {
    if (_permissionPermanentlyDenied) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Text('Wymagane uprawnienia zostały na stałe odrzucone.'),
            const SizedBox(height: 12),
            ElevatedButton(
              onPressed: openAppSettings,
              child: const Text('Otwórz ustawienia aplikacji'),
            ),
          ],
        ),
      );
    }

    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: const [
          Text('Aby wyszukać urządzenia, zezwól na Bluetooth i lokalizację.'),
          SizedBox(height: 12),
          Text('Naciśnij "Scan again", aby ponownie poprosić o zgody.'),
        ],
      ),
    );
  }
}
