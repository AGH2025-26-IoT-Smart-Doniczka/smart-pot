import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:device_info_plus/device_info_plus.dart';
import 'package:smart_pot_mobile_app/services/ble_adapter.dart';
import 'package:smart_pot_mobile_app/services/ble_service.dart';
import 'package:smart_pot_mobile_app/services/ble_types.dart';

class DeviceScanScreen extends StatefulWidget {
  const DeviceScanScreen({super.key, required this.onDeviceSelected});

  final Function(BleDevice) onDeviceSelected;

  @override
  State<DeviceScanScreen> createState() => _DeviceScanScreenState();
}

class _DeviceScanScreenState extends State<DeviceScanScreen> {
  final BleAdapter _adapter = bleAdapter;

  StreamSubscription<List<BleScanResult>>? _scanSub;
  bool _permissionsGranted = false;
  bool _permissionPermanentlyDenied = false;
  bool _isSupported = true;

  List<BleDevice> _connectedDevices = [];
  List<BleDevice> _bondedDevices = [];
  List<BleDevice> _webDevices = [];

  @override
  void initState() {
    super.initState();

    _isSupported = _adapter.isSupported;

    if (!kIsWeb) {
      _scanSub = _adapter.scanResults.listen(
        (results) => debugPrint('scanResults update: ${results.length} items'),
        onError: (err) => debugPrint('scanResults error: $err'),
      );

      _ensurePermissionsAndScan();
    } else {
      _permissionsGranted = _isSupported;
    }
  }

  @override
  void dispose() {
    _scanSub?.cancel();
    _adapter.stopScan();
    super.dispose();
  }

  Future<void> _ensurePermissionsAndScan() async {
    if (kIsWeb) return;

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
      final connected = await _adapter.getConnectedDevices();
      debugPrint('Connected devices: ${connected.length}');

      final bonded = await _adapter.getBondedDevices();
      debugPrint('Bonded devices: ${bonded.length}');

      final connectedIds = connected.map((d) => d.id).toSet();
      final bondedOnly = bonded.where((d) => !connectedIds.contains(d.id)).toList();

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
    if (kIsWeb) {
      return (granted: _isSupported, permanentlyDenied: false);
    }

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
      await _adapter.startScan(timeout: const Duration(seconds: 15));
      debugPrint('Scan started');
    } catch (e) {
      debugPrint('Error while scanning: $e');
    }
  }

  Future<bool> _needsLegacyBluetoothPermission() async {
    if (kIsWeb) return false;
    final info = await DeviceInfoPlugin().androidInfo;
    return info.version.sdkInt < 31;
  }

  Future<void> _requestWebDevice() async {
    final device = await _adapter.requestDevice(
      serviceUuids: [BleService.SERVICE_UUID],
    );

    if (device == null || !mounted) return;

    setState(() {
      _webDevices.removeWhere((d) => d.id == device.id);
      _webDevices.add(device);
    });
  }

  @override
  Widget build(BuildContext context) {
    if (kIsWeb) {
      return Scaffold(
        body: Column(
          children: [
            ElevatedButton.icon(
              onPressed: _isSupported ? _requestWebDevice : null,
              label: const Text("Wybierz urządzenie"),
              icon: const Icon(Icons.search),
            ),
            const SizedBox(height: 10),
            Expanded(
              child: !_isSupported
                  ? _buildWebUnsupported()
                  : _buildWebDeviceList(),
            ),
          ],
        ),
      );
    }

    return Scaffold(
      body: Column(
        children: [
          ElevatedButton.icon(
            onPressed: _ensurePermissionsAndScan,
            label: const Text("Skanuj ponownie"),
            icon: const Icon(Icons.refresh),
          ),
          const SizedBox(height: 10),
          Expanded(
            child: !_permissionsGranted
                ? _buildPermissionInfo()
                : StreamBuilder<List<BleScanResult>>(
                    stream: _adapter.scanResults,
                    initialData: const [],
                    builder: (context, snapshot) {
                      final results = List<BleScanResult>.from(
                        snapshot.data ?? const [],
                      );
                      debugPrint('builder results count: ${results.length}');

                      results.sort((a, b) => b.rssi.compareTo(a.rssi));

                      final knownIds = {
                        ..._connectedDevices.map((d) => d.id),
                        ..._bondedDevices.map((d) => d.id),
                      };
                      final filteredResults = results
                          .where((r) => !knownIds.contains(r.device.id))
                          .toList();

                      final hasConnected = _connectedDevices.isNotEmpty;
                      final hasBonded = _bondedDevices.isNotEmpty;
                      final hasScanned = filteredResults.isNotEmpty;

                      if (!hasConnected && !hasBonded && !hasScanned) {
                        return const Center(
                          child: Text("Nie znaleziono urządzeń"),
                        );
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
    );
  }

  Widget _buildWebUnsupported() {
    return const Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(
            'Web Bluetooth nie jest wspierany w tej przeglądarce.',
            textAlign: TextAlign.center,
          ),
          SizedBox(height: 12),
          Text(
            'Użyj Chrome oraz HTTPS lub localhost.',
            textAlign: TextAlign.center,
          ),
        ],
      ),
    );
  }

  Widget _buildWebDeviceList() {
    if (_webDevices.isEmpty) {
      return const Center(child: Text("Nie znaleziono urządzeń"));
    }

    return ListView(
      children: _webDevices
          .map(
            (device) => _buildKnownDeviceTile(
              device,
              isConnected: false,
            ),
          )
          .toList(),
    );
  }

  Widget _buildDeviceTile(BleScanResult res) {
    final advName = res.advName;
    final name = res.device.name.isNotEmpty
        ? res.device.name
        : advName.isNotEmpty
            ? advName
            : res.device.id;

    return ListTile(
      title: Text(name, style: const TextStyle(fontWeight: FontWeight.bold)),
      subtitle: Text(res.device.id),
      trailing: ElevatedButton(
        onPressed: () {
          widget.onDeviceSelected(res.device);
        },
        child: const Text("Połącz"),
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
    BleDevice device, {
    required bool isConnected,
  }) {
    final name = device.name.isNotEmpty ? device.name : device.id;

    return ListTile(
      leading: Icon(
        isConnected ? Icons.bluetooth_connected : Icons.bluetooth,
        color: isConnected ? Colors.green : Colors.blue,
      ),
      title: Text(name, style: const TextStyle(fontWeight: FontWeight.bold)),
      subtitle: Text(device.id),
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
          Text('Naciśnij "Skanuj ponownie", aby ponownie poprosić o zgody.'),
        ],
      ),
    );
  }
}
