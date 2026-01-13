import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/app.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';

void main() {
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => AuthController()..tryAutoLogin()),
        ChangeNotifierProvider(create: (_) => ThemeController()),
        ChangeNotifierProxyProvider<AuthController, PotsController>(
            create: (context) => PotsController(context.read<AuthController>()),
            update: (context, auth, previousPots) => PotsController(auth))
      ],
      child: const SmartPotApp(),
    ),
  );
}
