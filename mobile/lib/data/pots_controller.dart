import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:http/http.dart' as http;

class PotsController extends ChangeNotifier {
  final AuthController _authController;

  //TODO: to potem należy schować gdzieś do zmiennych środowiskowych
  static const String _baseUrl = 'https://jsonblob.com/api/jsonBlob/019ba2d5-7e45-79be-8a2b-cb153ff7f35e';

  List<Pot> _pots =[];
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
      final response = await http.get(Uri.parse(_baseUrl));
      if (response.statusCode != 200) {
        _error = "Błąd pobierania danych doniczek: ${response.statusCode}";
        return;
      }

      // Dekodujemy cały JSON
      final Map<String, dynamic> jsonData = jsonDecode(response.body);

      // Wyciągamy listę doniczek
      final List<dynamic> potsJson = jsonData['pots'] ?? [];

      // Filtrujemy doniczki należące do zalogowanego użytkownika
      final userPotsJson = potsJson
          .where((pot) => pot['userPublicKey'] == user.id)
          .toList();

      // Mapujemy JSON na obiekty Pot
      _pots = userPotsJson.map((json) => Pot.fromJson(json)).toList();
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
      throw new Exception("Użytkownik nie jest zalogowany");
    }

    final url = Uri.parse('$_baseUrl/pots/${potId}/pairing');

    try{
      final response = await http.post(
        url,
        headers: {
          'Content-Type': 'application/json',
          'Authorization': 'Bearer ${user.token}'
        },
        body: json.encode({
          "user_id": user.id,
        }),
      );

      if(response.statusCode == 201 || response.statusCode == 200){
        return json.decode(response.body);
      }else{
        final errorData = json.decode(response.body);
        throw Exception(errorData['detail'] ?? 'Błąd parowania: ${response.statusCode}');
      }
    }catch(e){
      throw Exception("Błąd połączenia z serwerem: $e");
    }
  }
}
