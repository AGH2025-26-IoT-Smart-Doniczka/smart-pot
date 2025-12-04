import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:smart_pot_mobile_app/screens/pairing/scan_view.dart';
import 'package:smart_pot_mobile_app/screens/pairing/wifi_form.dart';

// różne UUID
const String SERVICE_UUID = "5c73aa37-a268-40d1-b8d8-7f7a479490a2";
const String CHARACTERISTICS_PASS_UUID = "456f3c1e-3272-4a0c-9868-cebdd4fc0bee";
const String CHARACTERISTICS_SSID_UUID = "a3c35f05-51e9-4ad1-9b04-827243129788";

enum PairingStep {
  scanning,
  connecting,
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

  // referencja do osobnych characteristics
  BluetoothCharacteristic? _ssidCharacteristics;
  BluetoothCharacteristic? _passCharacteristics;

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
      if(device.bondState != BluetoothBondState.bonded){
        try{
          print("Parowanie rozpoczęte");
          await device.createBond();

          // czekamy na wyświetlenie okienka i wpisanie pinu prezz użytkownika
          await Future.delayed(const Duration(seconds: 3));
        }catch (e){
          print("Ostrzeżenie przy parowaniu: $e");
        }
      }

      print("Odkrywanie serwisów");
      final services = await device.discoverServices();

      BluetoothCharacteristic? ssidChar;
      BluetoothCharacteristic? passChar;

      for (var service in services) {
        if (service.uuid.toString() == SERVICE_UUID) {
          for (var char in service.characteristics) {
            if (char.uuid.toString() == CHARACTERISTICS_SSID_UUID) {
              ssidChar = char;
            }
            if (char.uuid.toString() == CHARACTERISTICS_PASS_UUID) {
              passChar = char;
            }
          }
        }
      }

      if(ssidChar == null || passChar == null){
        throw Exception("Nie znaleziono charakterystyk konfiguracji Wi-Fi (SSID/PASS)");
      };

      _ssidCharacteristics = ssidChar;
      _passCharacteristics = passChar;

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

    if(isSimulation){
      print("SYMUALCJA: Wysyłąm SSID: $ssid");
      print("SYMULACJA: Wysyłam PASS: $pass");
      await Future.delayed(const Duration(seconds: 2));
      setState(() {
        _step = PairingStep.success;
        _isProcessing = false;
      });
    }


    if (_ssidCharacteristics == null || _passCharacteristics == null) {
      setState(() {
        _errorMessage = "Utracono połączenie z usługami urządzenia";
        _step = PairingStep.failure;
      });
      return;
    }

    try {
      // Wysyłanie SSID
      print("Wysyłanie SSID...");
      await _ssidCharacteristics!.write(utf8.encode(ssid));

      // opóźnienie podobno potrzebne dla BLE stacka XD
      await Future.delayed(const Duration(milliseconds: 300));

      //Wysłanie PASS
      print("Wysyłanie Hasła...");
      await _passCharacteristics!.write(utf8.encode(pass));

      setState(() {
        _step = PairingStep.success;
      });

    } catch (e) {
      setState(() {
        _errorMessage = "Błąd podczas wysyłania danych: $e";
        _isProcessing = false;
        // bez wchodzenia do failure żeby user spróbować ponownie
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
      _ssidCharacteristics = null;
      _passCharacteristics = null;
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
              Text("Jeśli pojawi się systemowe okienko parowanie, \nwpisz kod z urządzenia", textAlign: TextAlign.center, style: TextStyle(color: Colors.grey),),
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
