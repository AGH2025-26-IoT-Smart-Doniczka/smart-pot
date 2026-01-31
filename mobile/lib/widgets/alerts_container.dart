import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/alert_model.dart';

class AlertsContainer extends StatefulWidget {
  const AlertsContainer({super.key});

  @override
  State<AlertsContainer> createState() => _AlertsContainerState();
}

class _AlertsContainerState extends State<AlertsContainer> {
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      context.read<PotsController>().fetchAlerts();
    });
  }

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

  String _formatData(DateTime date) {
    return DateFormat('dd.MM.yyyy HH:mm').format(date);
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<PotsController>(
      builder: (context, ctrl, child) {
        if (ctrl.isAlertsLoading) {
          return const Center(child: CircularProgressIndicator());
        }
        if (ctrl.alertsError != null) {
          return Center(child: Text(ctrl.alertsError!));
        }
        final activePotIds = ctrl.pots
            .where((pot) => pot.isActive)
            .map((pot) => pot.potId)
            .toSet();
        final filteredAlerts = ctrl.alerts
            .where((alert) => alert.potId.isNotEmpty && activePotIds.contains(alert.potId))
            .toList();
        if (filteredAlerts.isEmpty) {
          return const Center(child: Text("Brak alertów"));
        }
        return ListView.builder(
          padding: EdgeInsets.zero,
          itemCount: filteredAlerts.length,
          shrinkWrap: true,
          physics: const NeverScrollableScrollPhysics(),
          itemBuilder: (context, index) {
            final alert = filteredAlerts[index];
            return Card(
              margin: const EdgeInsets.symmetric(vertical: 6),
              child: ListTile(
                leading: Text(
                  _getEmojiForAlert(alert.alertType),
                  style: const TextStyle(fontSize: 28),
                ),
                title: Text(
                  alert.title,
                  style: const TextStyle(fontWeight: FontWeight.bold),
                ),
                subtitle: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(alert.description),
                    Text(_formatData(alert.dateTime)),
                  ],
                ),
              ),
            );
          },
        );
      },
    );
  }
}
