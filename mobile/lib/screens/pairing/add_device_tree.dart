import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_pot_mobile_app/screens/pairing/pin_entry_screen.dart';
import 'package:smart_pot_mobile_app/screens/pairing/scan_view.dart';
import 'package:smart_pot_mobile_app/screens/pairing/wifi_form.dart';

// różne UUID
const String SERVICE_UUID = "5c73aa37-a268-40d1-b8d8-7f7a479490a2";
const String CHARACTERISTICS_AUTH_UUID = "456f3c1e-3272-4a0c-9868-cebdd4fc0bee";
const String CHARACTERISTICS_WIFI_UUID = "a3c35f05-51e9-4ad1-9b04-827243129788";

enum PairingStep {
  scanning,
  connecting,
  codeEntry,
  wifiCredentials,
  success,
  failure,
}

class DeviceTree extends StatefulWidget {
  const DeviceTree({super.key});

  @override
  State<DeviceTree> createState() => _DeviceTreeState();
}

class _DeviceTreeState extends State<DeviceTree> {
  PairingStep _step = PairingStep.scanning;
  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _authCharacteristic;
  BluetoothCharacteristic? _wifiCharacteristic;

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
      print("SYMULACJA: Łączenie z urządzeniem...");
      await Future.delayed(const Duration(seconds: 2));
      print("SYMULACJA: Odkrywanie serwisów...");
      await Future.delayed(const Duration(seconds: 1));
      print("SYMULACJA: Wysłano żądanie REQ_PIN.");

      setState(() {
        _step = PairingStep.codeEntry;
      });
      return;
    }

    try {
      await device.connect(autoConnect: false, license: License.free);

      if (Platform.isAndroid)
        await Future.delayed(const Duration(milliseconds: 500));

      final services = await device.discoverServices();

      BluetoothCharacteristic? authChar;
      BluetoothCharacteristic? wifiChar;

      for (var service in services) {
        if (service.uuid.toString() == SERVICE_UUID) {
          for (var char in service.characteristics) {
            if (char.uuid.toString() == CHARACTERISTICS_AUTH_UUID) {
              authChar = char;
            }
            if (char.uuid.toString() == CHARACTERISTICS_WIFI_UUID) {
              wifiChar = char;
            }
          }
        }
      }

      if (authChar == null) {
        throw new Exception("Nie znaleziono usługi autoryzacji na urządzeniu");
      }
      if (wifiChar == null) {
        throw new Exception("Nie znaleziono usługi WiFi na urządzeniu");
      }

      _connectedDevice = device;
      _authCharacteristic = authChar;
      _wifiCharacteristic = wifiChar;

      // 2. wysłanie żądania o kod
      await authChar.write(utf8.encode("REQ_PIN"));

      setState(() {
        _step = PairingStep.codeEntry;
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

  // 3. Weryfikacja kodu
  Future<void> _verifyPin(String pin) async {
    setState(() {
      _isProcessing = true;
    });

    if (isSimulation) {
      print("SYMULACJA: Weryfikacja PINu: $pin");
      await Future.delayed(const Duration(seconds: 1));
      if (pin == "123456") {
        setState(() {
          _step = PairingStep.success;
          _isProcessing = false;
        });
      } else {
        print("SYMULACJA: PIN jest błędny");
        setState(() {
          _errorMessage = "Błędny kod PIN (w symulacji wpisz 123456)";
          _isProcessing = false;
        });

        //dla lepszego efektu
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Błędny PIN! Wpisz: 123456')),
        );
      }
      return;
    }

    if (_authCharacteristic == null) return;

    try {
      await _authCharacteristic!.write(utf8.encode(pin));

      // Tutaj czekamy na potwierdzenie.
      // Opcja A: ESP rozłącza się przy błędzie.
      // Opcja B: Czytamy wartość charakterystyki, która zmienia się na "OK" lub "FAIL".
      // Załóżmy Opcję B (odczyt):
      await Future.delayed(const Duration(milliseconds: 500));
      List<int> responseBytes = await _authCharacteristic!.read();
      String response = utf8.decode(responseBytes);

      if (response == "OK") {
        setState(() {
          _step = PairingStep.wifiCredentials;
          _isProcessing = false;
        });
      } else {
        throw Exception("Błędny kod PIN");
      }
    } catch (e) {
      setState(() {
        _errorMessage = "Weryfikacja nieudana: $e";
        _isProcessing = false; //zostajemy na ekranie kodu
      });
    }
  }

  Future<void> _sendWifiConfig(String ssid, String pass) async {
    setState(() {
      _isProcessing = true;
    });

    if (_wifiCharacteristic == null) {
      setState(() {
        _errorMessage = "Utracono połączenie z usługą WiFi";
        _step = PairingStep.failure;
      });
      return;
    }

    try {
      // Tworzymy Json
      Map<String, String> configData = {"ssid": ssid, "password": pass};
      String jsonString = jsonEncode(configData);
      List<int> bytes = utf8.encode(jsonString);

      await _wifiCharacteristic!.write(bytes);

      // czekanie na potwierdzenie od płytki czy odebrała i próbuje się łącztyć
      // poźniej dodaj tu nasłuchiwanie powiadomień

      setState(() {
        _step = PairingStep.success;
      });
    } catch (e) {
      setState(() {
        _errorMessage = "Wystąpił błąd: $e";
        _isProcessing = false;
        // bez wchodzenia do failure żeby user mógł poprawić hasło
      });
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text("Błąd: $e")));
    }
  }

  void _reset() {
    _connectedDevice?.disconnect();
    setState(() {
      _step = PairingStep.scanning;
      _connectedDevice = null;
      _authCharacteristic = null;
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
              Text("Nawiązywanie połączenia z doniczką..."),
              Text("Prosze czekać na wygenerowanie kodu."),
            ],
          ),
        );

      case PairingStep.codeEntry:
        return PinEntryScreen(
          onPinSubmitted: _verifyPin,
          isLoading: _isProcessing,
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
              const SizedBox(height: 20),
              ElevatedButton(
                onPressed: () {
                  Navigator.of(context).pop(); //zamknięcie dodawania nowej doniczki
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
      case PairingStep.codeEntry:
        return "Weryfikacja";
      case PairingStep.success:
        return "Gotowe";
      case PairingStep.failure:
        return "Błąd";
      case PairingStep.wifiCredentials:
        return "Konfiguracja Wi-Fi";
    }
  }
}
