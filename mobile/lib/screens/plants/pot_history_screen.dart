import 'dart:async';

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
  final bool isRecent;
  final int? count;

  const HistoryRangeOption({
    required this.label,
    required this.days,
    required this.bucket,
    this.isRecent = false,
    this.count,
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
    HistoryRangeOption(
      label: 'Ostatnie pomiary',
      days: 0,
      bucket: '1h',
      isRecent: true,
      count: 20,
    ),
    HistoryRangeOption(label: '7 dni', days: 7, bucket: '1h'),
    HistoryRangeOption(label: '30 dni', days: 30, bucket: '6h'),
    HistoryRangeOption(label: '3 miesiące', days: 90, bucket: '1d'),
  ];

  HistoryRangeOption _range = _ranges[0];
  bool _loading = false;
  bool _refreshing = false;
  String? _error;
  List<PotHistoryPoint> _points = const [];
  Timer? _refreshTimer;
  int? _refreshIntervalSec;
  late final ScrollController _scrollController;

  @override
  void initState() {
    super.initState();
    _scrollController = ScrollController();
    _loadHistory();
  }

  @override
  void dispose() {
    _refreshTimer?.cancel();
    _scrollController.dispose();
    super.dispose();
  }

  void _ensureAutoRefresh(int seconds) {
    if (!_range.isRecent || seconds <= 0) {
      _refreshTimer?.cancel();
      _refreshTimer = null;
      _refreshIntervalSec = null;
      return;
    }
    if (_refreshIntervalSec == seconds && _refreshTimer != null) {
      return;
    }
    _refreshTimer?.cancel();
    _refreshIntervalSec = seconds;
    _refreshTimer = Timer.periodic(
      Duration(seconds: seconds),
      (_) => _loadHistory(background: true),
    );
  }

  Future<void> _loadHistory({bool background = false}) async {
    if (_loading || _refreshing) return;
    setState(() {
      if (background && _points.isNotEmpty) {
        _refreshing = true;
      } else {
        _loading = true;
      }
      _error = null;
    });
    try {
      final controller = context.read<PotsController>();
      List<PotHistoryPoint> points;
      if (_range.isRecent) {
        points = await controller.fetchPotHistoryRecent(
          potId: widget.pot.potId,
          count: _range.count ?? 20,
        );
      } else {
        final now = DateTime.now().toUtc();
        final from = now.subtract(Duration(days: _range.days));
        points = await controller.fetchPotHistory(
          potId: widget.pot.potId,
          from: from,
          to: now,
          bucket: _range.bucket,
        );
        if (points.isEmpty) {
          final latestTs = await controller.fetchLatestMeasureTimestamp(
            potId: widget.pot.potId,
          );
          if (latestTs != null) {
            final to = latestTs.toUtc();
            final fromLatest = to.subtract(Duration(days: _range.days));
            points = await controller.fetchPotHistory(
              potId: widget.pot.potId,
              from: fromLatest,
              to: to,
              bucket: _range.bucket,
            );
          }
        }
      }
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
          _refreshing = false;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final currentPot = context.select<PotsController, Pot?>(
      (ctrl) => ctrl.pots.firstWhere(
        (p) => p.potId == widget.pot.potId,
        orElse: () => widget.pot,
      ),
    );
    final viewPot = currentPot ?? widget.pot;
    _ensureAutoRefresh(viewPot.config.sendIntervalSec);

    return Scaffold(
      appBar: AppBar(
        title: Text('Historia: ${widget.pot.name}'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: _loadHistory,
          ),
        ],
      ),
      body: Column(
        children: [
          if (_refreshing) const LinearProgressIndicator(minHeight: 2),
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
            child: _loading && _points.isEmpty
                ? const Center(child: CircularProgressIndicator())
                : _error != null && _points.isEmpty
                ? _buildError(context, _error!)
                : _points.isEmpty
                ? const Center(child: Text('Brak danych historycznych'))
                : ListView(
                    key: PageStorageKey('pot_history_${widget.pot.potId}'),
                    controller: _scrollController,
                    padding: const EdgeInsets.symmetric(horizontal: 12),
                    children: [
                      _buildChartCard(
                        context,
                        title: 'Temperatura (°C)',
                        color: Colors.red,
                        metricSelector: (p) => p.airTemp,
                        showMinMax: !_range.isRecent,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Ciśnienie (hPa)',
                        color: Colors.blueGrey,
                        metricSelector: (p) => p.airPressure,
                        showMinMax: !_range.isRecent,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Wilgotność gleby (%)',
                        color: Colors.blue,
                        metricSelector: (p) => p.soilMoisture,
                        showMinMax: !_range.isRecent,
                      ),
                      _buildChartCard(
                        context,
                        title: 'Oświetlenie (lx)',
                        color: Colors.amber,
                        metricSelector: (p) => p.illuminance,
                        showMinMax: !_range.isRecent,
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
    required bool showMinMax,
  }) {
    final avgSpots = _buildSpots(metricSelector, (m) => m.avg);
    final minSpots =
        showMinMax ? _buildSpots(metricSelector, (m) => m.min) : const <FlSpot>[];
    final maxSpots =
        showMinMax ? _buildSpots(metricSelector, (m) => m.max) : const <FlSpot>[];

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
                    if (showMinMax)
                      LineChartBarData(
                        spots: minSpots,
                        color: color.withValues(alpha: 0.6),
                        isCurved: false,
                        barWidth: 1.5,
                        dotData: const FlDotData(show: false),
                        dashArray: const [6, 4],
                      ),
                    if (showMinMax)
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
            if (showMinMax)
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
              )
            else
              Row(
                children: [
                  _legendDot(color),
                  const SizedBox(width: 6),
                  const Text('wartość'),
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
    final formatter = (_range.isRecent || _range.days <= 7)
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
