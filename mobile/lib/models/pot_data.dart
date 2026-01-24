class PotData {
  // final String potId;
  // final String timeStamp;
  final double airTemp;
  final double airHumidity;
  final double airPressure;
  final double soilMoisture;
  final double illuminance;

  PotData({
    this.airHumidity = 0.0,
    this.airTemp = 0.0,
    this.airPressure = 0.0,
    this.soilMoisture = 0.0,
    this.illuminance = 0.0,
  });

  // TODO: Domyślne wartości chyba nie powinny być '0'
  factory PotData.fromJson(Map<dynamic, dynamic> json) {
    return PotData(
      airTemp: (json['air_temp'] ?? 0).toDouble(),
      airHumidity: (json['air_humidity'] ?? 0).toDouble(),
      airPressure: (json['air_pressure'] ?? 0).toDouble(),
      soilMoisture: (json['soil_moisture'] ?? 0).toDouble(),
      illuminance: (json['illuminance'] ?? 0).toDouble(),
    );
  }

  // Metoda ta przyda się przy wysyłaniu danych
  Map<String, dynamic> toJson() => {
    'air_temp': '$airTemp',
    'air_humidity': '$airHumidity',
    'air_pressure': '$airPressure',
    'soil_moisture': '$soilMoisture',
    'illuminance': '$illuminance',
  };
}

class Pot {
  final String id;
  final String potId;
  final String timeStamp;
  final PotData data;
  final String userId; // Nie wiem czy to konieczne
  final String name;

  Pot({
    required this.id,
    required this.potId,
    required this.timeStamp,
    required this.data,
    required this.userId,
    required this.name,
  });

  factory Pot.fromJson(Map<String, dynamic> json) {
    final potId = json['pot_id'] ?? json['potId'] ?? json['id'] ?? "UNKNOWN";
    final lastMeasure = json['last_measure'] ?? json['data'] ?? {};
    final lastMeasureMap = lastMeasure is Map<String, dynamic>
        ? lastMeasure
        : {};

    return Pot(
      id: json['id'] ?? potId,
      potId: potId,
      name: json['name'] ?? potId,
      timeStamp:
          lastMeasureMap['timestamp'] ??
          json['timestamp'] ??
          "Time not specified",
      data: PotData.fromJson(lastMeasureMap),
      userId: json['user_id'] ?? json['userPublicKey'] ?? '',
    );
  }
}
