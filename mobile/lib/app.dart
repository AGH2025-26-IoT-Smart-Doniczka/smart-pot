import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/constants.dart';
import 'package:smart_pot_mobile_app/screens/auth/login_screen.dart';
import 'package:smart_pot_mobile_app/screens/auth/register_screen.dart';
import 'package:smart_pot_mobile_app/screens/home/home_screen.dart';
import 'package:smart_pot_mobile_app/screens/pairing/add_device_tree.dart';
import 'package:smart_pot_mobile_app/screens/plants/plants_screen.dart';
import 'package:smart_pot_mobile_app/shell.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';

class SmartPotApp extends StatelessWidget {
  const SmartPotApp({super.key});

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeController>();
    return MaterialApp(
      title: "Smart Pot",
      initialRoute: AppRoutes.login,
      routes: {
        AppRoutes.login: (context) => LoginScreen(),
        AppRoutes.register: (context) => RegisterScreen(),
        AppRoutes.home: (context) => MainShell(),
        AppRoutes.plants: (context) => MyPotsScreen(),
        AppRoutes.addPlant: (context) => DeviceTree()
      },
      themeMode: theme.mode,
      theme: ThemeData(
        useMaterial3: true,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.lightGreenAccent,
          brightness: Brightness.light,
        ),
      ),
      darkTheme: ThemeData(
        useMaterial3: true,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.lightGreenAccent,
          brightness: Brightness.dark,
        ),
      ),
    );
  }
}
