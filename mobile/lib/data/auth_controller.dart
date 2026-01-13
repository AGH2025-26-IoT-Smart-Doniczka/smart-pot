
import 'dart:convert';

import 'package:flutter/cupertino.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:smart_pot_mobile_app/models/user_model.dart';
import 'package:http/http.dart' as http;
class AuthController extends ChangeNotifier {

  static const String _baseUrl = 'http://192.168.0.21:8000';
  final _storage = const FlutterSecureStorage(); //bezpieczny magazyn do trzymania tokena jwt

  User? _currentUser;
  bool _isLoading = false;
  String? _errorMessage;

  User? get currentUser => _currentUser;
  bool get isLoading => _isLoading;
  String? get errorMessage => _errorMessage;
  bool get isAuthenticated => _currentUser != null;


  Future<void> tryAutoLogin() async {
    final token = await _storage.read(key: 'jwt_token');
    final userDataString = await _storage.read(key: 'user_data');

    if(token != null && userDataString != null){
      try{
        final userData = json.decode(userDataString);
        _currentUser = User.fromLocal(userData, token);
        notifyListeners();
      }catch(e){
        await logout();
      }
    }
  }

  Future<bool> register(String email, String password, String username) async {
    return _authenticate(endpoint: '/user',
        body: {
          'email': email,
          'password': password,
          'username': username
        });
  }

  Future<bool> login(String email, String password) async {
    return _authenticate(endpoint: '/auth/login',
        body: {
          'email': email,
          'password': password
        });
  }

  Future<bool> _authenticate({required String endpoint, required Map<String, String> body}) async{
    _setIsLoading(true);
    _errorMessage=null;
    try{
      final url = Uri.parse('$_baseUrl$endpoint');
      final response = await http.post(
        url,
        headers: {'Content-Type': 'application/json'},
        body: json.encode(body),
      );
      final data = json.decode(response.body);

      if(response.statusCode >= 200 && response.statusCode < 300){
        await _saveUserSession(data);
        return true;
      }else{
        _errorMessage = data['detail'] ?? 'Błąd autoryzacji ${response.statusCode}';
        return false;
      }
    }catch(e){
        _errorMessage = 'Błąd połączenia: ${e}. Sprawdź czy serwer działa i adres jest poprawny';
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
      await _storage.write(key: 'jwt_token', value: _currentUser!.token);
    }

    final userData = data['user'] ?? {};
    await _storage.write(key: 'user_data', value: json.encode(userData));
    notifyListeners();
  }

  void _setIsLoading(bool value) {
    _isLoading = value;
    notifyListeners();
  }
}