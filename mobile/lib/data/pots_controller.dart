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
      throw Exception("Błąd dodawania dostępu: ${response.statusCode}");
    }
  }

  Future<void> updatePotConnection(
    String potId, {
    required String connectionId,
    required String email,
    required String role,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = connectionId.isNotEmpty
        ? Uri.parse('$_baseUrl/pots/$potId/connections/$connectionId')
        : Uri.parse('$_baseUrl/pots/$potId/connections');

    final request = http.Request('PATCH', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });
    request.body = json.encode({'email': email, 'role': role});

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception("Błąd aktualizacji dostępu: ${response.statusCode}");
    }
  }

  Future<void> deletePotConnection(
    String potId, {
    required String connectionId,
    required String email,
  }) async {
    final user = _authController.currentUser;
    if (user == null) {
      throw Exception("Użytkownik nie jest zalogowany");
    }

    final url = connectionId.isNotEmpty
        ? Uri.parse('$_baseUrl/pots/$potId/connections/$connectionId')
        : Uri.parse('$_baseUrl/pots/$potId/connections');

    final request = http.Request('DELETE', url);
    request.headers.addAll({
      'Content-Type': 'application/json',
      'Authorization': 'Bearer ${user.token}',
    });
    if (connectionId.isEmpty) {
      request.body = json.encode({'email': email});
    }

    final response = await request.send();
    if (response.statusCode < 200 || response.statusCode >= 300) {
      throw Exception("Błąd usuwania dostępu: ${response.statusCode}");
    }
  }
}
