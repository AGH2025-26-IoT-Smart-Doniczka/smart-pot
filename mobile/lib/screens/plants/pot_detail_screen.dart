import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';


class PotDetailScreen extends StatelessWidget {
  final Pot pot;
  const PotDetailScreen({super.key, required this.pot});

  @override
  Widget build(BuildContext context) {
    final isHappy = pot.data.soilMoisture > 30;

    return Scaffold(
      appBar: AppBar(
        title: Text(pot.name),
        actions: [
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () {
              // TODO: Nawigacja do ustawień doniczki
            },
          ),
        ],
      ),
      body: SingleChildScrollView(
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Sekcja z głównym statusem rośliny
            Container(
              padding: const EdgeInsets.all(24),
              decoration: BoxDecoration(
                gradient: LinearGradient(
                  colors: isHappy
                    ? [Colors.green.shade300, Colors.green.shade500]
                    : [Colors.orange.shade300, Colors.orange.shade500],
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
              ),
              child: Column(
                children: [
                  Text(
                    isHappy ? '🌿' : '🥀',
                    style: const TextStyle(fontSize: 80),
                  ),
                  const SizedBox(height: 16),
                  Text(
                    isHappy ? 'Roślina czuje się świetnie!' : 'Roślina potrzebuje uwagi',
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Ostatnia aktualizacja: ${pot.timeStamp}',
                    style: const TextStyle(
                      fontSize: 14,
                      color: Colors.white70,
                    ),
                  ),
                ],
              ),
            ),

            // Sekcja z parametrami
            Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Parametry',
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 16),

                  // Temperatura powietrza
                  _buildParameterCard(
                    context,
                    icon: Icons.thermostat,
                    iconColor: Colors.red,
                    title: 'Temperatura powietrza',
                    value: '${pot.data.airTemp.toStringAsFixed(1)}°C',
                    description: _getTemperatureDescription(pot.data.airTemp),
                  ),
                  const SizedBox(height: 12),

                  // Wilgotność powietrza
                  _buildParameterCard(
                    context,
                    icon: Icons.water,
                    iconColor: Colors.lightBlue,
                    title: 'Wilgotność powietrza',
                    value: '${pot.data.airHumidity.toStringAsFixed(1)}%',
                    description: _getHumidityDescription(pot.data.airHumidity),
                  ),
                  const SizedBox(height: 12),

                  // Ciśnienie
                  _buildParameterCard(
                    context,
                    icon: Icons.compress,
                    iconColor: Colors.purple,
                    title: 'Ciśnienie',
                    value: '${pot.data.airPressure.toStringAsFixed(1)} hPa',
                    description: _getPressureDescription(pot.data.airPressure),
                  ),
                  const SizedBox(height: 12),

                  // Wilgotność gleby
                  _buildParameterCard(
                    context,
                    icon: Icons.water_drop,
                    iconColor: pot.data.soilMoisture > 30 ? Colors.blue : Colors.orange,
                    title: 'Wilgotność gleby',
                    value: '${pot.data.soilMoisture.toStringAsFixed(0)}%',
                    description: _getSoilMoistureDescription(pot.data.soilMoisture),
                  ),
                  const SizedBox(height: 12),

                  // Natężenie światła
                  _buildParameterCard(
                    context,
                    icon: Icons.wb_sunny,
                    iconColor: Colors.yellow,
                    title: 'Natężenie światła',
                    value: '${pot.data.illuminance.toStringAsFixed(0)} lx',
                    description: _getIlluminanceDescription(pot.data.illuminance),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildParameterCard(
    BuildContext context, {
    required IconData icon,
    required Color iconColor,
    required String title,
    required String value,
    required String description,
  }) {
    return Card(
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: iconColor.withOpacity(0.2),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(icon, color: iconColor, size: 32),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: Colors.grey.shade600,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    value,
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    description,
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.grey.shade500,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  String _getTemperatureDescription(double temp) {
    if (temp < 15) return 'Za zimno dla większości roślin';
    if (temp < 20) return 'Chłodno, ale akceptowalne';
    if (temp < 25) return 'Optymalna temperatura';
    if (temp < 30) return 'Ciepło, ale w normie';
    return 'Za gorąco, rozważ przeniesienie';
  }

  String _getHumidityDescription(double humidity) {
    if (humidity < 30) return 'Bardzo sucho';
    if (humidity < 40) return 'Niska wilgotność';
    if (humidity < 60) return 'Optymalna wilgotność';
    if (humidity < 70) return 'Podwyższona wilgotność';
    return 'Bardzo wysoka wilgotność';
  }

  String _getPressureDescription(double pressure) {
    if (pressure < 1000) return 'Niskie ciśnienie';
    if (pressure < 1020) return 'Normalne ciśnienie';
    return 'Wysokie ciśnienie';
  }

  String _getSoilMoistureDescription(double moisture) {
    if (moisture < 20) return 'Gleba bardzo sucha - podlej roślinę!';
    if (moisture < 30) return 'Gleba sucha - wkrótce podlej';
    if (moisture < 60) return 'Optymalna wilgotność gleby';
    if (moisture < 80) return 'Gleba wilgotna';
    return 'Gleba bardzo mokra - uważaj na przelanie!';
  }

  String _getIlluminanceDescription(double illuminance) {
    if (illuminance < 100) return 'Bardzo mało światła';
    if (illuminance < 500) return 'Słabe oświetlenie';
    if (illuminance < 1000) return 'Umiarkowane oświetlenie';
    if (illuminance < 5000) return 'Dobre oświetlenie';
    return 'Bardzo jasno - pełne słońce';
  }
}
