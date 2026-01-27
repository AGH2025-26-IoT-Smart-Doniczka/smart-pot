import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/widgets/alerts_container.dart';
import 'package:smart_pot_mobile_app/screens/plants/pot_detail_screen.dart';
import 'package:smart_pot_mobile_app/widgets/pot_card.dart';

class HomeScreen extends StatefulWidget {
  final VoidCallback onShowPlants;
  const HomeScreen({super.key, required this.onShowPlants});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  Widget build(BuildContext context) {
    return Consumer<PotsController>(
      builder: (context, ctrl, child) {
        const double floatingButtonZoneHeight = 120;
        final double bottomPadding =
            floatingButtonZoneHeight +
            MediaQuery.of(context).padding.bottom +
            16;
        return SingleChildScrollView(
          padding: EdgeInsets.fromLTRB(16, 16, 16, bottomPadding),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                "Twój własny Smart Ogród",
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
                        final hasMeasurement =
                            pot.timeStamp != 'Time not specified';
                        return Padding(
                          padding: EdgeInsets.only(
                            right: index == ctrl.pots.length - 1 ? 0 : 12,
                          ),
                          // TODO: trzeba jeszcze określić co to ma wyświetlać
                          child: PotMiniCard(
                            statusEmoji: pot.data.soilMoisture < 30
                                ? '⚠️'
                                : '😊',
                            description: hasMeasurement
                                ? "Temp: ${pot.data.airTemp.toStringAsFixed(1)}°C"
                                : "Brak pomiaru",
                            title: pot.name,
                            imageUrl: "assets/images/test_pot.png",
                            hasMeasurement: hasMeasurement,
                            onTap: () {
                              Navigator.push(
                                context,
                                MaterialPageRoute(
                                  builder: (context) => PotDetailScreen(pot: pot),
                                ),
                              );
                            },
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
              SizedBox(height: floatingButtonZoneHeight),
            ],
          ),
        );
      },
    );
  }
}
