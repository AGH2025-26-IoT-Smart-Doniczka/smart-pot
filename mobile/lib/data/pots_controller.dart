import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/models/alert_model.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/models/pot_history.dart';
import 'package:smart_pot_mobile_app/services/ble_service.dart';
import 'package:http/http.dart' as http;

import '../config/app_config.dart';

class PotsController extends ChangeNotifier {
  final AuthController _authController;

  static final String _baseUrl = AppConfig.baseUrl;

  List<Pot> _pots = [];
  bool _isLoading = false;
  String? _error;
  List<Alert> _alerts = [];
  bool _isAlertsLoading = false;
  String? _alertsError;

  List<Pot> get pots => _pots;
  bool get isLoading => _isLoading;
  String? get error => _error;
  List<Alert> get alerts => _alerts;
  bool get isAlertsLoading => _isAlertsLoading;
  String? get alertsError => _alertsError;

  PotsController(this._authController);

  AlertType _mapAlertLevel(int level) {
    switch (level) {
      case 1:
        return AlertType.info;
      case 2:
        return AlertType.warning;
      case 3:
      case 4:
      default:
        return AlertType.error;
    }
  }

  Future<void> fetchAlerts({int count = 20}) async {
    final user = _authController.currentUser;
    if (user == null) {
      _alertsError = "Użytkownik nie jest zalogowany";
      _alerts = [];
      notifyListeners();
      return;
    }

    _isAlertsLoading = true;
    _alertsError = null;
    notifyListeners();

    try {
      final url = Uri.parse('$_baseUrl/pots/logs?count=$count');
      final response = await http.get(
        url,
        headers: {'Authorization': 'Bearer ${user.token}'},
      );
      if (response.statusCode != 200) {
        String details = '';
        try {
          final decoded = jsonDecode(response.body);
          if (decoded is Map<String, dynamic>) {
            details = decoded['detail']?.toString() ?? '';
          } else if (decoded is String) {
            details = decoded;
          }
        } catch (_) {
          details = response.body;
        }
        _alertsError = details.isNotEmpty
            ? "Błąd pobierania alertów: ${response.statusCode} ($details)"
            : "Błąd pobierania alertów: ${response.statusCode}";
        _alerts = [];
        return;
      }

      final decoded = jsonDecode(response.body);
      if (decoded is! Map<String, dynamic>) {
        throw Exception("Nieprawidłowa odpowiedź serwera");
      }
      final logs = (decoded['logs'] as List<dynamic>?) ?? const [];
      _alerts = logs
          .whereType<Map<String, dynamic>>()
          .where((log) => (log['label']?.toString() ?? '') == 'alert')
          .map((log) {
            final payload = log['payload'] as Map<String, dynamic>? ?? const {};
            final level = payload['lvl'] as int? ?? 4;
            final message = payload['data']?.toString() ?? '';
            final potName = log['pot_name']?.toString() ?? log['pot_id']?.toString() ?? '';
            final timestamp = log['timestamp']?.toString();
            final date = timestamp != null ? DateTime.parse(timestamp) : DateTime.now();
            return Alert(
              title: potName,
              description: message,
              dateTime: date,
              alertType: _mapAlertLevel(level),
            );
          })
          .toList();
    } catch (e) {
      _alertsError = "Błąd pobierania alertów: $e";
      _alerts = [];
    } finally {
      _isAlertsLoading = false;
      notifyListeners();
    }
  }

  Future<void> fetchPots() async {
    print("Fetching pots...");

    final user = _authController.currentUser;
    if (user == null) {
      _error = "Użytkownik nie jest zalogowany";
      _pots = [];
      notifyListeners();
      return;
    }

    _isLoading = true;
    _error = null;
    notifyListeners();

    try {
      final response = await http.get(
        Uri.parse('$_baseUrl/pots'),
        headers: {'Authorization': 'Bearer ${user.token}'},
      );
      if (response.statusCode != 200) {
        String details = '';
        try {
          final decoded = jsonDecode(response.body);
          if (decoded is Map<String, dynamic>) {
            details = decoded['detail']?.toString() ?? '';
          } else if (decoded is String) {
            details = decoded;
          }
        } catch (_) {
          details = response.body;
        }
        _error = details.isNotEmpty
            ? "Błąd pobierania danych doniczek: ${response.statusCode} ($details)"
            : "Błąd pobierania danych doniczek: ${response.statusCode}";
        return;
      }

      // Dekodujemy cały JSON
      final Map<String, dynamic> jsonData = jsonDecode(response.body);

      // Wyciągamy listę doniczek
      final List<dynamic> potsJson = jsonData['pots'] ?? [];

      // Mapujemy JSON na obiekty Pot
      _pots = potsJson.map((json) => Pot.fromJson(json)).toList();
    } catch (e) {
      _error = "Błąd pobierania danych doniczek: $e";
      _pots = [];
    } finally {
      _isLoading = false;
      notifyListeners();
    }
  }

