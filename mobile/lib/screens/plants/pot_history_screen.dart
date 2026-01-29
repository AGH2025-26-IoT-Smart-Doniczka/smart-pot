import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';
import 'package:smart_pot_mobile_app/models/pot_history.dart';

class HistoryRangeOption {
  final String label;
  final int days;
  final String bucket;

  const HistoryRangeOption({
    required this.label,
    required this.days,
    required this.bucket,
  });
}

class PotHistoryScreen extends StatefulWidget {
  final Pot pot;

  const PotHistoryScreen({super.key, required this.pot});

  @override
  State<PotHistoryScreen> createState() => _PotHistoryScreenState();
}

class _PotHistoryScreenState extends State<PotHistoryScreen> {
  static const _ranges = [
    HistoryRangeOption(label: '7 dni', days: 7, bucket: '1h'),
    HistoryRangeOption(label: '30 dni', days: 30, bucket: '6h'),
    HistoryRangeOption(label: '3 miesiące', days: 90, bucket: '1d'),
  ];

  HistoryRangeOption _range = _ranges[0];
  bool _loading = false;
  String? _error;
  List<PotHistoryPoint> _points = const [];

  @override
  void initState() {
    super.initState();
    _loadHistory();
  }

  Future<void> _loadHistory() async {
    setState(() {
      _loading = true;
      _error = null;
    });
    try {
      final now = DateTime.now().toUtc();
      final from = now.subtract(Duration(days: _range.days));
      final points = await context.read<PotsController>().fetchPotHistory(
        potId: widget.pot.potId,
        from: from,
        to: now,
        bucket: _range.bucket,
      );
      if (!mounted) return;
      setState(() {
        _points = points;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _error = e.toString();
      });
    } finally {
      if (mounted) {
        setState(() {
          _loading = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Historia: ${widget.pot.name}')),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(12),
            child: Wrap(
              spacing: 8,
              children: _ranges
                  .map(
                    (option) => ChoiceChip(
                      label: Text(option.label),
                      selected: option == _range,
                      onSelected: (selected) {
                        if (!selected || option == _range) return;
                        setState(() {
                          _range = option;
                        });
                        _loadHistory();
                      },
                    ),
                  )
                  .toList(),
            ),
          ),
          Expanded(
            child: _loading
                ? const Center(child: CircularProgressIndicator())
                : _error != null
                ? _buildError(context, _error!)
                : _points.isEmpty
                ? const Center(child: Text('Brak danych historycznych'))
                : ListView(
                    padding: const EdgeInsets.symmetric(horizontal: 12),
                    children: [
                      _buildChartCard(
                        context,
                        title: 'Temperatura (°C)',
                        color: Colors.red,
                        metricSelector: (p) => p.airTemp,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Ciśnienie (hPa)',
                        color: Colors.blueGrey,
                        metricSelector: (p) => p.airPressure,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Wilgotność gleby (%)',
                        color: Colors.blue,
                        metricSelector: (p) => p.soilMoisture,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Oświetlenie (lx)',
                        color: Colors.amber,
                        metricSelector: (p) => p.illuminance,
                      ),
                      const SizedBox(height: 16),
                    ],
                  ),
          ),
        ],
      ),
    );
  }

