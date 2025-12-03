import 'package:flutter/material.dart';

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
  final Map<String, dynamic> pot;

  const PotCard({super.key, required this.pot});

  @override
  Widget build(BuildContext context) {
    final data = pot['data'];

    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
      elevation: 3,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
      ),
      clipBehavior: Clip.antiAlias,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          AspectRatio(
            aspectRatio: 16 / 9,
            child: Image.asset(
              'assets/images/test_pot.png',
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
            padding: const EdgeInsets.all(12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  "Doniczka: ${pot['potId']}",
                  style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
                const SizedBox(height: 8),
                _buildRow("Temperatura powietrza", "${data['air_temp']} °C"),
                _buildRow("Wilgotność powietrza", "${data['air_humidity']} %"),
                _buildRow("Ciśnienie", "${data['air_pressure']} hPa"),
                _buildRow("Wilgotność gleby", "${data['soil_moisture']} %"),
                _buildRow("Oświetlenie", "${data['illuminance']} lx"),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label),
          Text(value, style: const TextStyle(fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }
}