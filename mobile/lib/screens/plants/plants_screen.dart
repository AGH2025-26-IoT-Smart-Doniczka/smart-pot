import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class MyPotsScreen extends StatelessWidget {
  const MyPotsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<PotsController>();
    final List<Pot> pots = ctrl.pots;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Moje rośliny'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () => context.read<PotsController>().fetchPots(),
          )
        ],
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
                        // Tutaj w przyszłości nawigacja do parowania
                        Navigator.pushNamed(context, '/scan');
                      },
                      child: const Text("Dodaj doniczkę")
                  )
                ],
              ),
            );
          }

          return ListView.separated(
            padding: const EdgeInsets.all(16),
            itemCount: pots.length,
            separatorBuilder: (_, __) => const SizedBox(height: 12),
            itemBuilder: (context, index) {
              // 3. Przekazujemy cały obiekt Pot do karty
              return PotCard(pot: pots[index]);
            },
          );
        },
      ),
    );
  }
}

