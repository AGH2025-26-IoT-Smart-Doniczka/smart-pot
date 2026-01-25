import 'package:flutter/cupertino.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:smart_pot_mobile_app/data/auth_controller.dart';
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
  final _permissionsFormKey = GlobalKey<FormState>();
  late final TextEditingController _nameController;
  final TextEditingController _measurementPeriodController =
      TextEditingController();
  final TextEditingController _sendPeriodController = TextEditingController();
  final TextEditingController _wateringPeriodController = TextEditingController();
  final TextEditingController _measureHoursController = TextEditingController();
  final TextEditingController _measureMinutesController =
      TextEditingController();
  final TextEditingController _measureSecondsController =
      TextEditingController();
  final TextEditingController _sendHoursController = TextEditingController();
  final TextEditingController _sendMinutesController = TextEditingController();
  final TextEditingController _sendSecondsController = TextEditingController();
  final TextEditingController _wateringHoursController =
      TextEditingController();
  final TextEditingController _wateringMinutesController =
      TextEditingController();
  final TextEditingController _wateringSecondsController =
      TextEditingController();
  final TextEditingController _minTempController = TextEditingController();
  final TextEditingController _maxTempController = TextEditingController();
  final TextEditingController _minMoistureController = TextEditingController();
  final TextEditingController _maxMoistureController = TextEditingController();
  final TextEditingController _permissionEmailController =
      TextEditingController();
  int _measurementIntervalSec = 0;
  int _sendIntervalSec = 0;
  int _wateringIntervalSec = 0;
  bool _autoWateringEnabled = false;
  bool _isSaving = false;
  bool _isDisconnecting = false;
  bool _isPermissionsBusy = false;
  PotRole _newPermissionRole = PotRole.viewer;
  String _illuminance = 'medium';

  @override
  void initState() {
    super.initState();
    _nameController = TextEditingController(text: widget.pot.name);
    _measurementIntervalSec = widget.pot.config.measureIntervalSec;
    _sendIntervalSec = widget.pot.config.sendIntervalSec;
    _measurementPeriodController.text =
        _formatDurationForPicker(_measurementIntervalSec);
    _sendPeriodController.text = _formatDurationForPicker(_sendIntervalSec);
    _setWebFieldsFromSeconds(
      seconds: _measurementIntervalSec,
      hours: _measureHoursController,
      minutes: _measureMinutesController,
      secs: _measureSecondsController,
    );
    _setWebFieldsFromSeconds(
      seconds: _sendIntervalSec,
      hours: _sendHoursController,
      minutes: _sendMinutesController,
      secs: _sendSecondsController,
    );
    if (widget.pot.config.wateringIntervalSec != null) {
      _autoWateringEnabled = true;
      _wateringIntervalSec = widget.pot.config.wateringIntervalSec ?? 0;
      _wateringPeriodController.text =
          _formatDurationForPicker(_wateringIntervalSec);
      _setWebFieldsFromSeconds(
        seconds: _wateringIntervalSec,
        hours: _wateringHoursController,
        minutes: _wateringMinutesController,
        secs: _wateringSecondsController,
      );
    }
    _minTempController.text = widget.pot.config.minTemp.toString();
    _maxTempController.text = widget.pot.config.maxTemp.toString();
    _minMoistureController.text = widget.pot.config.minMoisture.toString();
    _maxMoistureController.text = widget.pot.config.maxMoisture.toString();
    _illuminance = widget.pot.config.illuminance;

    WidgetsBinding.instance.addPostFrameCallback((_) {
      _redirectIfViewer();
    });
  }

  @override
  void dispose() {
    _nameController.dispose();
    _measurementPeriodController.dispose();
    _sendPeriodController.dispose();
    _wateringPeriodController.dispose();
    _measureHoursController.dispose();
    _measureMinutesController.dispose();
    _measureSecondsController.dispose();
    _sendHoursController.dispose();
    _sendMinutesController.dispose();
    _sendSecondsController.dispose();
    _wateringHoursController.dispose();
    _wateringMinutesController.dispose();
    _wateringSecondsController.dispose();
    _minTempController.dispose();
    _maxTempController.dispose();
    _minMoistureController.dispose();
    _maxMoistureController.dispose();
    _permissionEmailController.dispose();
    super.dispose();
  }

  Future<void> _saveConfiguration() async {
    if (!_formKey.currentState!.validate()) {
      return;
    }

    final minTemp = double.parse(_minTempController.text.trim());
    final maxTemp = double.parse(_maxTempController.text.trim());
    final minMoisture = int.parse(_minMoistureController.text.trim());
    final maxMoisture = int.parse(_maxMoistureController.text.trim());
    final measureInterval = _measurementIntervalSec;
    final sendInterval = _sendIntervalSec;
    final wateringInterval = _autoWateringEnabled
        ? _wateringIntervalSec
        : null;

    final trimmedName = _nameController.text.trim();
    final payload = <String, dynamic>{
      'pot_name': trimmedName.isEmpty ? null : trimmedName,
      'measure_interval_sec': measureInterval,
      'send_interval_sec': sendInterval,
      'watering_interval_sec': wateringInterval,
      'min_temp': minTemp,
      'max_temp': maxTemp,
      'min_moisture': minMoisture,
      'max_moisture': maxMoisture,
      'illuminance': _illuminance,
    };

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

  Future<void> _disconnectPot() async {
    final shouldDisconnect = await _confirmDisconnect();
    if (!shouldDisconnect) return;
    if (!mounted) return;

    setState(() {
      _isDisconnecting = true;
    });

    try {
      await context.read<PotsController>().disconnectPot(widget.pot.potId);
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Doniczka została rozłączona.')),
      );
      Navigator.of(context).popUntil((route) => route.isFirst);
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Nie udało się rozłączyć: $e')),
      );
    } finally {
      if (!mounted) return;
      setState(() {
        _isDisconnecting = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final currentUserId = context.read<AuthController>().currentUser?.id ?? '';
    final currentPot = context.select<PotsController, Pot?>(
      (ctrl) => ctrl.pots.firstWhere(
        (p) => p.potId == widget.pot.potId,
        orElse: () => widget.pot,
      ),
    );
    final viewPot = currentPot ?? widget.pot;
    final role = _resolveRole(viewPot, currentUserId);

    if (role == PotRole.viewer || role == PotRole.unknown) {
      return const Scaffold(
        body: SafeArea(
          child: Center(child: CircularProgressIndicator()),
        ),
      );
    }

    final isOwner = role == PotRole.owner;

    final scaffold = Scaffold(
      appBar: AppBar(
        title: const Text('Konfiguracja doniczki'),
        bottom: isOwner
            ? const TabBar(
                tabs: [
                  Tab(text: 'Konfiguracja'),
                  Tab(text: 'Uprawnienia'),
                ],
              )
            : null,
      ),
      body: isOwner
          ? TabBarView(
              children: [
                _buildConfigurationForm(viewPot),
                _buildPermissionsTab(viewPot),
              ],
            )
          : SafeArea(child: _buildConfigurationForm(viewPot)),
      bottomNavigationBar: SafeArea(
        top: false,
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: FilledButton.icon(
            onPressed: _isDisconnecting ? null : _disconnectPot,
            style: FilledButton.styleFrom(
              backgroundColor: Theme.of(context).colorScheme.error,
              foregroundColor: Theme.of(context).colorScheme.onError,
            ),
            icon: const Icon(Icons.link_off),
            label: Text(
              _isDisconnecting ? 'Rozłączanie...' : 'Rozłącz doniczkę',
            ),
          ),
        ),
      ),
    );

    return isOwner ? DefaultTabController(length: 2, child: scaffold) : scaffold;
  }

  Widget _buildConfigurationForm(Pot pot) {
    return SafeArea(
      child: Form(
        key: _formKey,
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            Text(
              'Ustawienia ${pot.name}',
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
              validator: (_) => null,
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: TextFormField(
                    controller: _minTempController,
                    decoration: const InputDecoration(
                      labelText: 'Min. temperatura (°C)',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: const TextInputType.numberWithOptions(
                      signed: true,
                      decimal: true,
                    ),
                    textInputAction: TextInputAction.next,
                    validator: (value) {
                      final parsed = double.tryParse(value?.trim() ?? '');
                      if (parsed == null) {
                        return 'Podaj minimalną temperaturę.';
                      }
                      return null;
                    },
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: TextFormField(
                    controller: _maxTempController,
                    decoration: const InputDecoration(
                      labelText: 'Maks. temperatura (°C)',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: const TextInputType.numberWithOptions(
                      signed: true,
                      decimal: true,
                    ),
                    textInputAction: TextInputAction.next,
                    validator: (value) {
                      final parsed = double.tryParse(value?.trim() ?? '');
                      if (parsed == null) {
                        return 'Podaj maksymalną temperaturę.';
                      }
                      final minParsed =
                          double.tryParse(_minTempController.text.trim());
                      if (minParsed != null && parsed <= minParsed) {
                        return 'Musi być większa od minimalnej.';
                      }
                      return null;
                    },
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: TextFormField(
                    controller: _minMoistureController,
                    decoration: const InputDecoration(
                      labelText: 'Min. wilgotność (%)',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    textInputAction: TextInputAction.next,
                    validator: (value) {
                      final parsed = int.tryParse(value?.trim() ?? '');
                      if (parsed == null) {
                        return 'Podaj minimalną wilgotność.';
                      }
                      if (parsed < 0 || parsed > 100) {
                        return 'Zakres 0-100.';
                      }
                      return null;
                    },
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: TextFormField(
                    controller: _maxMoistureController,
                    decoration: const InputDecoration(
                      labelText: 'Maks. wilgotność (%)',
                      border: OutlineInputBorder(),
                    ),
                    keyboardType: TextInputType.number,
                    textInputAction: TextInputAction.next,
                    validator: (value) {
                      final parsed = int.tryParse(value?.trim() ?? '');
                      if (parsed == null) {
                        return 'Podaj maksymalną wilgotność.';
                      }
                      if (parsed < 0 || parsed > 100) {
                        return 'Zakres 0-100.';
                      }
                      final minParsed =
                          int.tryParse(_minMoistureController.text.trim());
                      if (minParsed != null && parsed < minParsed) {
                        return 'Musi być >= minimalnej.';
                      }
                      return null;
                    },
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            DropdownButtonFormField<String>(
              value: _illuminance,
              decoration: const InputDecoration(
                labelText: 'Nasłonecznienie',
                border: OutlineInputBorder(),
              ),
              items: const [
                DropdownMenuItem(value: 'low', child: Text('Niskie')),
                DropdownMenuItem(value: 'medium', child: Text('Średnie')),
                DropdownMenuItem(value: 'high', child: Text('Wysokie')),
              ],
              onChanged: (value) {
                if (value == null) return;
                setState(() {
                  _illuminance = value;
                });
              },
            ),
            const SizedBox(height: 16),
            kIsWeb
                ? _buildWebDurationFields(
                    label: 'Okres pomiaru',
                    hoursController: _measureHoursController,
                    minutesController: _measureMinutesController,
                    secondsController: _measureSecondsController,
                    onChanged: (value) {
                      setState(() {
                        _measurementIntervalSec = value;
                      });
                    },
                    validator: () {
                      if (_measurementIntervalSec <= 0) {
                        return 'Podaj okres pomiaru.';
                      }
                      return null;
                    },
                  )
                : TextFormField(
                    controller: _measurementPeriodController,
                    decoration: const InputDecoration(
                      labelText: 'Okres pomiaru',
                      hintText: 'Wybierz czas',
                      border: OutlineInputBorder(),
                    ),
                    readOnly: true,
                    onTap: () async {
                      final picked = await _showDurationPicker(
                        context,
                        initialSeconds: _measurementIntervalSec,
                      );
                      if (picked == null) return;
                      setState(() {
                        _measurementIntervalSec = picked;
                        _measurementPeriodController.text =
                            _formatDurationForPicker(picked);
                      });
                    },
                    textInputAction: TextInputAction.next,
                    validator: (_) {
                      if (_measurementIntervalSec <= 0) {
                        return 'Podaj okres pomiaru.';
                      }
                      return null;
                    },
                  ),
            const SizedBox(height: 16),
            kIsWeb
                ? _buildWebDurationFields(
                    label: 'Okres wysyłania',
                    hoursController: _sendHoursController,
                    minutesController: _sendMinutesController,
                    secondsController: _sendSecondsController,
                    onChanged: (value) {
                      setState(() {
                        _sendIntervalSec = value;
                      });
                    },
                    validator: () {
                      if (_sendIntervalSec <= 0) {
                        return 'Podaj okres wysyłania.';
                      }
                      return null;
                    },
                  )
                : TextFormField(
                    controller: _sendPeriodController,
                    decoration: const InputDecoration(
                      labelText: 'Okres wysyłania',
                      hintText: 'Wybierz czas',
                      border: OutlineInputBorder(),
                    ),
                    readOnly: true,
                    onTap: () async {
                      final picked = await _showDurationPicker(
                        context,
                        initialSeconds: _sendIntervalSec,
                      );
                      if (picked == null) return;
                      setState(() {
                        _sendIntervalSec = picked;
                        _sendPeriodController.text =
                            _formatDurationForPicker(picked);
                      });
                    },
                    textInputAction: TextInputAction.done,
                    validator: (_) {
                      if (_sendIntervalSec <= 0) {
                        return 'Podaj okres wysyłania.';
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
            kIsWeb
                ? _buildWebDurationFields(
                    label: 'Okres podlewania',
                    hoursController: _wateringHoursController,
                    minutesController: _wateringMinutesController,
                    secondsController: _wateringSecondsController,
                    enabled: _autoWateringEnabled,
                    onChanged: (value) {
                      setState(() {
                        _wateringIntervalSec = value;
                      });
                    },
                    validator: () {
                      if (!_autoWateringEnabled) {
                        return null;
                      }
                      if (_wateringIntervalSec <= 0) {
                        return 'Podaj okres podlewania.';
                      }
                      return null;
                    },
                  )
                : TextFormField(
                    controller: _wateringPeriodController,
                    decoration: const InputDecoration(
                      labelText: 'Okres podlewania',
                      hintText: 'Wybierz czas',
                      border: OutlineInputBorder(),
                    ),
                    enabled: _autoWateringEnabled,
                    readOnly: true,
                    onTap: _autoWateringEnabled
                        ? () async {
                            final picked = await _showDurationPicker(
                              context,
                              initialSeconds: _wateringIntervalSec,
                            );
                            if (picked == null) return;
                            setState(() {
                              _wateringIntervalSec = picked;
                              _wateringPeriodController.text =
                                  _formatDurationForPicker(picked);
                            });
                          }
                        : null,
                    textInputAction: TextInputAction.done,
                    validator: (_) {
                      if (!_autoWateringEnabled) {
                        return null;
                      }
                      if (_wateringIntervalSec <= 0) {
                        return 'Podaj okres podlewania.';
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
    );
  }

  Widget _buildPermissionsTab(Pot pot) {
    final connections = pot.connections
        .where((conn) => conn.role != PotRole.owner)
        .toList();

    return SafeArea(
      child: Form(
        key: _permissionsFormKey,
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            Text(
              'Uprawnienia do doniczki',
              style: Theme.of(context).textTheme.titleLarge,
            ),
            const SizedBox(height: 16),
            TextFormField(
              controller: _permissionEmailController,
              decoration: const InputDecoration(
                labelText: 'Email użytkownika',
                border: OutlineInputBorder(),
              ),
              keyboardType: TextInputType.emailAddress,
              textInputAction: TextInputAction.next,
              validator: (value) {
                final trimmed = value?.trim() ?? '';
                if (trimmed.isEmpty) {
                  return 'Podaj email.';
                }
                if (!trimmed.contains('@')) {
                  return 'Podaj poprawny email.';
                }
                return null;
              },
            ),
            const SizedBox(height: 12),
            DropdownButtonFormField<PotRole>(
              value: _newPermissionRole,
              decoration: const InputDecoration(
                labelText: 'Rola',
                border: OutlineInputBorder(),
              ),
              items: const [
                DropdownMenuItem(
                  value: PotRole.viewer,
                  child: Text('Viewer'),
                ),
                DropdownMenuItem(
                  value: PotRole.editor,
                  child: Text('Editor'),
                ),
              ],
              onChanged: (value) {
                if (value == null) return;
                setState(() {
                  _newPermissionRole = value;
                });
              },
            ),
            const SizedBox(height: 12),
            FilledButton(
              onPressed: _isPermissionsBusy ? null : () => _addPermission(pot),
              child: Text(_isPermissionsBusy ? 'Dodawanie...' : 'Dodaj dostęp'),
            ),
            const SizedBox(height: 24),
            Text(
              'Lista użytkowników',
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 12),
            if (connections.isEmpty)
              Text(
                'Brak dodatkowych użytkowników.',
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Colors.grey.shade600,
                ),
              )
            else
              ...connections.map(
                (conn) => Card(
                  child: ListTile(
                    title: Text(conn.email.isEmpty ? conn.userId : conn.email),
                    subtitle: Text('Rola: ${potRoleToString(conn.role)}'),
                    trailing: IconButton(
                      icon: const Icon(Icons.edit),
                      onPressed: () => _showEditPermissionDialog(pot, conn),
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Future<void> _addPermission(Pot pot) async {
    if (!_permissionsFormKey.currentState!.validate()) {
      return;
    }

    final email = _permissionEmailController.text.trim();
    final existing = pot.connections.any(
      (conn) => conn.email.toLowerCase() == email.toLowerCase(),
    );
    if (existing) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Użytkownik już ma dostęp. Użyj edycji.'),
        ),
      );
      return;
    }

    setState(() {
      _isPermissionsBusy = true;
    });

    try {
      await context.read<PotsController>().addPotConnection(
        pot.potId,
        email: email,
        role: potRoleToString(_newPermissionRole),
      );
      _permissionEmailController.clear();
      setState(() {
        _newPermissionRole = PotRole.viewer;
      });
      await context.read<PotsController>().fetchPots();
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Nie udało się dodać dostępu: $e')),
      );
    } finally {
      if (!mounted) return;
      setState(() {
        _isPermissionsBusy = false;
      });
    }
  }

  Future<void> _showEditPermissionDialog(Pot pot, PotConnection conn) async {
    PotRole selectedRole = conn.role;
    await showDialog<void>(
      context: context,
      builder: (dialogContext) {
        return StatefulBuilder(
          builder: (context, setDialogState) {
            return AlertDialog(
              title: const Text('Edytuj uprawnienia'),
              content: DropdownButtonFormField<PotRole>(
                value: selectedRole == PotRole.owner
                    ? PotRole.editor
                    : selectedRole,
                decoration: const InputDecoration(
                  labelText: 'Rola',
                  border: OutlineInputBorder(),
                ),
                items: const [
                  DropdownMenuItem(
                    value: PotRole.viewer,
                    child: Text('Viewer'),
                  ),
                  DropdownMenuItem(
                    value: PotRole.editor,
                    child: Text('Editor'),
                  ),
                ],
                onChanged: (value) {
                  if (value == null) return;
                  setDialogState(() {
                    selectedRole = value;
                  });
                },
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(dialogContext).pop(),
                  child: const Text('Anuluj'),
                ),
                TextButton(
                  onPressed: () async {
                    final shouldRemove = await _confirmRemove();
                    if (!shouldRemove) return;
                    if (!mounted) return;
                    await _removePermission(pot, conn);
                    if (mounted) {
                      Navigator.of(dialogContext).pop();
                    }
                  },
                  child: Text(
                    'Usuń',
                    style: TextStyle(
                      color: Theme.of(context).colorScheme.error,
                    ),
                  ),
                ),
                FilledButton(
                  onPressed: () async {
                    await _updatePermission(pot, conn, selectedRole);
                    if (mounted) {
                      Navigator.of(dialogContext).pop();
                    }
                  },
                  child: const Text('Zapisz'),
                ),
              ],
            );
          },
        );
      },
    );
  }

  Future<void> _updatePermission(
    Pot pot,
    PotConnection conn,
    PotRole newRole,
  ) async {
    setState(() {
      _isPermissionsBusy = true;
    });

    try {
      await context.read<PotsController>().updatePotConnection(
        pot.potId,
        connectionId: conn.id,
        email: conn.email,
        role: potRoleToString(newRole),
      );
      await context.read<PotsController>().fetchPots();
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Nie udało się zmienić roli: $e')),
      );
    } finally {
      if (!mounted) return;
      setState(() {
        _isPermissionsBusy = false;
      });
    }
  }

  Future<void> _removePermission(Pot pot, PotConnection conn) async {
    setState(() {
      _isPermissionsBusy = true;
    });

    try {
      await context.read<PotsController>().deletePotConnection(
        pot.potId,
        connectionId: conn.id,
        email: conn.email,
      );
      await context.read<PotsController>().fetchPots();
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Nie udało się usunąć dostępu: $e')),
      );
    } finally {
      if (!mounted) return;
      setState(() {
        _isPermissionsBusy = false;
      });
    }
  }

  Future<bool> _confirmRemove() async {
    return await showDialog<bool>(
          context: context,
          builder: (dialogContext) {
            return AlertDialog(
              title: const Text('Usunąć użytkownika?'),
              content: const Text('Użytkownik straci dostęp do doniczki.'),
              actions: [
                TextButton(
                  onPressed: () => Navigator.of(dialogContext).pop(false),
                  child: const Text('Anuluj'),
                ),
                FilledButton(
                  onPressed: () => Navigator.of(dialogContext).pop(true),
                  child: const Text('Usuń'),
                ),
              ],
            );
          },
        ) ??
        false;
  }

  void _redirectIfViewer() {
    final currentUserId = context.read<AuthController>().currentUser?.id ?? '';
    final pot = context.read<PotsController>().pots.firstWhere(
          (p) => p.potId == widget.pot.potId,
          orElse: () => widget.pot,
        );
    final role = _resolveRole(pot, currentUserId);
    if (role == PotRole.viewer || role == PotRole.unknown) {
      Navigator.of(context).maybePop();
    }
  }

  PotRole _resolveRole(Pot pot, String userId) {
    if (userId.isEmpty) return PotRole.unknown;
    final match = pot.connections.firstWhere(
      (conn) => conn.userId == userId,
      orElse: () => const PotConnection(
        id: '',
        userId: '',
        email: '',
        role: PotRole.unknown,
      ),
    );
    return match.role;
  }

  Future<bool> _confirmDisconnect() async {
    return await showDialog<bool>(
          context: context,
          builder: (dialogContext) {
            bool acknowledged = false;
            return StatefulBuilder(
              builder: (context, setDialogState) {
                return AlertDialog(
                  title: const Text('Rozłączyć doniczkę?'),
                  content: Column(
                    mainAxisSize: MainAxisSize.min,
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        'Ta operacja jest nieodwracalna. Doniczka zostanie odłączona od konta.',
                      ),
                      const SizedBox(height: 12),
                      CheckboxListTile(
                        contentPadding: EdgeInsets.zero,
                        title: const Text(
                          'Rozumiem, że tej operacji nie można cofnąć.',
                        ),
                        value: acknowledged,
                        onChanged: (value) {
                          setDialogState(() {
                            acknowledged = value ?? false;
                          });
                        },
                      ),
                    ],
                  ),
                  actions: [
                    TextButton(
                      onPressed: () => Navigator.of(dialogContext).pop(false),
                      child: const Text('Anuluj'),
                    ),
                    FilledButton(
                      onPressed: acknowledged
                          ? () => Navigator.of(dialogContext).pop(true)
                          : null,
                      child: const Text('Rozłącz'),
                    ),
                  ],
                );
              },
            );
          },
        ) ??
        false;
  }

  Future<int?> _showDurationPicker(
    BuildContext context, {
    required int initialSeconds,
  }) async {
    const int minute = 60;
    const int hour = 60 * minute;
    const int day = 24 * hour;

    final initialDays = initialSeconds ~/ day;
    final initialHours = (initialSeconds % day) ~/ hour;
    final initialMinutes = (initialSeconds % hour) ~/ minute;
    final initialSecs = initialSeconds % minute;

    return showModalBottomSheet<int>(
      context: context,
      builder: (sheetContext) {
        int days = initialDays;
        int hours = initialHours;
        int minutes = initialMinutes;
        int seconds = initialSecs;

        return StatefulBuilder(
          builder: (context, setModalState) {
            return SafeArea(
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Padding(
                    padding: const EdgeInsets.symmetric(
                      horizontal: 16,
                      vertical: 12,
                    ),
                    child: Row(
                      children: [
                        TextButton(
                          onPressed: () => Navigator.of(context).pop(),
                          child: const Text('Anuluj'),
                        ),
                        const Spacer(),
                        TextButton(
                          onPressed: () {
                            final total = (days * day) +
                                (hours * hour) +
                                (minutes * minute) +
                                seconds;
                            Navigator.of(context).pop(total);
                          },
                          child: const Text('Zapisz'),
                        ),
                      ],
                    ),
                  ),
                  Row(
                    children: [
                      _buildPickerColumn(
                        label: 'd',
                        itemCount: 31,
                        initialItem: days,
                        onChanged: (value) =>
                            setModalState(() => days = value),
                      ),
                      _buildPickerColumn(
                        label: 'h',
                        itemCount: 24,
                        initialItem: hours,
                        onChanged: (value) =>
                            setModalState(() => hours = value),
                      ),
                      _buildPickerColumn(
                        label: 'm',
                        itemCount: 60,
                        initialItem: minutes,
                        onChanged: (value) =>
                            setModalState(() => minutes = value),
                      ),
                      _buildPickerColumn(
                        label: 's',
                        itemCount: 60,
                        initialItem: seconds,
                        onChanged: (value) =>
                            setModalState(() => seconds = value),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                ],
              ),
            );
          },
        );
      },
    );
  }

  Widget _buildPickerColumn({
    required String label,
    required int itemCount,
    required int initialItem,
    required ValueChanged<int> onChanged,
  }) {
    return Expanded(
      child: Column(
        children: [
          Text(label, style: const TextStyle(fontWeight: FontWeight.w600)),
          SizedBox(
            height: 160,
            child: CupertinoPicker(
              itemExtent: 32,
              scrollController: FixedExtentScrollController(
                initialItem: initialItem.clamp(0, itemCount - 1),
              ),
              onSelectedItemChanged: onChanged,
              children: List.generate(
                itemCount,
                (index) => Center(child: Text(index.toString())),
              ),
            ),
          ),
        ],
      ),
    );
  }

  String _formatDurationForPicker(int seconds) {
    const int minute = 60;
    const int hour = 60 * minute;
    const int day = 24 * hour;

    final days = seconds ~/ day;
    final hours = (seconds % day) ~/ hour;
    final minutes = (seconds % hour) ~/ minute;
    final secs = seconds % minute;

    if (days > 0) {
      return '${days}d ${hours}h ${minutes}m ${secs}s';
    }
    return '${hours}h ${minutes}m ${secs}s';
  }

  void _setWebFieldsFromSeconds({
    required int seconds,
    required TextEditingController hours,
    required TextEditingController minutes,
    required TextEditingController secs,
  }) {
    const int minute = 60;
    const int hour = 60 * minute;
    final totalHours = seconds ~/ hour;
    final remaining = seconds % hour;
    hours.text = totalHours.toString();
    minutes.text = (remaining ~/ minute).toString();
    secs.text = (remaining % minute).toString();
  }

  int _parseWebDuration({
    required TextEditingController hours,
    required TextEditingController minutes,
    required TextEditingController secs,
  }) {
    final h = int.tryParse(hours.text.trim()) ?? 0;
    final m = int.tryParse(minutes.text.trim()) ?? 0;
    final s = int.tryParse(secs.text.trim()) ?? 0;
    return (h * 3600) + (m * 60) + s;
  }

  Widget _buildWebDurationFields({
    required String label,
    required TextEditingController hoursController,
    required TextEditingController minutesController,
    required TextEditingController secondsController,
    required ValueChanged<int> onChanged,
    required String? Function() validator,
    bool enabled = true,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(label, style: Theme.of(context).textTheme.bodyLarge),
        const SizedBox(height: 8),
        Row(
          children: [
            Expanded(
              child: TextFormField(
                controller: hoursController,
                decoration: const InputDecoration(
                  labelText: 'Godziny',
                  border: OutlineInputBorder(),
                ),
                enabled: enabled,
                keyboardType: TextInputType.number,
                onChanged: (_) => onChanged(
                  _parseWebDuration(
                    hours: hoursController,
                    minutes: minutesController,
                    secs: secondsController,
                  ),
                ),
              ),
            ),
            const SizedBox(width: 8),
            Expanded(
              child: TextFormField(
                controller: minutesController,
                decoration: const InputDecoration(
                  labelText: 'Minuty',
                  border: OutlineInputBorder(),
                ),
                enabled: enabled,
                keyboardType: TextInputType.number,
                onChanged: (_) => onChanged(
                  _parseWebDuration(
                    hours: hoursController,
                    minutes: minutesController,
                    secs: secondsController,
                  ),
                ),
              ),
            ),
            const SizedBox(width: 8),
            Expanded(
              child: TextFormField(
                controller: secondsController,
                decoration: const InputDecoration(
                  labelText: 'Sekundy',
                  border: OutlineInputBorder(),
                ),
                enabled: enabled,
                keyboardType: TextInputType.number,
                onChanged: (_) => onChanged(
                  _parseWebDuration(
                    hours: hoursController,
                    minutes: minutesController,
                    secs: secondsController,
                  ),
                ),
              ),
            ),
          ],
        ),
        Builder(
          builder: (_) {
            final message = validator();
            if (message == null) return const SizedBox.shrink();
            return Padding(
              padding: const EdgeInsets.only(top: 8),
              child: Text(
                message,
                style: TextStyle(
                  color: Theme.of(context).colorScheme.error,
                ),
              ),
            );
          },
        ),
      ],
    );
  }
}
