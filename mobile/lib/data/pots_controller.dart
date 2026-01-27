import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:http/http.dart' as http;

import '../config/app_config.dart';

class PotsController extends ChangeNotifier {
  final AuthController _authController;

  static final String _baseUrl = AppConfig.baseUrl;

  List<Pot> _pots = [];
  bool _isLoading = false;
  String? _error;

  List<Pot> get pots => _pots;
  bool get isLoading => _isLoading;
  String? get error => _error;

  PotsController(this._authController);

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

  Future<void> updatePotConfig(
    String potId,
    Map<String, dynamic> payload,
  ) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
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
}
