import 'package:flutter/material.dart';

class LoginScreen extends StatelessWidget {
  const LoginScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Login')),
      body: Padding(
        padding: EdgeInsets.all(24.0),
        child: SingleChildScrollView(
          child: Column(
            children: [
              Text("Log in", style: Theme.of(context).textTheme.headlineMedium),
              SizedBox(height: 40),
              TextField(decoration: InputDecoration(labelText: 'Email')),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Password'),
                obscureText: true,
              ),
              SizedBox(height: 40),
              ElevatedButton(
                onPressed: () {
                  // obsługa logowania - dodać później
                  Navigator.pushReplacementNamed(context, '/home');
                },
                child: Text("Login"),
              ),
              SizedBox(height: 40),
              TextButton(
                onPressed: () {
                  Navigator.pushNamed(context, '/register');
                },
                child: Text("Nie masz konta? Zarejestruj się"),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
