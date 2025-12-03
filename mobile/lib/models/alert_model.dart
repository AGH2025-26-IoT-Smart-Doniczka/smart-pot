enum AlertType {
  info,
  warning,
  error
}

class Alert {
  final String description;
  final DateTime dateTime;
  final AlertType alertType;

  Alert({
    required this.dateTime,
    required this.description,
    required this.alertType
  });
}
