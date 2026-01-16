import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/theme/theme_controller.dart';
import 'package:smart_pot_mobile_app/widgets/alerts_container.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class HomeScreen extends StatefulWidget {
  final VoidCallback onShowPlants;
  const HomeScreen({super.key, required this.onShowPlants});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  void initState() {
    // TODO: implement initState
    super.initState();
    // WidgetsBinding.instance.addPostFrameCallback((_) {
    //   context.read<PotsController>().fetchPots();
    // });
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<PotsController>(
      builder: (context, ctrl, child) {
        return SingleChildScrollView(
          padding: EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                "Twój właśny Smart Ogród",
                style: Theme.of(context).textTheme.headlineSmall,
              ),
              Align(
                alignment: Alignment.centerRight,
                child: TextButton(
                  onPressed: widget.onShowPlants,
                  child: Text("Wszystkie doniczki"),
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
                      return const Center(
                        child: Text("Brak doniczek. Dodaj pierwszą!"),
                      );
                    }
                    return ListView.builder(
                      scrollDirection: Axis.horizontal,
                      itemCount: ctrl.pots.length,
                      itemBuilder: (context, index) {
                        final pot = ctrl.pots[index];
                        return Padding(
                          padding: EdgeInsets.only(
                            right: index == ctrl.pots.length - 1 ? 0 : 12,
                          ),
                          // TODO: trzeba jeszcze określić co to ma wyświetlać
                          child: PotMiniCard(
                            statusEmoji: pot.data.soilMoisture < 30
                                ? '⚠️'
                                : '😊',
                            description:
                                "Temp: ${pot.data.airTemp.toStringAsFixed(1)}°C",
                            title: pot.potId ?? 'Pot',
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
      },
    );
  }
}
