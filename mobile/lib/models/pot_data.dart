class PotData {
  // final String potId;
  // final String timeStamp;
  final double airTemp;
  final double airPressure;
  final double soilMoisture;
  final double illuminance;

  PotData({
    this.airTemp = 0.0,
    this.airPressure = 0.0,
    this.soilMoisture = 0.0,
    this.illuminance = 0.0,
  });

  // TODO: Domyślne wartości chyba nie powinny być '0'
  factory PotData.fromJson(Map<dynamic, dynamic> json) {
    return PotData(
      airTemp: (json['air_temp'] ?? 0).toDouble(),
      airPressure: (json['air_pressure'] ?? 0).toDouble(),
      soilMoisture: (json['soil_moisture'] ?? 0).toDouble(),
      illuminance: (json['illuminance'] ?? 0).toDouble(),
    );
  }

  // Metoda ta przyda się przy wysyłaniu danych
  Map<String, dynamic> toJson() => {
    'air_temp': '$airTemp',
    'air_pressure': '$airPressure',
    'soil_moisture': '$soilMoisture',
    'illuminance': '$illuminance',
  };
}

enum PotRole { viewer, editor, owner, unknown }

PotRole potRoleFromString(String? value) {
  if (value == null) return PotRole.unknown;
  final normalized = value.trim().toLowerCase();
  final role = normalized.contains('.')
      ? normalized.split('.').last
      : normalized;
  switch (role) {
    case 'viewer':
      return PotRole.viewer;
    case 'editor':
      return PotRole.editor;
    case 'owner':
      return PotRole.owner;
    default:
      return PotRole.unknown;
  }
}

String potRoleToString(PotRole role) {
  switch (role) {
    case PotRole.viewer:
      return 'viewer';
    case PotRole.editor:
      return 'editor';
    case PotRole.owner:
      return 'owner';
    case PotRole.unknown:
      return 'unknown';
  }
}

class PotConnection {
  final String id;
  final String userId;
  final String email;
  final PotRole role;

  const PotConnection({
    required this.id,
    required this.userId,
    required this.email,
    required this.role,
  });

  factory PotConnection.fromJson(Map<String, dynamic> json) {
    return PotConnection(
      id: (json['id'] ?? json['connection_id'] ?? '').toString(),
      userId: (json['user_id'] ?? json['userId'] ?? '').toString(),
      email: (json['email'] ?? json['user_email'] ?? '').toString(),
      role: potRoleFromString(json['role']?.toString()),
    );
  }
}

class PotConfig {
  final String potName;
  final int measureIntervalSec;
  final int sendIntervalSec;
  final int? wateringIntervalSec;
  final double maxTemp;
  final double minTemp;
  final int minMoisture;
  final int maxMoisture;
  final String illuminance;

  const PotConfig({
    required this.potName,
    required this.measureIntervalSec,
    required this.sendIntervalSec,
    required this.wateringIntervalSec,
    required this.maxTemp,
    required this.minTemp,
    required this.minMoisture,
    required this.maxMoisture,
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
      minMoisture: (cfg['min_moisture'] ?? 0) as int,
      maxMoisture: (cfg['max_moisture'] ?? 0) as int,
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
    'min_moisture': minMoisture,
    'max_moisture': maxMoisture,
    'illuminance': illuminance,
  };
}

class Pot {
  final String id;
  final String potId;
  final String timeStamp;
  final PotData data;
  final String userId; // Nie wiem czy to konieczne
  final PotRole role;
  final String name;
  final PotConfig config;
  final List<PotConnection> connections;

  Pot({
    required this.id,
    required this.potId,
    required this.timeStamp,
    required this.data,
    required this.userId,
    required this.role,
    required this.name,
    required this.config,
    required this.connections,
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
    final connectionsJson = (json['connections'] as List<dynamic>?) ?? const [];
    final connections = connectionsJson
        .whereType<Map>()
        .map((item) => PotConnection.fromJson(item.cast<String, dynamic>()))
        .toList();

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
      role: potRoleFromString(json['role']?.toString()),
      config: config,
      connections: connections,
    );
  }
}
