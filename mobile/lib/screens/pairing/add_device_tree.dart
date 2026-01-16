import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/screens/pairing/scan_view.dart';
import 'package:smart_pot_mobile_app/screens/pairing/wifi_form.dart';
import 'package:smart_pot_mobile_app/services/ble_service.dart';

enum PairingStep { scanning, connecting, wifiCredentials, success, failure }

class DeviceTree extends StatefulWidget {
  const DeviceTree({super.key});

  @override
  State<DeviceTree> createState() => _DeviceTreeState();
}

class _DeviceTreeState extends State<DeviceTree> {
  PairingStep _step = PairingStep.scanning;
  BluetoothDevice? _connectedDevice;

  String _errorMessage = "";
  bool _isProcessing = false;

  //do symulacji
  bool isSimulation = false;

  // 1. Logika łączenia - GAP
  Future<void> _connectToDevice(BluetoothDevice device) async {
    setState(() {
      _step = PairingStep.connecting;
      _errorMessage = "";
    });

    // SYMULACJA
    if (isSimulation) {
      print("SYMULACJA: Łączenie...");
      await Future.delayed(const Duration(seconds: 2));
      setState(() {
        _step = PairingStep.wifiCredentials;
      });
      return;
    }

    try {
      await device.connect(autoConnect: false, license: License.free);

      if (Platform.isAndroid)
        await Future.delayed(const Duration(milliseconds: 500));

      _connectedDevice = device;

      // Parowanie (bounding)
      if (device.bondState != BluetoothBondState.bonded) {
        try {
          print("Parowanie rozpoczęte");
          await device.createBond();

          // czekamy na wyświetlenie okienka i wpisanie pinu prezz użytkownika
          await Future.delayed(const Duration(seconds: 3));
        } catch (e) {
          print("Ostrzeżenie przy parowaniu: $e");
        }
      }

      setState(() {
        _step = PairingStep.wifiCredentials;
      });
    } catch (e) {
      print("Błąd połączenia: $e");
      await device.disconnect();
      setState(() {
        _errorMessage = e.toString();
        _step = PairingStep.failure;
      });
    }
  }

  Future<void> _sendWifiConfig(String ssid, String pass) async {
    setState(() {
      _isProcessing = true;
    });

    try {
      if (_connectedDevice == null && isSimulation == false) {
        throw Exception("Utracono połączenie z urządzeniem BLE");
      }

      final currentUser = context.read<AuthController>().currentUser;
      if (currentUser == null)
        throw Exception("Użytkownik nie jest zalogowany. ");

      final potId = isSimulation
          ? "Sim-Pot-001"
          : _connectedDevice!.remoteId.str.replaceAll(':', '');
      print(_connectedDevice!.remoteId.str);
      print(potId);

      Map<String, dynamic> pairingData;
      if (!isSimulation) {
        pairingData = await context.read<PotsController>().pairPotWithServer(
          potId,
        );
      } else {
        pairingData = {
          "role": "owner",
          "mqtt": {"username": "TEST_MQTT_USER", "password": "TEST_MQTT_PASS"},
        };
      }
      print(pairingData);
      final String role = pairingData['role'] ?? 'user';
      final Map<String, dynamic> mqttData = pairingData['mqtt'] ?? {};

      String mqttUser = mqttData['username'] ?? '';
      String mqttPass = mqttData['password'] ?? '';

      // If backend did not return MQTT credentials, try previously stored ones.
      if (mqttPass.isEmpty) {
        const storage = FlutterSecureStorage();
        final storedPass = await storage.read(key: 'mqtt_password');
        final storedUser = await storage.read(key: 'mqtt_username');
        if (storedPass != null && storedPass.isNotEmpty) {
          mqttPass = storedPass;
          if (storedUser != null && storedUser.isNotEmpty) {
            mqttUser = storedUser;
          }
        }
      }

      final bool isOwner = role == 'owner';

      print("Status właściciela: ${isOwner}");

      if (!isSimulation) {
        if (_connectedDevice!.isConnected == false) {
          await _connectedDevice!.connect(license: License.free);
        }

        // Persist freshly issued MQTT credentials for later use
        if (mqttPass.isNotEmpty) {
          const storage = FlutterSecureStorage();
          await storage.write(key: 'mqtt_password', value: mqttPass);
          if (mqttUser.isNotEmpty) {
            await storage.write(key: 'mqtt_username', value: mqttUser);
          }
        }

        await BleService().writeConfiguration(
          device: _connectedDevice!,
          ssid: ssid,
          wifiPass: pass,
          mqttPass: mqttPass,
          mqttUser: mqttUser,
        );

        await _connectedDevice!.disconnect();
      } else {
        await Future.delayed(const Duration(seconds: 2));
      }

      // if (mounted) {
      //   context
      //       .read<PotsController>()
      //       // .fetchPots(); // odświeżanie listy doniczek
      //   setState(() {
      //     _isProcessing = false;
      //     _step = PairingStep.success;
      //   });
      // }
    } catch (e) {
      setState(() {
        _errorMessage = "Wystąpił błąd: $e";
        _step = PairingStep.failure;
        _isProcessing = false;
      });
    }
  }

