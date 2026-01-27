import 'dart:io';

import 'package:flutter/foundation.dart';

class AppConfig {
  static const String _envBaseUrl =
      'https://unevenly-undecried-rafael.ngrok-free.dev';

  static String get baseUrl {
    if (_envBaseUrl.isNotEmpty) return _envBaseUrl;

    // Web runs in the browser on the host machine.
    if (kIsWeb) return 'http://localhost:8000';

    // Android emulator uses 10.0.2.2 to reach host machine.
    if (Platform.isAndroid) return 'http://10.0.2.2:8000';

    // iOS simulator and desktop can use localhost.
    return 'http://127.0.0.1:8000';
  }
}
