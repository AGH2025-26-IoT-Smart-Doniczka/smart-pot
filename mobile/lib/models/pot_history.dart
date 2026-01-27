class MetricAggregate {
  final double avg;
  final double min;
  final double max;

  const MetricAggregate({
    required this.avg,
    required this.min,
    required this.max,
  });

  factory MetricAggregate.fromJson(Map<String, dynamic> json) {
    return MetricAggregate(
      avg: (json['avg'] ?? 0).toDouble(),
      min: (json['min'] ?? 0).toDouble(),
      max: (json['max'] ?? 0).toDouble(),
    );
  }
}

class PotHistoryPoint {
  final DateTime timestamp;
  final MetricAggregate airTemp;
  final MetricAggregate airPressure;
  final MetricAggregate soilMoisture;
  final MetricAggregate illuminance;

  const PotHistoryPoint({
    required this.timestamp,
    required this.airTemp,
    required this.airPressure,
    required this.soilMoisture,
    required this.illuminance,
  });

  factory PotHistoryPoint.fromJson(Map<String, dynamic> json) {
    final data = (json['data'] as Map<String, dynamic>?) ?? const {};
    return PotHistoryPoint(
      timestamp: DateTime.parse(json['timestamp'] as String),
      airTemp: MetricAggregate.fromJson(
        (data['air_temp'] as Map<String, dynamic>?) ?? const {},
      ),
      airPressure: MetricAggregate.fromJson(
        (data['air_pressure'] as Map<String, dynamic>?) ?? const {},
      ),
      soilMoisture: MetricAggregate.fromJson(
        (data['soil_moisture'] as Map<String, dynamic>?) ?? const {},
      ),
      illuminance: MetricAggregate.fromJson(
        (data['illuminance'] as Map<String, dynamic>?) ?? const {},
      ),
    );
  }
}