  void _reset() {
    _connectedDevice?.disconnect();
    setState(() {
      _step = PairingStep.scanning;
      _connectedDevice = null;
      _errorMessage = "";
      _isProcessing = false;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(_getTitleForStep(_step)),
        leading: _step != PairingStep.scanning
            ? IconButton(onPressed: _reset, icon: const Icon(Icons.arrow_back))
            : null,
      ),
      body: SafeArea(
        child: Padding(padding: EdgeInsets.all(20), child: _buildBody()),
      ),
    );
  }

  Widget _buildBody() {
    switch (_step) {
      case PairingStep.scanning:
        return DeviceScanScreen(onDeviceSelected: _connectToDevice);

      case PairingStep.connecting:
        return const Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              CircularProgressIndicator(),
              SizedBox(height: 20),
              Text("Łączenie i parowanie..."),
              SizedBox(height: 20),
              Text(
                "Jeśli pojawi się systemowe okienko parowanie, \nwpisz kod z urządzenia",
                textAlign: TextAlign.center,
                style: TextStyle(color: Colors.grey),
              ),
            ],
          ),
        );

      case PairingStep.wifiCredentials:
        return WifiForm(onSubmit: _sendWifiConfig, isSending: _isProcessing);

      case PairingStep.success:
        return Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.check_circle, color: Colors.green, size: 80),
              const SizedBox(height: 20),
              const Text(
                "Doniczka skonfigurowana",
                style: TextStyle(fontSize: 20),
              ),
              const SizedBox(height: 10),
              const Text("Urządzenie łączy się teraz z Twoim Wi-Fi."),
              const SizedBox(height: 30),
              ElevatedButton(
                onPressed: () {
                  Navigator.of(context).pop();
                },
                child: const Text("Wróć do ekranu głównego"),
              ),
            ],
          ),
        );

      case PairingStep.failure:
        return Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.error_outline, color: Colors.red, size: 80),
              const SizedBox(height: 20),
              const Text("Wystąpił błąd", style: TextStyle(fontSize: 20)),
              Padding(
                padding: const EdgeInsets.all(16.0),
                child: Text(_errorMessage, textAlign: TextAlign.center),
              ),
              ElevatedButton(
                onPressed: _reset,
                child: const Text("Spróbuj ponownie"),
              ),
            ],
          ),
        );
    }
    return Container();
  }

  String _getTitleForStep(PairingStep step) {
    switch (step) {
      case PairingStep.scanning:
        return "Szukanie urządzeń";
      case PairingStep.connecting:
        return "Łączenie...";
      case PairingStep.success:
        return "Gotowe";
      case PairingStep.failure:
        return "Błąd";
      case PairingStep.wifiCredentials:
        return "Konfiguracja Wi-Fi";
    }
  }
}
