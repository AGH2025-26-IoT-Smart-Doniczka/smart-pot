import 'package:flutter/material.dart';

class WifiForm extends StatefulWidget {
  final Function(String ssid, String password) onSubmit;
  final bool isSending;

  const WifiForm({super.key, required this.onSubmit, this.isSending = false});

  @override
  State<WifiForm> createState() => _WifiFormState();
}

class _WifiFormState extends State<WifiForm> {
  final _ssidController = TextEditingController();
  final _passController = TextEditingController();

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        Text("Skonfiguruj Wi-Fi", style: Theme.of(context).textTheme.headlineSmall,),
        SizedBox(height: 20,),
        TextField(
          controller: _ssidController,
          decoration: InputDecoration(
            labelText: "Nazwa sieci (SSID)",
            border: OutlineInputBorder(),
            prefixIcon: Icon(Icons.wifi),
          ),
        ),
        SizedBox(height: 15,),
        TextField(
          controller: _passController,
          obscureText: true,
          decoration: InputDecoration(
            labelText: "Hasło do Wi-Fi",
            border: OutlineInputBorder(),
            prefixIcon: Icon(Icons.lock),
          ),
        ),
        SizedBox(height: 30,),
        if(widget.isSending) CircularProgressIndicator()
        else
          ElevatedButton(
            onPressed: () {
              if(_ssidController.text.isNotEmpty){
                widget.onSubmit(_ssidController.text, _passController.text);
              }
            },
            child: const Padding(
              padding: EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              child: Text("Wyślij do doniczki"),
            ),
          )
      ],
    );
  }
}
