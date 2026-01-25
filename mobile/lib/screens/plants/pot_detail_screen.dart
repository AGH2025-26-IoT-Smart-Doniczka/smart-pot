import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/screens/plants/pot_config_screen.dart';

class PotDetailScreen extends StatelessWidget {
  final Pot pot;
  const PotDetailScreen({super.key, required this.pot});

  @override
  Widget build(BuildContext context) {
    final currentPot = context.select<PotsController, Pot?>(
      (ctrl) =>
          ctrl.pots.firstWhere((p) => p.potId == pot.potId, orElse: () => pot),
    );

    final viewPot = currentPot ?? pot;

    final role = viewPot.role;
    final hasMeasurement = viewPot.timeStamp != 'Time not specified';
    final isHappy = hasMeasurement && viewPot.data.soilMoisture > 30;
    final measureInterval = viewPot.config.measureIntervalSec;
    final sendInterval = viewPot.config.sendIntervalSec;
    final wateringInterval = viewPot.config.wateringIntervalSec;

    return Scaffold(
      appBar: AppBar(
        title: Text(viewPot.name),
        actions: [
          if (role == PotRole.editor || role == PotRole.owner)
            IconButton(
              icon: const Icon(Icons.settings),
              onPressed: () async {
                await Navigator.of(context).push(
                  MaterialPageRoute(
                    builder: (context) => PotConfigScreen(pot: viewPot),
                  ),
                );
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
                  colors: hasMeasurement
                      ? (isHappy
                            ? [Colors.green.shade300, Colors.green.shade500]
                            : [Colors.orange.shade300, Colors.orange.shade500])
                      : [Colors.grey.shade400, Colors.grey.shade600],
                  begin: Alignment.topLeft,
                  end: Alignment.bottomRight,
                ),
              ),
              child: Column(
                children: [
                  SizedBox(
                    width: 160,
                    child: AspectRatio(
                      aspectRatio: 1.0,
                      child: Container(
                        decoration: BoxDecoration(
                          color: Colors.white,
                          borderRadius: BorderRadius.circular(24),
                          boxShadow: [
                            BoxShadow(
                              color: Colors.black.withValues(alpha: 0.1),
                              blurRadius: 4,
                              offset: Offset(0, 2),
                            ),
                          ],
                        ),
                        child: Center(
                          child: Text(
                            hasMeasurement ? (isHappy ? '🌿' : '🥀') : '🌱',
                            style: const TextStyle(fontSize: 80),
                          ),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(height: 16),
                  Text(
                    hasMeasurement
                        ? (isHappy
                              ? 'Roślina czuje się świetnie!'
                              : 'Roślina potrzebuje uwagi')
                        : 'Roślina czeka na pierwszy pomiar parametrów',
                    style: const TextStyle(
                      fontSize: 20,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Ostatni pomiar: ${hasMeasurement ? viewPot.timeStamp : 'Brak'}',
                    style: const TextStyle(fontSize: 14, color: Colors.white70),
                  ),
                  const SizedBox(height: 16),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      _buildConfigInfoChip(
                        context,
                        icon: Icons.sensors,
                        label: _formatInterval(measureInterval),
                      ),
                      const SizedBox(width: 12),
                      _buildConfigInfoChip(
                        context,
                        icon: Icons.cloud_upload,
                        label: _formatInterval(sendInterval),
                      ),
                      if (wateringInterval != null) ...[
                        const SizedBox(width: 12),
                        _buildConfigInfoChip(
                          context,
                          icon: Icons.water_drop,
                          label: _formatInterval(wateringInterval),
                        ),
                      ],
                    ],
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
                  hasMeasurement
                      ? _buildParametersGrid(context)
                      : Text(
                          'Brak pomiaru',
                          style: Theme.of(context).textTheme.bodyMedium
                              ?.copyWith(color: Colors.grey.shade500),
                        ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildConfigInfoChip(
    BuildContext context, {
    required IconData icon,
    required String label,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
      decoration: BoxDecoration(
        color: Colors.white.withOpacity(0.2),
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        children: [
          Icon(icon, color: Colors.white, size: 18),
          const SizedBox(width: 6),
          Text(
            label,
            style: const TextStyle(
              color: Colors.white,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  String _formatInterval(int seconds) {
    const int minute = 60;
    const int hour = 60 * minute;
    const int day = 24 * hour;

    if (seconds < minute) return '${seconds}s';
    if (seconds < hour) {
      final minutes = seconds ~/ minute;
      return '${minutes}m';
    }
    if (seconds < day) {
      final hours = seconds ~/ hour;
      return '${hours}h';
    }
    final days = seconds ~/ day;
    return '${days}d';
  }

  Widget _buildParametersGrid(BuildContext context) {
    const double minCardWidth = 450;
    const double spacing = 12;

    final cards = [
      _buildParameterCard(
        context,
        icon: Icons.thermostat,
        iconColor: Colors.red,
        title: 'Temperatura powietrza',
        value: '${pot.data.airTemp.toStringAsFixed(1)}°C',
        description: _getTemperatureDescription(pot.data.airTemp),
      ),
      _buildParameterCard(
        context,
        icon: Icons.compress,
        iconColor: Colors.purple,
        title: 'Ciśnienie',
        value: '${pot.data.airPressure.toStringAsFixed(1)} hPa',
        description: _getPressureDescription(pot.data.airPressure),
      ),
      _buildParameterCard(
        context,
        icon: Icons.water_drop,
        iconColor: pot.data.soilMoisture > 30 ? Colors.blue : Colors.orange,
        title: 'Wilgotność gleby',
        value: '${pot.data.soilMoisture.toStringAsFixed(0)}%',
        description: _getSoilMoistureDescription(pot.data.soilMoisture),
      ),
      _buildParameterCard(
        context,
        icon: Icons.wb_sunny,
        iconColor: Colors.yellow,
        title: 'Natężenie światła',
        value: '${pot.data.illuminance.toStringAsFixed(0)} lx',
        description: _getIlluminanceDescription(pot.data.illuminance),
      ),
    ];

    return LayoutBuilder(
      builder: (context, constraints) {
        final columns = (constraints.maxWidth / minCardWidth)
            .floor()
            .clamp(1, cards.length)
            .toInt();
        final totalSpacing = spacing * (columns - 1);
        final cardWidth = (constraints.maxWidth - totalSpacing) / columns;

        return Wrap(
          spacing: spacing,
          runSpacing: spacing,
          children: cards
              .map((card) => SizedBox(width: cardWidth, child: card))
              .toList(),
        );
      },
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
