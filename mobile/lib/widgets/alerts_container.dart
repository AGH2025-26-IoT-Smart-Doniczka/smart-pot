import 'package:flutter/material.dart';
import 'package:smart_pot_mobile_app/models/alert_model.dart';
import 'package:intl/intl.dart';
class AlertsContainer extends StatelessWidget {
  AlertsContainer({super.key});

  final List<Alert> lastAlerts = [
    Alert(
      description: "Storczyk został podlany",
      alertType: AlertType.info,
      dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
    Alert(
        description: "Bratki wymagają podlania",
        alertType: AlertType.error,
        dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
    Alert(
        description: "Przestaw Hiacynta w słoneczne miejsce",
        alertType: AlertType.warning,
        dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
    Alert(
        description: "Storczyk wymaga podlania",
        alertType: AlertType.error,
        dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
    Alert(
        description: "Przestaw Bratka w cieplejsze miejsce",
        alertType: AlertType.warning,
        dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
    Alert(
        description: "Bratek został podlany",
        alertType: AlertType.info,
        dateTime: DateTime.now().subtract(Duration(days: 1))
    ),
  ];

  // W przyszłości można zmienić na IKONY zamiast emotek
  String _getEmojiForAlert(AlertType type) {
    switch (type) {
      case AlertType.error:
        return "❗";
      case AlertType.warning:
        return "⚠️";
      case AlertType.info:
        return "✅";
    }
  }

  String _formatData(DateTime date){
    return DateFormat('dd.MM.yyyy HH:mm').format(date);
  }

  @override
  Widget build(BuildContext context) {
    return ListView.builder(
      padding: EdgeInsets.zero,
      itemCount: lastAlerts.length,
      shrinkWrap: true,
      physics: NeverScrollableScrollPhysics(),
      itemBuilder: (context, index){
        final alert = lastAlerts[index];
        return Card(
          margin: EdgeInsets.symmetric(vertical: 6),
          child: ListTile(
            leading: Text(
              _getEmojiForAlert(alert.alertType),
              style: TextStyle(fontSize: 28)
            ),
            title: Text(alert.description, style: TextStyle(fontWeight: FontWeight.bold)),
            subtitle: Text(_formatData(alert.dateTime)),
          ),
        );
      },
    );
  }
}
