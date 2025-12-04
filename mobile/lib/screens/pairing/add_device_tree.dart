import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/screens/pairing/scan_view.dart';
import 'package:smart_pot_mobile_app/screens/pairing/wifi_form.dart';
import 'package:smart_pot_mobile_app/services/ble_service.dart';
import 'package:smart_pot_mobile_app/services/ble_types.dart';

enum PairingStep { scanning, connecting, wifiCredentials, success, failure }

class DeviceTree extends StatefulWidget {
  const DeviceTree({super.key});

  @override
  State<DeviceTree> createState() => _DeviceTreeState();
}

class _DeviceTreeState extends State<DeviceTree> {
  PairingStep _step = PairingStep.scanning;
  BleDevice? _connectedDevice;

  String _errorMessage = "";
  bool _isProcessing = false;
  String? _detectedPotId;
  bool _isHardReset = false;

  Future<void> _connectToDevice(BleDevice device) async {
    setState(() {
      _step = PairingStep.connecting;
      _errorMessage = "";
    });

    try {
      await device.connect();

      if (!kIsWeb && defaultTargetPlatform == TargetPlatform.android) {
        await Future.delayed(const Duration(milliseconds: 500));
      }

      _connectedDevice = device;

      try {
        final result = await BleService().readResetCharacteristic(device.id);
        _detectedPotId = result.potId;
        _isHardReset = result.isHardReset;
        debugPrint(
          "Detected Pot ID: $_detectedPotId, Hard Reset: $_isHardReset",
        );
      } catch (e) {
        final isIos =
            !kIsWeb && defaultTargetPlatform == TargetPlatform.iOS;
        final errorText = e.toString().toLowerCase();
        final isTimeout = errorText.contains("timed out");
        if (isIos && isTimeout) {
          await device.disconnect();
          setState(() {
            _errorMessage = "Niepoprawny kod parowania. Spróbuj ponownie.";
            _step = PairingStep.connecting;
          });
          return;
        }
        debugPrint("Could not read reset characteristic: $e");
        _detectedPotId = null;
        _isHardReset = false;
      }

      if (!kIsWeb && defaultTargetPlatform == TargetPlatform.android) {
        final alreadyBonded = await device.isBonded;
        if (!alreadyBonded) {
          try {
            await device.createBond();
            await Future.delayed(const Duration(seconds: 3));
          } catch (e) {
            await device.disconnect();
            setState(() {
              _errorMessage = "Niepoprawny kod parowania. Spróbuj ponownie.";
              _step = PairingStep.connecting;
            });
            return;
          }
        }

        final bondedNow = await device.isBonded;
        if (!bondedNow) {
          await device.disconnect();
          setState(() {
            _errorMessage = "Niepoprawny kod parowania. Spróbuj ponownie.";
            _step = PairingStep.connecting;
          });
          return;
        }
      }

      setState(() {
        _step = PairingStep.wifiCredentials;
      });

    } catch (e) {
      print("Błąd połączenia: $e");
      await device.disconnect();
      final isIos =
          !kIsWeb && defaultTargetPlatform == TargetPlatform.iOS;
      setState(() {
        if (isIos) {
          _errorMessage = "Niepoprawny kod parowania. Spróbuj ponownie.";
          _step = PairingStep.connecting;
        } else {
          _errorMessage = e.toString();
          _step = PairingStep.failure;
        }
      });
    }
  }

  Future<void> _sendWifiConfig(String ssid, String pass, Map<String, dynamic>? config) async {
    setState(() {
      _isProcessing = true;
    });

    try {
      if (_connectedDevice == null) {
        throw Exception("Utracono połączenie z urządzeniem BLE");
      }

      final currentUser = context.read<AuthController>().currentUser;
      if (currentUser == null) {
        throw Exception("Użytkownik nie jest zalogowany. ");
      }

      final deviceName = _connectedDevice!.name;
      String potId;
      if (_detectedPotId != null) {
        potId = _detectedPotId!;
      } else if (deviceName.startsWith("PROV_")) {
        potId = deviceName.replaceFirst("PROV_", "");
      } else {
        potId = _connectedDevice!.id.replaceAll(':', '');
      }

      if (_isHardReset) {
        await context.read<PotsController>().notifyHardReset(potId);
      }

      print("Device ID: ${_connectedDevice!.id}");
      print("Device Name: $deviceName");
      print("Resolved Pot ID: $potId");

      Map<String, dynamic> pairingData = await context
          .read<PotsController>()
          .pairPotWithServer(potId);

      print(pairingData);
      final String role = pairingData['role'] ?? 'user';
      final Map<String, dynamic> mqttData = pairingData['mqtt'] ?? {};

      String mqttUser = mqttData['username'] ?? '';
      String mqttPass = mqttData['password'] ?? '';

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

      print("Status właściciela: $isOwner");

      if (await _connectedDevice!.isConnected == false) {
        await _connectedDevice!.connect();
      }

      if (mqttPass.isNotEmpty) {
        const storage = FlutterSecureStorage();
        await storage.write(key: 'mqtt_password', value: mqttPass);
        if (mqttUser.isNotEmpty) {
          await storage.write(key: 'mqtt_username', value: mqttUser);
        }
      }

      final String? ssidToSend = ssid.isNotEmpty ? ssid : null;
      final String? passToSend = pass.isNotEmpty ? pass : null;

      await BleService().writeConfiguration(
        device: _connectedDevice!,
        ssid: ssidToSend,
        wifiPass: passToSend,
        mqttPass: mqttPass,
        mqttUser: mqttUser,
        customConfig: config,
        sendConfig: config != null,
      );

      await _connectedDevice!.disconnect();

      if (mounted) {
        context.read<PotsController>().fetchPots();
        setState(() {
          _isProcessing = false;
          _step = PairingStep.success;
        });
      }
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
        if (_errorMessage.isNotEmpty) {
          return Center(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Icon(Icons.error_outline, color: Colors.red, size: 80),
                const SizedBox(height: 20),
                const Text(
                  "Nie udało się sparować",
                  style: TextStyle(fontSize: 20),
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 16),
                Text(
                  _errorMessage,
                  textAlign: TextAlign.center,
                  style: const TextStyle(color: Colors.red),
                ),
                const SizedBox(height: 12),
                ElevatedButton(
                  onPressed: _reset,
                  child: const Text("Powrót"),
                ),
              ],
            ),
          );
        }
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
        return WifiForm(
          onSubmit: _sendWifiConfig,
          isSending: _isProcessing,
          isHardReset: _isHardReset,
        );

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
