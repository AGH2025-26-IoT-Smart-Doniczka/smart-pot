import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';

class RegisterScreen extends StatefulWidget {
  const RegisterScreen({super.key});

  @override
  State<RegisterScreen> createState() => _RegisterScreenState();
}

class _RegisterScreenState extends State<RegisterScreen> {
  final _usernameController = TextEditingController();
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();
  final _confirmPasswordController = TextEditingController();
  @override
  Widget build(BuildContext context) {
    final authController = context.watch<AuthController>();
    return Scaffold(
      appBar: AppBar(title: const Text('Register')),
      body: Padding(
        padding: EdgeInsets.all(24.0),
        child: SingleChildScrollView(
          child: Column(
            children: [
              Text(
                "Register",
                style: Theme.of(context).textTheme.headlineMedium,
              ),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Username', prefixIcon: Icon(Icons.person)),
                controller: _usernameController,
              ),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Email', prefixIcon: Icon(Icons.email)),
                controller: _emailController,
                keyboardType: TextInputType.emailAddress,
              ),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Password', prefixIcon: Icon(Icons.lock)),
                obscureText: true,
                controller: _passwordController,
              ),
              SizedBox(height: 40),
              TextField(
                decoration: InputDecoration(labelText: 'Confirm Password', prefixIcon: Icon(Icons.lock)),
                obscureText: true,
                controller: _confirmPasswordController,
              ),
              SizedBox(height: 10),

              if (authController.errorMessage != null)
                Padding(
                  padding: const EdgeInsets.only(bottom: 10),
                  child: Text(
                    authController.errorMessage!,
                    style: const TextStyle(color: Colors.red),
                  ),
                ),

              const SizedBox(height: 20),

              authController.isLoading
                  ? const CircularProgressIndicator()
                  : ElevatedButton(
                      onPressed: () async {
                        final username = _usernameController.text;
                        final email = _emailController.text;
                        final pass = _passwordController.text;
                        final confirmPass = _confirmPasswordController.text;

                        if (username.isEmpty ||
                            email.isEmpty ||
                            pass.isEmpty ||
                            confirmPass.isEmpty) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text("Wypełnij wszystkie pola"),
                            ),
                          );
                          return;
                        }
                        ;
                        if (pass != confirmPass) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text("Hasła nie są takie same"),
                            ),
                          );
                          return;
                        }
                        final success = await authController.register(
                          email,
                          pass,
                          username
                        );
                        if (success && context.mounted) {
                          Navigator.pushReplacementNamed(context, '/home');
                        }
                      },
                      style: ElevatedButton.styleFrom(
                        padding: const EdgeInsets.symmetric(
                          vertical: 15,
                          horizontal: 50,
                        ),
                      ),
                      child: Text("Register"),
                    ),

              SizedBox(height: 20),
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
