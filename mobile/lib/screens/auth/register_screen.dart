import 'package:flutter/material.dart';

class RegisterScreen extends StatelessWidget {
  const RegisterScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Register')),
      body: Padding(
        padding: EdgeInsets.all(24.0),
        child: SingleChildScrollView(
          child: Column(
            children: [
              Text("Register", style: Theme.of(context).textTheme.headlineMedium),
              SizedBox(height: 40),
              TextField(decoration: InputDecoration(labelText: 'Username')),
              SizedBox(height: 40),
              TextField(decoration: InputDecoration(labelText: 'Email')),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Password'),
                obscureText: true,
              ),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Confirm Password'),
                obscureText: true,
              ),
              SizedBox(height: 40),
              ElevatedButton(
                onPressed: () {
                  // obsługa rejestracji - dodać później
                  Navigator.pushReplacementNamed(context, '/home');
                },
                child: Text("Register"),
              ),
              SizedBox(height: 40),
              TextButton(
                onPressed: () {
                  Navigator.pop(context);
                },
                child: Text("Masz już konto? Zaloguj się"),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
