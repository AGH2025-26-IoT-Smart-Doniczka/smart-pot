import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
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
  Widget build(BuildContext context) {
    final theme = context.watch<ThemeController>();
    final titles = ['Home', 'Collection'];
    return Scaffold(
      appBar: AppBar(
        title: Text(titles[_index]),
        centerTitle: true,
        elevation: 2.0,
        //ikony po prawej
        actions: [
          //tymaczasowo zmiana theme w pasku
          IconButton(
            icon: Icon(theme.isDark ? Icons.light_mode : Icons.dark_mode),
            onPressed: () {
              context.read<ThemeController>().toggle();
            },
          ),
          IconButton(
            icon: Icon(Icons.settings),
            onPressed: () {
              // dodaj przejście do ustawień
            },
          ),
          IconButton(
            icon: Icon(Icons.person_2_rounded),
            onPressed: () {
              // dodaj przejście do profilu
            },
          ),
        ],
      ),
      body: IndexedStack(
        index: _index,
        children: [
          HomeScreen(
            onShowPlants: () => setState(() => _index=1),
          ),
          const MyPotsScreen()],
      ),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _index,
        onDestinationSelected: (i) => setState(() => _index = i),
        destinations: [
          NavigationDestination(
            icon: Icon(Icons.home_outlined),
            label: "Home",
            selectedIcon: Icon(Icons.home),
          ),
          NavigationDestination(
            icon: Icon(Icons.local_florist_outlined),
            label: "Plants",
            selectedIcon: Icon(Icons.local_florist),
          ),
        ],
      ),
      floatingActionButton: _index == 0
        ? Padding(
        padding: EdgeInsets.all(0),
        child: SizedBox(
          width: 110,
          height: 70,
          child: FloatingActionButton.extended(
            onPressed: () {
              Navigator.pushNamed(context, '/new_plant');
            },
            icon: Icon(Icons.add),
            label: Text("Add  Plant"),
          ),
        ),
      )
      : null,
        floatingActionButtonLocation: FloatingActionButtonLocation.endFloat,
    );
  }
}
