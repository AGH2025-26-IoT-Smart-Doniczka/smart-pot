
class User {
  final String id;
  final String email;
  final String username;
  final String? token; //przyda się później

  User({
    required this.id,
    required this.email,
    required this.username,
    this.token,
  });

  factory User.fromApiResponse(Map<String, dynamic> json) {
    final userData = json['user'] ?? {};
    return User(
      id: userData['id']?.toString() ?? '',
      email: userData['email'] ?? '',
      username: userData['username'] ?? '',
      token: json['access_token']
    );
  }

  factory User.fromLocal(Map<String, dynamic> json, String token) {
    return User(
      id: json['id']?.toString() ?? '',
      email: json['email'] ?? '',
      username: json['username'] ?? '',
      token: token
    );
  }
}