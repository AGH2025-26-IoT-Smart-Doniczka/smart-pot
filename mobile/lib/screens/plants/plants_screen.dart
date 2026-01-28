import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class MyPotsScreen extends StatelessWidget {
  const MyPotsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<PotsController>();
    final List<Pot> pots = ctrl.pots;
    final activePots = pots.where((p) => p.isActive).toList();
    final inactivePots = pots.where((p) => !p.isActive).toList();

    return DefaultTabController(
      length: 2,
      child: Scaffold(
        appBar: AppBar(
          title: const Text('Moje rośliny'),
          actions: [
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: () => {context.read<PotsController>().fetchPots()},
            ),
          ],
          bottom: const TabBar(
            tabs: [
              Tab(text: 'Aktywne'),
              Tab(text: 'Archiwum'),
            ],
          ),
        ),
        body: Builder(
          builder: (_) {
            if (ctrl.isLoading) {
              return const Center(child: CircularProgressIndicator());
            }
            if (ctrl.error != null) {
              return Center(child: Text(ctrl.error!));
            }
            if (pots.isEmpty) {
              return Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const Text('Nie masz jeszcze żadnych roślin.'),
                    const SizedBox(height: 10),
                    ElevatedButton(
                      onPressed: () {
                        Navigator.pushNamed(context, '/new_plant');
                      },
                      child: const Text("Dodaj doniczkę"),
                    ),
                  ],
                ),
              );
            }

            return TabBarView(
              children: [
                _buildPotList(
                  context,
                  activePots,
                  emptyMessage: 'Brak aktywnych doniczek.',
                  showAddButton: true,
                ),
                _buildPotList(
                  context,
                  inactivePots,
                  emptyMessage: 'Brak doniczek w archiwum.',
                  showAddButton: false,
                ),
              ],
            );
          },
        ),
      ),
    );
  }
}

Widget _buildPotList(
  BuildContext context,
  List<Pot> pots, {
  required String emptyMessage,
  required bool showAddButton,
}) {
  if (pots.isEmpty) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(emptyMessage),
          if (showAddButton) ...[
            const SizedBox(height: 10),
            ElevatedButton(
              onPressed: () => Navigator.pushNamed(context, '/new_plant'),
              child: const Text("Dodaj doniczkę"),
            ),
          ],
        ],
      ),
    );
  }

  return ListView.separated(
    padding: const EdgeInsets.all(16),
    itemCount: pots.length,
    separatorBuilder: (_, __) => const SizedBox(height: 12),
    itemBuilder: (context, index) => PotCard(pot: pots[index]),
  );
}