  Future<Map<String, dynamic>> pairPotWithServer(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/pairing');

    try {
      final response = await http.post(
        url,
        headers: {
          'Content-Type': 'application/json',
          'Authorization': 'Bearer ${user.token}',
        },
        body: json.encode({"user_id": user.id}),
      );

      if (response.statusCode == 201 || response.statusCode == 200) {
        return json.decode(response.body);
      } else {
        final errorData = json.decode(response.body);
        throw Exception(
          errorData['detail'] ?? 'Błąd parowania: ${response.statusCode}',
        );
      }
    } catch (e) {
      throw Exception("Błąd połączenia z serwerem: $e");
    }
  }

  final _bleService = BleService();

  Future<void> updatePotConfig(
    String potId,
    Map<String, dynamic> payload,
  ) async {
    final existing = _pots.firstWhere(
      (pot) => pot.potId == potId,
      orElse: () => Pot(
        id: potId,
        potId: potId,
        timeStamp: 'Time not specified',
        data: PotData(),
        userId: '',
        role: PotRole.unknown,
        name: potId,
        config: PotConfig.fromJson(null, potId),
        connections: const [],
        isActive: true,
      ),
    );
    if (!existing.isActive) {
      throw Exception("Doniczka jest nieaktywna");
    }
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    // Hybrid Update: BLE first
    try {
      final isConnected = await _bleService.isPotConnected(potId);
      if (isConnected) {
        debugPrint("Pot $potId is connected via BLE. Updating directly...");
        final config = PotConfig.fromJson(payload, "Unknown");
        final firmwareJson = config.toFirmwareJson();
        await _bleService.writeCharacteristic(
          potId,
          BleService.CHARACTERISTICS_CONFIG_UUID,
          firmwareJson,
        );
        debugPrint("BLE update successful.");
      }
    } catch (e) {
      debugPrint("BLE update failed (fallback to HTTP): $e");
      // Continue to HTTP even if BLE fails
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/actions/config');
    final response = await http.post(
      url,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ${user.token}',
      },
      body: json.encode(payload),
    );

    if (response.statusCode != 202) {
      throw Exception("Błąd zapisu konfiguracji: ${response.statusCode}");
    }

    await fetchPots();
  }

  Future<void> renamePot(String potId, String? potName) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/name');
    final response = await http.patch(
      url,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ${user.token}',
      },
      body: json.encode({'pot_name': potName}),
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd zmiany nazwy: ${response.statusCode} ($details)"
            : "Błąd zmiany nazwy: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<void> disconnectPot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/pairing');
    final response = await http.delete(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200 && response.statusCode != 204) {
      throw Exception("Błąd rozłączania doniczki: ${response.statusCode}");
    }

