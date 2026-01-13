import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/screens/plants/pot_detail_screen.dart';

class PotMiniCard extends StatelessWidget {
  final String statusEmoji;
  final String description;
  final String title;
  final String imageUrl;

  const PotMiniCard({
    super.key,
    required this.statusEmoji,
    required this.description,
    required this.title,
    required this.imageUrl,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 160,
      child: Card(
        clipBehavior: Clip.antiAlias,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            AspectRatio(
              aspectRatio: 16 / 9,
              child: Image.asset(
                imageUrl,
                fit: BoxFit.cover,
                errorBuilder: (c, _, __) => Container(
                  color: Colors.grey.shade800,
                  child: const Center(
                    child: Icon(Icons.local_florist, size: 48),
                  ),
                ),
              ),
            ),
            Padding(
              padding: EdgeInsets.all(8),
              child: Row(
                children: [
                  Text(statusEmoji, style: TextStyle(fontSize: 20)),
                  SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      title,
                      style: Theme.of(context).textTheme.titleMedium,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                ],
              ),
            ),
            Padding(
              padding: EdgeInsets.symmetric(horizontal: 8),
              child: Text(
                description,
                style: Theme.of(context).textTheme.bodySmall,
                overflow: TextOverflow.ellipsis,
              ),
            ),
            SizedBox(height: 8),
          ],
        ),
      ),
    );
  }
}


class PotCard extends StatelessWidget {
  final Pot pot; // Teraz widget przyjmuje silnie typowany obiekt

  const PotCard({super.key, required this.pot});

  @override
  Widget build(BuildContext context) {
    // Wyciągamy dane dla wygody
    final temp = pot.data.airTemp.toStringAsFixed(1);
    final moisture = pot.data.soilMoisture.toStringAsFixed(0);
    final isHappy = pot.data.soilMoisture > 30;

    return GestureDetector(
      onTap: () {
        Navigator.push(
          context,
          MaterialPageRoute(
            builder: (context) => PotDetailScreen(pot: pot),
          ),
        );
      },
      child: Card(
        elevation: 4,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Row(
            children: [
              // Ikona / Obrazek
              Container(
                width: 60,
                height: 60,
                decoration: BoxDecoration(
                  color: isHappy ? Colors.green.shade100 : Colors.orange.shade100,
                  shape: BoxShape.circle,
                ),
                child: Center(
                  child: Text(
                    isHappy ? '🌿' : '🥀',
                    style: const TextStyle(fontSize: 30),
                  ),
                ),
              ),
              const SizedBox(width: 16),
              // Teksty
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      pot.name,
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text("ID: ${pot.potId}", style: Theme.of(context).textTheme.bodySmall),
                  ],
                ),
              ),
              // Parametry po prawej
              Column(
                crossAxisAlignment: CrossAxisAlignment.end,
                children: [
                  Row(
                    children: [
                      const Icon(Icons.thermostat, size: 16, color: Colors.grey),
                      const SizedBox(width: 4),
                      Text("$temp°C"),
                    ],
                  ),
                  const SizedBox(height: 4),
                  Row(
                    children: [
                      Icon(
                          Icons.water_drop,
                          size: 16,
                          color: isHappy ? Colors.blue : Colors.orange
                      ),
                      const SizedBox(width: 4),
                      Text("$moisture%"),
                    ],
                  ),
                ],
              )
            ],
          ),
        ),
      ),
    );
  }
}