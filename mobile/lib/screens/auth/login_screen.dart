import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});
  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();

  @override
  Widget build(BuildContext context) {
    final authController = context.watch<AuthController>();

    return Scaffold(
      appBar: AppBar(title: const Text('Login')),
      body: Padding(
        padding: EdgeInsets.all(24.0),
        child: SingleChildScrollView(
          child: Column(
            children: [
              Text("Log in", style: Theme.of(context).textTheme.headlineMedium),
              SizedBox(height: 40),
              TextField(controller: _emailController, decoration: InputDecoration(labelText: 'Email', prefixIcon: Icon(Icons.email)),),
              SizedBox(height: 40),
              TextField(
                controller: _passwordController,
                decoration: InputDecoration(labelText: 'Password', prefixIcon: Icon(Icons.lock)),
                obscureText: true,
              ),
              SizedBox(height: 10),

              if(authController.errorMessage != null)
                Padding(
                  padding: const EdgeInsets.only(bottom: 10),
                  child: Text(
                    authController.errorMessage!,
                    style: const TextStyle(color: Colors.red),
                  ),
                ),

              const SizedBox(height: 20),

              authController.isLoading ?
                  const CircularProgressIndicator()
              : ElevatedButton(
                  onPressed: () async {
                    final email = _emailController.text;
                    final pass = _passwordController.text;
                    if(email.isEmpty || pass.isEmpty){
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(content: Text("Wypełnij wszystkie pola"))
                      );
                      return;
                    }
                    final success = await authController.login(email, pass);
                    if(success && context.mounted){
                      Navigator.pushReplacementNamed(context, '/home');
                    }
                  },
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(vertical: 15, horizontal: 50),
                  ),
                  child: const Text('Login')),

              SizedBox(height: 20),
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
