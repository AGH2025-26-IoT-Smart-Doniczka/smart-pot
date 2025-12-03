import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class MyPotsScreen extends StatelessWidget {
  const MyPotsScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<PotsController>();
    final pots = ctrl.pots;

    return Builder(builder: (_){
      if (ctrl.isLoading){
        return const Center(child: CircularProgressIndicator());
      }
      if(pots.isEmpty){
        return const Center(child: Text('No pots found'));
      }

      return ListView.separated(
        padding: EdgeInsets.symmetric(vertical: 8),
        itemBuilder: (context, index) {
          return PotCard(pot: pots[index]);
        },
        separatorBuilder: (_, __) => const SizedBox(height: 4),
        itemCount: pots.length,
      );
    });
  }
}

