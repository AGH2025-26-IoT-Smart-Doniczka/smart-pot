enum AlertType {
  info,
  warning,
  error
}

class Alert {
  final String title;
  final String description;
  final DateTime dateTime;
  final AlertType alertType;

  Alert({
    required this.title,
    required this.dateTime,
    required this.description,
    required this.alertType
  });
}
