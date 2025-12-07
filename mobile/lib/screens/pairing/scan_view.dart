import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';

class DeviceScanScreen extends StatefulWidget {
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
  List<BluetoothDevice> _connectedDevices = [];
  List<BluetoothDevice> _bondedDevices = [];

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
      await _loadConnectedAndBondedDevices();
      await _startScanning();
    }
  }

  Future<void> _loadConnectedAndBondedDevices() async {
    try {
      final connected = FlutterBluePlus.connectedDevices;
      debugPrint('Connected devices: ${connected.length}');

      final bonded = await FlutterBluePlus.bondedDevices;
      debugPrint('Bonded devices: ${bonded.length}');

      final connectedIds = connected.map((d) => d.remoteId.str).toSet();
      final bondedOnly = bonded
          .where((d) => !connectedIds.contains(d.remoteId.str))
          .toList();

      if (mounted) {
        setState(() {
          _connectedDevices = connected;
          _bondedDevices = bondedOnly;
        });
      }
    } catch (e) {
      debugPrint('Error loading connected/bonded devices: $e');
    }
  }

  Future<({bool granted, bool permanentlyDenied})> _requestPermissions() async {
    final List<Permission> permissions;

    if (await _needsLegacyBluetoothPermission()) {
      permissions = [Permission.bluetooth, Permission.locationWhenInUse];
    } else {
      permissions = [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ];
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
    return info.version.sdkInt < 31;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          ElevatedButton.icon(
            onPressed: _ensurePermissionsAndScan,
            label: Text("Skanuj ponownie"),
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

                      final knownIds = {
                        ..._connectedDevices.map((d) => d.remoteId.str),
                        ..._bondedDevices.map((d) => d.remoteId.str),
                      };
                      final filteredResults = results
                          .where(
                            (r) => !knownIds.contains(r.device.remoteId.str),
                          )
                          .toList();

                      final hasConnected = _connectedDevices.isNotEmpty;
                      final hasBonded = _bondedDevices.isNotEmpty;
                      final hasScanned = filteredResults.isNotEmpty;

                      if (!hasConnected && !hasBonded && !hasScanned) {
                        return const Center(child: Text("No devices found"));
                      }

                      return ListView(
                        children: [
                          if (hasConnected) ...[
                            _buildSectionHeader(
                              "Połączone urządzenia",
                              Icons.bluetooth_connected,
                              Colors.green,
                            ),
                            ..._connectedDevices.map(
                              (device) => _buildKnownDeviceTile(
                                device,
                                isConnected: true,
                              ),
                            ),
                            const Divider(thickness: 2),
                          ],
                          if (hasBonded) ...[
                            _buildSectionHeader(
                              "Sparowane urządzenia",
                              Icons.bluetooth,
                              Colors.blue,
                            ),
                            ..._bondedDevices.map(
                              (device) => _buildKnownDeviceTile(
                                device,
                                isConnected: false,
                              ),
                            ),
                            const Divider(thickness: 2),
                          ],
                          if (hasScanned) ...[
                            _buildSectionHeader(
                              "Wykryte urządzenia",
                              Icons.search,
                              Colors.grey,
                            ),
                            ...filteredResults.map(
                              (result) => _buildDeviceTile(result),
                            ),
                          ],
                        ],
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

  Widget _buildSectionHeader(String title, IconData icon, Color color) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      child: Row(
        children: [
          Icon(icon, color: color, size: 20),
          const SizedBox(width: 8),
          Text(
            title,
            style: TextStyle(
              fontWeight: FontWeight.bold,
              color: color,
              fontSize: 14,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildKnownDeviceTile(
    BluetoothDevice device, {
    required bool isConnected,
  }) {
    final name = device.platformName.isNotEmpty
        ? device.platformName
        : device.remoteId.str;

    return ListTile(
      leading: Icon(
        isConnected ? Icons.bluetooth_connected : Icons.bluetooth,
        color: isConnected ? Colors.green : Colors.blue,
      ),
      title: Text(name, style: const TextStyle(fontWeight: FontWeight.bold)),
      subtitle: Text(device.remoteId.str),
      trailing: ElevatedButton(
        onPressed: () {
          widget.onDeviceSelected(device);
        },
        style: ElevatedButton.styleFrom(
          backgroundColor: isConnected ? Colors.green : null,
        ),
        child: Text(isConnected ? "Użyj" : "Połącz"),
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
