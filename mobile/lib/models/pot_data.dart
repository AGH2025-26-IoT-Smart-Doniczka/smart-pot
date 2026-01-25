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

class HumidityRange {
  final int min;
  final int optMin;
  final int optMax;
  final int max;

  const HumidityRange({
    required this.min,
    required this.optMin,
    required this.optMax,
    required this.max,
  });

  factory HumidityRange.fromJson(Map<dynamic, dynamic> json) {
    return HumidityRange(
      min: (json['min'] ?? json['very_low'] ?? 10) as int,
      optMin: (json['opt_min'] ?? json['low'] ?? 35) as int,
      optMax: (json['opt_max'] ?? json['high'] ?? 60) as int,
      max: (json['max'] ?? json['very_high'] ?? 90) as int,
    );
  }

  Map<String, dynamic> toJson() => {
    'min': min,
    'opt_min': optMin,
    'opt_max': optMax,
    'max': max,
  };
}

class PotConfig {
  final String potName;
  final int measureIntervalSec;
  final int sendIntervalSec;
  final int? wateringIntervalSec;
  final double maxTemp;
  final double minTemp;
  final HumidityRange humidity;
  final String illuminance;

  const PotConfig({
    required this.potName,
    required this.measureIntervalSec,
    required this.sendIntervalSec,
    required this.wateringIntervalSec,
    required this.maxTemp,
    required this.minTemp,
    required this.humidity,
    required this.illuminance,
  });

  factory PotConfig.fromJson(Map<dynamic, dynamic>? json, String fallbackName) {
    final cfg = json ?? {};
    return PotConfig(
      potName: (cfg['pot_name'] ?? fallbackName) as String,
      measureIntervalSec: (cfg['measure_interval_sec'] ?? 300) as int,
      sendIntervalSec: (cfg['send_interval_sec'] ?? 300) as int,
      wateringIntervalSec: cfg['watering_interval_sec'] as int?,
      maxTemp: (cfg['max_temp'] ?? 30.0).toDouble(),
      minTemp: (cfg['min_temp'] ?? 10.0).toDouble(),
      humidity: HumidityRange.fromJson(
        (cfg['humidity'] as Map<dynamic, dynamic>?) ?? const {},
      ),
      illuminance: (cfg['illuminance'] ?? 'medium') as String,
    );
  }

  Map<String, dynamic> toApiJson({String? overrideName}) => {
    'pot_name': overrideName ?? potName,
    'measure_interval_sec': measureIntervalSec,
    'send_interval_sec': sendIntervalSec,
    'watering_interval_sec': wateringIntervalSec,
    'max_temp': maxTemp,
    'min_temp': minTemp,
    'humidity': humidity.toJson(),
    'illuminance': illuminance,
  };
}

class Pot {
  final String id;
  final String potId;
  final String timeStamp;
  final PotData data;
  final String userId; // Nie wiem czy to konieczne
  final String name;
  final PotConfig config;

  Pot({
    required this.id,
    required this.potId,
    required this.timeStamp,
    required this.data,
    required this.userId,
    required this.name,
    required this.config,
  });

  factory Pot.fromJson(Map<String, dynamic> json) {
    final potId = json['pot_id'] ?? json['potId'] ?? json['id'] ?? "UNKNOWN";
    final lastMeasure = json['last_measure'] ?? json['data'] ?? {};
    final lastMeasureMap = lastMeasure is Map<String, dynamic>
        ? lastMeasure
        : {};
    final name = json['name'] ?? potId;
    final config = PotConfig.fromJson(
      json['config'] as Map<String, dynamic>?,
      name,
    );

    return Pot(
      id: json['id'] ?? potId,
      potId: potId,
      name: name,
      timeStamp:
          lastMeasureMap['timestamp'] ??
          json['timestamp'] ??
          "Time not specified",
      data: PotData.fromJson(lastMeasureMap),
      userId: json['user_id'] ?? json['userPublicKey'] ?? '',
      config: config,
    );
  }
}
