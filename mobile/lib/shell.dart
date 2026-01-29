import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/constants.dart';
import 'package:smart_pot_mobile_app/screens/home/home_screen.dart';
import 'package:smart_pot_mobile_app/screens/plants/plants_screen.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';

class MainShell extends StatefulWidget {
  const MainShell({super.key});

  @override
  State<MainShell> createState() => _MainShellState();
}

class _MainShellState extends State<MainShell> {
  int _index = 0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<PotsController>().fetchPots();
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeController>();
    return Scaffold(
      appBar: AppBar(
        elevation: 2.0,
        title: Text("SmartPot"),
        //ikony po prawej
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              context.read<PotsController>().fetchPots();
              context.read<PotsController>().fetchAlerts();
            },
          ),
          IconButton(
            icon: Icon(theme.isDark ? Icons.light_mode : Icons.dark_mode),
            onPressed: () {
              context.read<ThemeController>().toggle();
            },
          ),

          IconButton(
            icon: const Icon(Icons.logout),
            onPressed: () {
              context.read<AuthController>().logout();
              Navigator.pushNamedAndRemoveUntil(
                context,
                AppRoutes.login,
                (route) => false,
              );
            },
          ),
        ],
      ),
      body: IndexedStack(
        index: _index,
        children: [
          HomeScreen(onShowPlants: () => setState(() => _index = 1)),
          const MyPotsScreen(),
        ],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (i) => setState(() => _index = i),
        destinations: [
          NavigationDestination(
            icon: Icon(Icons.home_outlined),
            label: "Strona główna",
            selectedIcon: Icon(Icons.home),
          ),
          NavigationDestination(
            icon: Icon(Icons.local_florist_outlined),
            label: "Rośliny",
            selectedIcon: Icon(Icons.local_florist),
          ),
        ],
      ),
      floatingActionButton: _index == 0
          ? Padding(
              padding: EdgeInsets.all(0),
              child: SizedBox(
                width: 160,
                height: 70,
                child: FloatingActionButton.extended(
                  onPressed: () {
                    Navigator.pushNamed(context, '/new_plant');
                  },
                  icon: Icon(Icons.add),
                  label: Text("Dodaj roślinę"),
                ),
              ),
            )
          : null,
      floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
    );
  }
}
