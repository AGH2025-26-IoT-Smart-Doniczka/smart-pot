import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/pots_controller.dart';
import 'package:smart_pot_mobile_app/models/pot_data.dart';

class PotConfigScreen extends StatefulWidget {
  final Pot pot;

  const PotConfigScreen({super.key, required this.pot});

  @override
  State<PotConfigScreen> createState() => _PotConfigScreenState();
}

class _PotConfigScreenState extends State<PotConfigScreen> {
  final _formKey = GlobalKey<FormState>();
  late final TextEditingController _nameController;
  final TextEditingController _measurementPeriodController =
      TextEditingController();
  final TextEditingController _sendPeriodController = TextEditingController();
  final TextEditingController _wateringPeriodController = TextEditingController();
  bool _autoWateringEnabled = false;
  bool _isSaving = false;

  @override
  void initState() {
    super.initState();
    _nameController = TextEditingController(text: widget.pot.name);
    _measurementPeriodController.text =
        widget.pot.config.measureIntervalSec.toString();
    _sendPeriodController.text = widget.pot.config.sendIntervalSec.toString();
    if (widget.pot.config.wateringIntervalSec != null) {
      _autoWateringEnabled = true;
      _wateringPeriodController.text =
          widget.pot.config.wateringIntervalSec.toString();
    }
  }

  @override
  void dispose() {
    _nameController.dispose();
    _measurementPeriodController.dispose();
    _sendPeriodController.dispose();
    _wateringPeriodController.dispose();
    super.dispose();
  }

  Future<void> _saveConfiguration() async {
    if (!_formKey.currentState!.validate()) {
      return;
    }

    final measureInterval =
        int.tryParse(_measurementPeriodController.text.trim()) ?? 0;
    final sendInterval =
        int.tryParse(_sendPeriodController.text.trim()) ?? 0;
    final wateringInterval = _autoWateringEnabled
        ? int.tryParse(_wateringPeriodController.text.trim())
        : null;

    final payload = widget.pot.config.toApiJson(
      overrideName: _nameController.text.trim(),
    );
    payload['measure_interval_sec'] = measureInterval;
    payload['send_interval_sec'] = sendInterval;
    payload['watering_interval_sec'] = wateringInterval;

    setState(() {
      _isSaving = true;
    });

    try {
      await context
          .read<PotsController>()
          .updatePotConfig(widget.pot.potId, payload);
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Konfiguracja zapisana.')),
      );
      Navigator.of(context).pop();
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Nie udało się zapisać konfiguracji: $e')),
      );
    } finally {
      if (!mounted) return;
      setState(() {
        _isSaving = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Konfiguracja doniczki'),
      ),
      body: SafeArea(
        child: Form(
          key: _formKey,
          child: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              Text(
                'Ustawienia ${widget.pot.name}',
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 24),
              TextFormField(
                controller: _nameController,
                decoration: const InputDecoration(
                  labelText: 'Nazwa doniczki',
                  border: OutlineInputBorder(),
                ),
                textInputAction: TextInputAction.next,
                validator: (value) {
                  if (value == null || value.trim().isEmpty) {
                    return 'Podaj nazwę doniczki.';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 16),
              TextFormField(
                controller: _measurementPeriodController,
                decoration: const InputDecoration(
                  labelText: 'Okres pomiaru (sekundy)',
                  hintText: 'Np. 60',
                  border: OutlineInputBorder(),
                ),
                keyboardType: TextInputType.number,
                textInputAction: TextInputAction.next,
                validator: (value) {
                  if (value == null || value.trim().isEmpty) {
                    return 'Podaj okres pomiaru.';
                  }
                  final parsed = int.tryParse(value);
                  if (parsed == null || parsed <= 0) {
                    return 'Wpisz dodatnią liczbę sekund.';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 16),
              TextFormField(
                controller: _sendPeriodController,
                decoration: const InputDecoration(
                  labelText: 'Okres wysyłania (sekundy)',
                  hintText: 'Np. 300',
                  border: OutlineInputBorder(),
                ),
                keyboardType: TextInputType.number,
                textInputAction: TextInputAction.done,
                validator: (value) {
                  if (value == null || value.trim().isEmpty) {
                    return 'Podaj okres wysyłania.';
                  }
                  final parsed = int.tryParse(value);
                  if (parsed == null || parsed <= 0) {
                    return 'Wpisz dodatnią liczbę sekund.';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 16),
              SwitchListTile(
                contentPadding: EdgeInsets.zero,
                title: const Text('Automatyczne podlewanie'),
                subtitle: const Text('Włącz automatyczne uruchamianie podlewania.'),
                value: _autoWateringEnabled,
                onChanged: (value) {
                  setState(() {
                    _autoWateringEnabled = value;
                  });
                },
              ),
              const SizedBox(height: 8),
              TextFormField(
                controller: _wateringPeriodController,
                decoration: const InputDecoration(
                  labelText: 'Okres podlewania (sekundy)',
                  hintText: 'Np. 600',
                  border: OutlineInputBorder(),
                ),
                enabled: _autoWateringEnabled,
                keyboardType: TextInputType.number,
                textInputAction: TextInputAction.done,
                validator: (value) {
                  if (!_autoWateringEnabled) {
                    return null;
                  }
                  if (value == null || value.trim().isEmpty) {
                    return 'Podaj okres podlewania.';
                  }
                  final parsed = int.tryParse(value);
                  if (parsed == null || parsed <= 0) {
                    return 'Wpisz dodatnią liczbę sekund.';
                  }
                  return null;
                },
              ),
              const SizedBox(height: 24),
              FilledButton.icon(
                onPressed: _isSaving ? null : _saveConfiguration,
                icon: const Icon(Icons.save),
                label: Text(_isSaving ? 'Zapisywanie...' : 'Zapisz konfigurację'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