  Widget _buildError(BuildContext context, String error) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(Icons.error_outline, size: 48, color: Colors.redAccent),
            const SizedBox(height: 12),
            Text(
              'Nie udało się pobrać historii',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 8),
            Text(
              error,
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodySmall,
            ),
            const SizedBox(height: 16),
            ElevatedButton(
              onPressed: _loadHistory,
              child: const Text('Spróbuj ponownie'),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildChartCard(
    BuildContext context, {
    required String title,
    required Color color,
    required MetricAggregate Function(PotHistoryPoint) metricSelector,
  }) {
    final avgSpots = _buildSpots(metricSelector, (m) => m.avg);
    final minSpots = _buildSpots(metricSelector, (m) => m.min);
    final maxSpots = _buildSpots(metricSelector, (m) => m.max);

    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            SizedBox(
              height: 220,
              child: LineChart(
                LineChartData(
                  minX: _minX(),
                  maxX: _maxX(),
                  gridData: FlGridData(show: true),
                  borderData: FlBorderData(show: false),
                  titlesData: _buildTitles(context),
                  lineBarsData: [
                    LineChartBarData(
                      spots: avgSpots,
                      color: color,
                      isCurved: true,
                      barWidth: 2.5,
                      dotData: const FlDotData(show: false),
                    ),
                    LineChartBarData(
                      spots: minSpots,
                      color: color.withValues(alpha: 0.6),
                      isCurved: false,
                      barWidth: 1.5,
                      dotData: const FlDotData(show: false),
                      dashArray: const [6, 4],
                    ),
                    LineChartBarData(
                      spots: maxSpots,
                      color: color.withValues(alpha: 0.6),
                      isCurved: false,
                      barWidth: 1.5,
                      dotData: const FlDotData(show: false),
                      dashArray: const [6, 4],
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 8),
            Row(
              children: [
                _legendDot(color),
                const SizedBox(width: 6),
                const Text('avg'),
                const SizedBox(width: 16),
                _legendDash(color.withValues(alpha: 0.6)),
                const SizedBox(width: 6),
                const Text('min/max'),
              ],
            ),
          ],
        ),
      ),
    );
  }

  List<FlSpot> _buildSpots(
    MetricAggregate Function(PotHistoryPoint) metricSelector,
    double Function(MetricAggregate) valueSelector,
  ) {
    return _points
        .map((point) {
          final metric = metricSelector(point);
          return FlSpot(
            point.timestamp.toLocal().millisecondsSinceEpoch.toDouble(),
            valueSelector(metric),
          );
        })
        .toList();
  }

  double _minX() {
    if (_points.isEmpty) return 0;
    return _points
        .first
        .timestamp
        .toLocal()
        .millisecondsSinceEpoch
        .toDouble();
  }

  double _maxX() {
    if (_points.isEmpty) return 0;
    return _points
        .last
        .timestamp
        .toLocal()
        .millisecondsSinceEpoch
        .toDouble();
  }

  FlTitlesData _buildTitles(BuildContext context) {
    final formatter = _range.days <= 7
        ? DateFormat('dd.MM HH:mm')
        : DateFormat('dd.MM');

    return FlTitlesData(
      topTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
      rightTitles: const AxisTitles(sideTitles: SideTitles(showTitles: false)),
      leftTitles: AxisTitles(
        sideTitles: SideTitles(
          showTitles: true,
          reservedSize: 44,
          interval: null,
        ),
      ),
      bottomTitles: AxisTitles(
        sideTitles: SideTitles(
          showTitles: true,
          reservedSize: 32,
          interval: _labelInterval(),
          getTitlesWidget: (value, meta) {
            final date =
                DateTime.fromMillisecondsSinceEpoch(value.toInt()).toLocal();
            return SideTitleWidget(
              axisSide: meta.axisSide,
              child: Text(
                formatter.format(date),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            );
          },
        ),
      ),
    );
  }

  double _labelInterval() {
    if (_points.length <= 1) return 1;
    final start = _minX();
    final end = _maxX();
    final span = end - start;
    return span / 4;
  }

  Widget _legendDot(Color color) {
    return Container(
      width: 10,
      height: 10,
      decoration: BoxDecoration(shape: BoxShape.circle, color: color),
    );
  }

  Widget _legendDash(Color color) {
    return SizedBox(
      width: 18,
      height: 6,
      child: CustomPaint(
        painter: _DashPainter(color),
      ),
    );
  }
}

class _DashPainter extends CustomPainter {
  final Color color;

  _DashPainter(this.color);

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = color
      ..strokeWidth = 2;
    const dashWidth = 4.0;
    const dashSpace = 3.0;
    double startX = 0;
    while (startX < size.width) {
      canvas.drawLine(
        Offset(startX, size.height / 2),
        Offset(startX + dashWidth, size.height / 2),
        paint,
      );
      startX += dashWidth + dashSpace;
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