    await fetchPots();
  }

  Future<void> hardResetPot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/hard-reset');
    final response = await http.post(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd twardego resetu: ${response.statusCode} ($details)"
            : "Błąd twardego resetu: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<void> notifyHardReset(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    // Zgodnie ze specyfikacją, używamy tego samego endpointu co przy ręcznym hard reset
    final url = Uri.parse('$_baseUrl/pots/$potId/hard-reset');

    try {
      final response = await http.post(
        url,
        headers: {'Authorization': 'Bearer ${user.token}'},
      );

      if (response.statusCode < 200 || response.statusCode >= 300) {
        debugPrint("Notify Hard Reset failed: ${response.statusCode}");
        // Nie rzucamy wyjątku, aby nie przerywać parowania, ale logujemy błąd
      } else {
        debugPrint("Notify Hard Reset successful for $potId");
      }
    } catch (e) {
      debugPrint("Notify Hard Reset error: $e");
    }
  }

  Future<void> disconnectResetPot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/disconnect-reset');
    final response = await http.post(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd rozłączania i resetu: ${response.statusCode} ($details)"
            : "Błąd rozłączania i resetu: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<List<PotHistoryPoint>> fetchPotHistory({
    required String potId,
    required DateTime from,
    required DateTime to,
    required String bucket,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final query = Uri(
      queryParameters: {
        'from': from.toUtc().toIso8601String(),
        'to': to.toUtc().toIso8601String(),
        'bucket': bucket,
      },
    );
    final url = Uri.parse(
      '$_baseUrl/pots/$potId/measures',
    ).replace(query: query.query);

    final response = await http.get(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd pobierania historii: ${response.statusCode} ($details)"
            : "Błąd pobierania historii: ${response.statusCode}",
      );
    }

    final decoded = jsonDecode(response.body);
    if (decoded is! Map<String, dynamic>) {
      throw Exception("Nieprawidłowa odpowiedź serwera");
    }

    final measures = (decoded['measures'] as List<dynamic>?) ?? const [];
    return measures
        .whereType<Map<String, dynamic>>()
        .map(PotHistoryPoint.fromJson)
        .toList();
  }

  Future<List<PotHistoryPoint>> fetchPotHistoryRecent({
    required String potId,
    required int count,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final query = Uri(
      queryParameters: {
        'count': count.toString(),
      },
    );
    final url = Uri.parse('$_baseUrl/pots/$potId/measures')
        .replace(query: query.query);

    final response = await http.get(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd pobierania historii: ${response.statusCode} ($details)"
            : "Błąd pobierania historii: ${response.statusCode}",
      );
    }

    final decoded = jsonDecode(response.body);
    if (decoded is! Map<String, dynamic>) {
      throw Exception("Nieprawidłowa odpowiedź serwera");
    }

    final measures = (decoded['measures'] as List<dynamic>?) ?? const [];
    final points = measures
        .whereType<Map<String, dynamic>>()
        .map((item) {
          final data = (item['data'] as Map<String, dynamic>?) ?? const {};
          double _asDouble(dynamic value) =>
              value == null ? 0 : (value as num).toDouble();

          final airTemp = _asDouble(data['tem']);
          final airPressure = _asDouble(data['pre']);
          final soilMoisture = _asDouble(data['moi']);
          final illuminance = _asDouble(data['lux']);

          return PotHistoryPoint(
            timestamp: DateTime.parse(item['timestamp'] as String),
            airTemp: MetricAggregate(
              avg: airTemp,
              min: airTemp,
              max: airTemp,
            ),
            airPressure: MetricAggregate(
              avg: airPressure,
              min: airPressure,
              max: airPressure,
            ),
            soilMoisture: MetricAggregate(
              avg: soilMoisture,
              min: soilMoisture,
              max: soilMoisture,
            ),
            illuminance: MetricAggregate(
              avg: illuminance,
              min: illuminance,
              max: illuminance,
            ),
          );
        })
        .toList();

    points.sort((a, b) => a.timestamp.compareTo(b.timestamp));
    return points;
  }

  Future<DateTime?> fetchLatestMeasureTimestamp({
    required String potId,
  }) async {
    final points = await fetchPotHistoryRecent(potId: potId, count: 1);
    if (points.isEmpty) return null;
    return points.last.timestamp;
  }

  Future<void> addPotConnection(
    String potId, {
    required String email,
    required String role,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');
    final response = await http.post(
      url,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ${user.token}',
      },
      body: json.encode({'email': email, 'role': role}),
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd dodawania dostępu: ${response.statusCode} ($details)"
            : "Błąd dodawania dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> updatePotConnection(
    String potId, {
    String? email,
    String? userId,
    required String role,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    if (email == null && userId == null) {
      throw Exception("Musisz podać email lub userId");
    }

    Uri url;
    if (userId != null && userId.isNotEmpty) {
      url = Uri.parse('$_baseUrl/pots/$potId/connections/$userId');
    } else {
      url = Uri.parse('$_baseUrl/pots/$potId/connections');
    }

    final request = http.Request('PATCH', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });

    final body = {'role': role};
    if (userId == null || userId.isEmpty) {
      body['email'] = email!;
    } else {
      body['user_id'] = userId;
    }
    request.body = json.encode(body);

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = await response.stream.bytesToString();
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd aktualizacji dostępu: ${response.statusCode} ($details)"
            : "Błąd aktualizacji dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> deletePotConnection(
    String potId, {
    String? email,
    String? userId,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    if (email == null && userId == null) {
      throw Exception("Musisz podać email lub userId");
    }

    Uri url;
    if (userId != null && userId.isNotEmpty) {
      url = Uri.parse('$_baseUrl/pots/$potId/connections/$userId');
    } else {
      url = Uri.parse('$_baseUrl/pots/$potId/connections');
    }

    final request = http.Request('DELETE', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });

    if (userId == null || userId.isEmpty) {
      request.body = json.encode({'email': email});
    }

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = await response.stream.bytesToString();
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd usuwania dostępu: ${response.statusCode} ($details)"
            : "Błąd usuwania dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> removeSelfConnection(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');
    final request = http.Request('DELETE', url);
    request.headers.addAll({
      'Authorization': 'Bearer ${user.token}',
    });

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = await response.stream.bytesToString();
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd rozłączania: ${response.statusCode} ($details)"
            : "Błąd rozłączania: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<void> fetchPotConnections(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      return;
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');
    final response = await http.get(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200) {
      return;
    }

    final List<dynamic> jsonData = jsonDecode(response.body);
    final connections = jsonData
        .whereType<Map>()
        .map((item) => PotConnection.fromJson(item.cast<String, dynamic>()))
        .toList();

    final index = _pots.indexWhere((pot) => pot.potId == potId);
    if (index == -1) return;
    final existing = _pots[index];
    _pots[index] = Pot(
      id: existing.id,
      potId: existing.potId,
      timeStamp: existing.timeStamp,
      data: existing.data,
      userId: existing.userId,
      role: existing.role,
      name: existing.name,
      config: existing.config,
      connections: connections,
      isActive: existing.isActive,
    );
    notifyListeners();
  }

  Future<bool> waterPot(String potId, {int duration = 5}) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/actions/water');
    final response = await http.post(
      url,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ${user.token}',
      },
      body: json.encode({'duration': duration}),
    );

    if (response.statusCode == 202) {
      return true;
    } else {
      final body = response.body;
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd podlewania: ${response.statusCode} ($details)"
            : "Błąd podlewania: ${response.statusCode}",
      );
    }
  }

  Future<void> deletePot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId');
    final response = await http.delete(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd usuwania doniczki: ${response.statusCode} ($details)"
            : "Błąd usuwania doniczki: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<void> disconnectPot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/pairing');
    final response = await http.delete(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200 && response.statusCode != 204) {
      throw Exception("Błąd rozłączania doniczki: ${response.statusCode}");
    }

    await fetchPots();
  }

  Future<void> hardResetPot(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/hard-reset');
    final response = await http.post(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd twardego resetu: ${response.statusCode} ($details)"
            : "Błąd twardego resetu: ${response.statusCode}",
      );
    }

    await fetchPots();
  }

  Future<void> addPotConnection(
    String potId, {
    required String email,
    required String role,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');
    final response = await http.post(
      url,
      headers: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ${user.token}',
      },
      body: json.encode({'email': email, 'role': role}),
    );

    if (response.statusCode < 200 || response.statusCode >= 300) {
      String details = '';
      try {
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = response.body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd dodawania dostępu: ${response.statusCode} ($details)"
            : "Błąd dodawania dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> updatePotConnection(
    String potId, {
    required String email,
    required String role,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');

    final request = http.Request('PATCH', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });
    request.body = json.encode({'email': email, 'role': role});

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = await response.stream.bytesToString();
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd aktualizacji dostępu: ${response.statusCode} ($details)"
            : "Błąd aktualizacji dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> deletePotConnection(
    String potId, {
    required String email,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');

    final request = http.Request('DELETE', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });
    request.body = json.encode({'email': email});

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      final body = await response.stream.bytesToString();
      String details = '';
      try {
        final decoded = jsonDecode(body);
        if (decoded is Map<String, dynamic>) {
          details = decoded['detail']?.toString() ?? '';
        } else if (decoded is String) {
          details = decoded;
        }
      } catch (_) {
        details = body;
      }
      throw Exception(
        details.isNotEmpty
            ? "Błąd usuwania dostępu: ${response.statusCode} ($details)"
            : "Błąd usuwania dostępu: ${response.statusCode}",
      );
    }
    await fetchPotConnections(potId);
  }

  Future<void> fetchPotConnections(String potId) async {
    final user = _authController.currentUser;
    if (user == null) {
      return;
    }

    final url = Uri.parse('$_baseUrl/pots/$potId/connections');
    final response = await http.get(
      url,
      headers: {'Authorization': 'Bearer ${user.token}'},
    );

    if (response.statusCode != 200) {
      return;
    }

    final List<dynamic> jsonData = jsonDecode(response.body);
    final connections = jsonData
        .whereType<Map>()
        .map((item) => PotConnection.fromJson(item.cast<String, dynamic>()))
        .toList();

    final index = _pots.indexWhere((pot) => pot.potId == potId);
    if (index == -1) return;
    final existing = _pots[index];
    _pots[index] = Pot(
      id: existing.id,
      potId: existing.potId,
      timeStamp: existing.timeStamp,
      data: existing.data,
      userId: existing.userId,
      role: existing.role,
      name: existing.name,
      config: existing.config,
      connections: connections,
    );
    notifyListeners();
  }
}
