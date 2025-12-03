import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';
import 'package:smart_pot_mobile_app/widgets/alerts_container.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class HomeScreen extends StatelessWidget {
  final VoidCallback onShowPlants;
  const HomeScreen({super.key, required this.onShowPlants});

  @override
  Widget build(BuildContext context) {
    final ctrl = context.watch<PotsController>();

    return SingleChildScrollView(
      padding: EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            "Your own Pots Ecosystem",
            style: Theme.of(context).textTheme.headlineSmall,
          ),
          Align(
            alignment: Alignment.centerRight,
            child: ElevatedButton(
              onPressed: onShowPlants,
              child: Text("All Your Pots"),
            ),
          ),
          // Lista obrazków doniczek
          SizedBox(
            height: 200,
            child: Builder(
              builder: (_) {
                if (ctrl.isLoading) {
                  return Center(child: CircularProgressIndicator());
                }
                if (ctrl.pots.isEmpty) {
                  return const Center(child: Text("No pots found"));
                }
                return ListView.builder(
                  scrollDirection: Axis.horizontal,
                  itemCount: ctrl.pots.length,
                  itemBuilder: (context, index) {
                    final pot = ctrl.pots[index] as Map<String, dynamic>;
                    final data = (pot['data'] ?? {}) as Map<String, dynamic>;
                    final soil =
                        double.tryParse('${data['soil_moisture']}') ?? 0.0;
                    final temp = double.tryParse('${data['air_temp']}') ?? 0.0;

                    return Padding(
                      padding: EdgeInsets.only(
                        right: index == ctrl.pots.length - 1 ? 0 : 12,
                      ),
                      child: PotMiniCard(
                        statusEmoji: soil < 30 ? '⚠️' : '😊',
                        description: "Temp: ${temp.toStringAsFixed(1)}°C",
                        title: pot['potId']??'Pot',
                        imageUrl: "assets/images/test_pot.png",
                      ),
                    );
                  },
                );
              },
            ),
          ),
          SizedBox(height: 20),
          Text("Alerty", style: Theme.of(context).textTheme.headlineSmall),
          SizedBox(height: 20),
          AlertsContainer(),
        ],
      ),
    );
  }
}
