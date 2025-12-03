import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';

class PinEntryScreen extends StatefulWidget {
  final Function(String) onPinSubmitted;
  final bool isLoading;

  const PinEntryScreen({
    super.key,
    required this.onPinSubmitted,
    this.isLoading = false,
  });

  @override
  State<PinEntryScreen> createState() => _PinEntryScreenState();
}

class _PinEntryScreenState extends State<PinEntryScreen> {
  final TextEditingController _pinController = TextEditingController();

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        const Text(
          "Wpisz kod z wyświetlacza na urządzeniu",
          style: TextStyle(fontSize: 18),
          textAlign: TextAlign.center,
        ),
        const SizedBox(height: 20),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 40),
          child: TextField(
            controller: _pinController,
            keyboardType: TextInputType.number,
            maxLength: 6,
            textAlign: TextAlign.center,
            style: const TextStyle(fontSize: 24, letterSpacing: 8),
            decoration: const InputDecoration(
              hintText: "000000",
              border: OutlineInputBorder(),
            ),
          ),
        ),
        const SizedBox(height: 20,),
        if(widget.isLoading) const CircularProgressIndicator()
        else
          ElevatedButton(
            onPressed: () {
              if (_pinController.text.length == 6) {
                widget.onPinSubmitted(_pinController.text);
              }
            },
            child: const Text("Paruj"),
          )
      ],
    );
  }
}
