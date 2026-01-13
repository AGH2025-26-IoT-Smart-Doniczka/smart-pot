
import 'dart:convert';

import 'package:flutter/cupertino.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:smart_pot_mobile_app/models/user_model.dart';
import 'package:http/http.dart' as http
class AuthController extends ChangeNotifier {

  // Mock backend - json-blob URL
  static const String _baseUrl = 'https://jsonblob.com/api/jsonBlob/019ba2d5-7e45-79be-8a2b-cb153ff7f35e';
  final _storage = const FlutterSecureStorage(); //bezpieczny magazyn do trzymania tokena jwt

  User? _currentUser;
  bool _isLoading = false;
  String? _errorMessage;

  User? get currentUser => _currentUser;
  bool get isLoading => _isLoading;
  String? get errorMessage => _errorMessage;
  bool get isAuthenticated => _currentUser != null;


  Future<void> tryAutoLogin() async {
    final token = await _storage.read(key: 'jwt');
    final userDataString = await _storage.read(key: 'user_data');

    if(token != null && userDataString != null){
      try{
        final userData = json.decode(userDataString);
        _currentUser = User.fromLocal(userData, token);
        notifyListeners();
      }catch(e){
        logout();
      }
    }
  }

  Future<bool> login(String email, String password) async {
    _setIsLoading(true);
    // TODO: Potem zmień na prawdziwy endpoint z backendu do loginu
    final url = Uri.parse(_baseUrl);

    try{
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'email': email,
          'password': password
        }),
      );

      final data = json.decode(response.body);
      if(response.statusCode == 200 || response.statusCode == 201){
        await _saveUserSession(data);
        _errorMessage = null;
        return true;
      }else if (response.statusCode == 403){
        _errorMessage = data['error'] ?? 'Błędny login lub hasło';
        return false;
      }else{
        _errorMessage = "Błąd logowania: ${response.statusCode}";
        return false;
      }
    }catch(e){
      _errorMessage = "Błąd połączenia: $e";
      return false;
    }finally{
      _setIsLoading(false);

    }
  }

  Future<bool> register(String email, String password, String username) async{
    _setIsLoading(true);

    // TODO: Potem zmień na prawdziwy endpoint z backendu
    final url = Uri.parse(_baseUrl);

    try{
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: json.encode({
          'email': email,
          'password': password,
          'username': username,
        }),
      );

      final data = json.decode(response.body);

      if(response.statusCode == 201 ){
        await _saveUserSession(data);
        _errorMessage = null;
        return true;
      }else if(response.statusCode == 403){
      //email już jest używany
        _errorMessage = data['error'] ?? 'Rejestracja nieudana';
        return false;
      }else{
        _errorMessage = 'Błąd serwera: ${response.statusCode}';
        return false;
      }
    }catch(e){
      _errorMessage = "Błąd połączenia: $e";
      return false;
    }finally{
      _setIsLoading(false);
    }
  }

  Future<void> logout() async{
    await _storage.deleteAll();
    _currentUser = null;
    notifyListeners();
  }

  Future<void> _saveUserSession(Map<String, dynamic> data) async {
    _currentUser = User.fromApiResponse(data);

    if(_currentUser!.token != null){
      await _storage.write(key: 'jwt', value: _currentUser!.token);
    }

    // Zapisuje podstawowe informacje o użytkowniku do storage
    await _storage.write(key: 'user_data', value: json.encode(data['user']));
    notifyListeners();
  }

  void _setIsLoading(bool value) {
    _isLoading = value;
    notifyListeners();
  }
}