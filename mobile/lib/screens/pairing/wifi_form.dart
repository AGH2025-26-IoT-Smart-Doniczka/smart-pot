import 'package:flutter/material.dart';

class WifiForm extends StatefulWidget {
  final Function(String ssid, String password, Map<String, dynamic> config) onSubmit;
  final bool isSending;

  const WifiForm({super.key, required this.onSubmit, this.isSending = false});

  @override
  State<WifiForm> createState() => _WifiFormState();
}

class _WifiFormState extends State<WifiForm> {
  final _ssidController = TextEditingController();
  final _passController = TextEditingController();

  final _measureIntervalController = TextEditingController(text: "3600");
  final _sendIntervalController = TextEditingController(text: "7200");
  final _wateringIntervalController = TextEditingController(text: "0");
  final _wateringDurationController = TextEditingController(text: "5");
  RangeValues _moistureRange = const RangeValues(20, 60);

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Text("Skonfiguruj Wi-Fi", style: Theme.of(context).textTheme.headlineSmall),
          SizedBox(height: 20),
          TextField(
            controller: _ssidController,
            decoration: InputDecoration(
              labelText: "Nazwa sieci (SSID)",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.wifi),
            ),
          ),
          SizedBox(height: 15),
          TextField(
            controller: _passController,
            obscureText: true,
            decoration: InputDecoration(
              labelText: "Hasło do Wi-Fi",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.lock),
            ),
          ),
          
          SizedBox(height: 30),
          Text("Ustawienia początkowe", style: Theme.of(context).textTheme.titleMedium),
          SizedBox(height: 15),
          
          TextField(
            controller: _measureIntervalController,
            keyboardType: TextInputType.number,
            decoration: InputDecoration(
              labelText: "Częstotliwość pomiarów (s)",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.timer),
            ),
          ),
          SizedBox(height: 15),

          TextField(
            controller: _sendIntervalController,
             keyboardType: TextInputType.number,
            decoration: InputDecoration(
              labelText: "Częstotliwość wysyłania (s)",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.cloud_upload),
            ),
          ),
          SizedBox(height: 15),
           
          Text("Zakres wilgotności: ${_moistureRange.start.round()}% - ${_moistureRange.end.round()}%"),
          RangeSlider(
            values: _moistureRange,
            min: 0,
            max: 100,
            divisions: 100,
            labels: RangeLabels(
              _moistureRange.start.round().toString(),
               _moistureRange.end.round().toString(),
            ),
            onChanged: (RangeValues values) {
              setState(() {
                _moistureRange = values;
              });
            },
          ),
          SizedBox(height: 15),

          TextField(
            controller: _wateringIntervalController,
             keyboardType: TextInputType.number,
            decoration: InputDecoration(
              labelText: "Interwał podlewania (s, 0=wył)",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.schedule),
            ),
          ),
          SizedBox(height: 15),
          
          TextField(
            controller: _wateringDurationController,
             keyboardType: TextInputType.number,
            decoration: InputDecoration(
              labelText: "Czas podlewania (s)",
              border: OutlineInputBorder(),
              prefixIcon: Icon(Icons.water_drop),
            ),
          ),


          SizedBox(height: 30),
          if(widget.isSending) Center(child: CircularProgressIndicator())
          else
            ElevatedButton(
              onPressed: _handleSubmit,
              child: const Padding(
                padding: EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                child: Text("Wyślij do doniczki"),
              ),
            )
        ],
      ),
    );
  }

  void _handleSubmit() {
    if (_ssidController.text.isEmpty) {
       ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Podaj nazwę sieci")));
       return;
    }

    final int? measureInterval = int.tryParse(_measureIntervalController.text);
    if (measureInterval == null || measureInterval < 0) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Błędny czas pomiaru")));
      return;
    }

    final int? sendInterval = int.tryParse(_sendIntervalController.text);
    if (sendInterval == null || sendInterval < 0) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Błędny czas wysyłania")));
      return;
    }

    final int? wateringInterval = int.tryParse(_wateringIntervalController.text);
    if (wateringInterval == null || wateringInterval < 0) {
       ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Błędny interwał podlewania")));
       return;
    }

    final int? wateringDuration = int.tryParse(_wateringDurationController.text);
    if (wateringDuration == null || wateringDuration < 0) {
       ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("Błędny czas podlewania")));
       return;
    }

    final config = {
      "mes": measureInterval,
      "moi": [_moistureRange.start.round(), _moistureRange.end.round()],
      "wat": wateringDuration,
      "sen": sendInterval,
      "lux": 1, 
      "wai": wateringInterval,
    };

    widget.onSubmit(_ssidController.text, _passController.text, config);
  }
}
