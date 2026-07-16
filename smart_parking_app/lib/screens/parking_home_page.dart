import 'dart:async';

import 'package:flutter/material.dart';

import '../models/parking_state.dart';
import '../services/parking_api.dart';
import '../services/server_discovery.dart';

class ParkingHomePage extends StatefulWidget {
  const ParkingHomePage({super.key});

  @override
  State<ParkingHomePage> createState() => _ParkingHomePageState();
}

class _ParkingHomePageState extends State<ParkingHomePage>
    with SingleTickerProviderStateMixin {
  static const String _defaultHost = String.fromEnvironment(
    'SERVER_HOST',
    defaultValue: '192.168.100.21:8000',
  );

  late ParkingApi _api;
  late TabController _tabController;
  late TextEditingController _serverController;
  StreamSubscription<ParkingState>? _stateSubscription;
  StreamSubscription<bool>? _connectionSubscription;
  Timer? _autoDiscoveryTimer;

  ParkingState? _parkingState;
  bool _connected = false;
  bool _busy = false;
  bool _discovering = false;
  String? _connectionError;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 4, vsync: this);
    _serverController = TextEditingController(text: _defaultHost);
    _api = ParkingApi(serverAddress: _defaultHost);
    _listenToApi();
    unawaited(_initializeConnection());
  }

  void _listenToApi() {
    _stateSubscription = _api.states.listen((state) {
      if (!mounted) return;
      setState(() {
        _parkingState = state;
        _connectionError = null;
      });

      final reserved = state.reservedSlotId;
      if (reserved.isNotEmpty) {
        final slot = state.slots
            .where((item) => item.id == reserved)
            .firstOrNull;
        if (slot != null && _tabController.index != slot.floorIndex) {
          _tabController.animateTo(slot.floorIndex);
        }
      }
    });

    _connectionSubscription = _api.connections.listen((connected) {
      if (!mounted) return;
      setState(() => _connected = connected);
      if (connected) {
        _autoDiscoveryTimer?.cancel();
      } else {
        _scheduleAutomaticRediscovery();
      }
    });
  }

  Future<void> _initializeConnection() async {
    final savedServer = await ServerDiscovery.loadSavedServer();
    if (savedServer != null && await ServerDiscovery.isReachable(savedServer)) {
      await _connectToServer(savedServer);
      return;
    }

    final detectedServer = await ServerDiscovery.discover();
    if (detectedServer != null) {
      await _connectToServer(detectedServer);
      return;
    }

    await _connectToServer(
      savedServer?.isNotEmpty == true ? savedServer! : _defaultHost,
      showError: true,
    );
  }

  Future<void> _connect() async {
    await _connectToServer(_api.serverAddress, showError: true);
  }

  Future<void> _connectToServer(
    String address, {
    bool showError = false,
  }) async {
    if (!mounted) return;
    _serverController.text = address;
    setState(() {
      _connectionError = null;
      _connected = false;
    });

    try {
      await _api.changeServer(address);
      await ServerDiscovery.saveServer(_api.serverAddress);
    } catch (error) {
      if (!mounted) return;
      if (showError) {
        setState(() => _connectionError = error.toString());
      }
      _scheduleAutomaticRediscovery();
    }
  }

  Future<void> _findServerAutomatically({bool showError = true}) async {
    if (_discovering) return;
    if (mounted) {
      setState(() {
        _discovering = true;
        _connectionError = null;
      });
    }

    try {
      final detectedServer = await ServerDiscovery.discover();
      if (detectedServer == null) {
        if (showError && mounted) {
          setState(() {
            _connectionError =
                'No se encontró la computadora Smart Parking en esta red Wi-Fi.';
          });
        }
        _scheduleAutomaticRediscovery();
        return;
      }
      await _connectToServer(detectedServer, showError: showError);
    } finally {
      if (mounted) setState(() => _discovering = false);
    }
  }

  void _scheduleAutomaticRediscovery() {
    if (!mounted || _connected || _autoDiscoveryTimer?.isActive == true) {
      return;
    }
    _autoDiscoveryTimer = Timer(const Duration(seconds: 6), () {
      unawaited(_findServerAutomatically(showError: false));
    });
  }

  Future<void> _changeServer() async {
    final controller = TextEditingController(text: _api.serverAddress);
    final value = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Dirección del servidor'),
        content: TextField(
          controller: controller,
          autofocus: true,
          keyboardType: TextInputType.url,
          decoration: const InputDecoration(
            labelText: 'IPv4 de la PC y puerto',
            hintText: '192.168.100.21:8000',
            prefixIcon: Icon(Icons.lan_outlined),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancelar'),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(context, controller.text.trim()),
            child: const Text('Conectar'),
          ),
        ],
      ),
    );

    if (value == null || value.isEmpty) return;
    _serverController.text = value;
    setState(() {
      _connectionError = null;
      _connected = false;
    });

    await _connectToServer(value, showError: true);
  }

  Future<void> _runCommand(Future<void> Function() action) async {
    if (_busy) return;
    setState(() => _busy = true);
    try {
      await action();
    } on ParkingApiException catch (error) {
      _showMessage(error.message, isError: true);
    } catch (error) {
      _showMessage(error.toString(), isError: true);
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _reserveSlot(ParkingSlot slot) async {
    if (slot.status != SlotStatus.available) return;

    final accepted = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        icon: const Icon(Icons.local_parking, size: 42),
        title: Text('Ir al espacio ${slot.id}'),
        content: Text(
          'El espacio cambiará de verde a amarillo. OpenGL calculará '
          'la ruta con Dijkstra.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Cancelar'),
          ),
          FilledButton.icon(
            onPressed: () => Navigator.pop(context, true),
            icon: const Icon(Icons.route),
            label: const Text('Mostrar ruta'),
          ),
        ],
      ),
    );

    if (accepted == true) {
      await _runCommand(() => _api.reserve(slot.id));
    }
  }

  void _showMessage(String message, {bool isError = false}) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: isError ? Colors.red.shade800 : null,
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final state = _parkingState;

    return Scaffold(
      appBar: AppBar(
        title: const Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Smart Parking'),
            Text(
              'Parqueadero EPN · Tiempo real',
              style: TextStyle(fontSize: 12, fontWeight: FontWeight.normal),
            ),
          ],
        ),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Chip(
              avatar: Icon(
                _connected ? Icons.cloud_done : Icons.cloud_off,
                size: 18,
                color: _connected ? Colors.green : Colors.red,
              ),
              label: Text(_connected ? 'Conectado' : 'Sin conexión'),
            ),
          ),
          IconButton(
            tooltip: 'Buscar la computadora automáticamente',
            onPressed: _discovering
                ? null
                : () => unawaited(_findServerAutomatically()),
            icon: _discovering
                ? const SizedBox.square(
                    dimension: 20,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.wifi_find),
          ),
          IconButton(
            tooltip: 'Cambiar servidor manualmente',
            onPressed: _changeServer,
            icon: const Icon(Icons.settings_ethernet),
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          isScrollable: false,
          tabs: const [
            Tab(text: 'PB'),
            Tab(text: 'Piso 1'),
            Tab(text: 'Piso 2'),
            Tab(text: 'Piso 3'),
          ],
        ),
      ),
      body: Column(
        children: [
          if (_connectionError != null)
            MaterialBanner(
              content: Text(
                'No se pudo conectar a ${_api.serverAddress}. '
                '$_connectionError',
              ),
              leading: const Icon(Icons.wifi_off, color: Colors.red),
              actions: [
                TextButton(
                  onPressed: _discovering
                      ? null
                      : () => unawaited(_findServerAutomatically()),
                  child: const Text('Buscar PC'),
                ),
                TextButton(
                  onPressed: _connect,
                  child: const Text('Reintentar'),
                ),
                TextButton(
                  onPressed: _changeServer,
                  child: const Text('Cambiar IP'),
                ),
              ],
            ),
          if (state?.waitingForOpenGL == true)
            const MaterialBanner(
              content: Text(
                'El servidor está activo, pero OpenGL todavía no ha generado '
                'el estado del parqueadero.',
              ),
              leading: Icon(Icons.desktop_windows_outlined),
              actions: [SizedBox.shrink()],
            ),
          if (state != null) _SummarySection(state: state),
          if (state?.guidanceActive == true) _GuidanceBanner(state: state!),
          Expanded(
            child: state == null
                ? const Center(child: CircularProgressIndicator())
                : TabBarView(
                    controller: _tabController,
                    children: List.generate(
                      4,
                      (floorIndex) => _FloorView(
                        state: state,
                        floorIndex: floorIndex,
                        onSlotTap: _reserveSlot,
                      ),
                    ),
                  ),
          ),
          _BottomActions(
            busy: _busy,
            hasActiveRoute: state?.guidanceActive == true,
            onSimulateRandom: () => _runCommand(_api.simulateRandom),
            onReserveBest: () => _runCommand(_api.reserveBest),
            onRouteExit: () => _runCommand(_api.routeToExit),
            onCancel: () => _runCommand(_api.cancelReservation),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    _stateSubscription?.cancel();
    _connectionSubscription?.cancel();
    _autoDiscoveryTimer?.cancel();
    unawaited(_api.dispose());
    _tabController.dispose();
    _serverController.dispose();
    super.dispose();
  }
}

class _SummarySection extends StatelessWidget {
  const _SummarySection({required this.state});

  final ParkingState state;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      padding: const EdgeInsets.fromLTRB(12, 12, 12, 4),
      child: Row(
        children: [
          _SummaryCard(
            label: 'Disponibles',
            value: state.available,
            color: Colors.green,
            icon: Icons.check_circle,
          ),
          _SummaryCard(
            label: 'Ocupados',
            value: state.occupied,
            color: Colors.red,
            icon: Icons.directions_car,
          ),
          _SummaryCard(
            label: 'Reservados',
            value: state.reserved,
            color: Colors.amber,
            icon: Icons.bookmark,
          ),
          _SummaryCard(
            label: 'Accesibles',
            value: state.disabled,
            color: Colors.blue,
            icon: Icons.accessible,
          ),
        ],
      ),
    );
  }
}

class _SummaryCard extends StatelessWidget {
  const _SummaryCard({
    required this.label,
    required this.value,
    required this.color,
    required this.icon,
  });

  final String label;
  final int value;
  final Color color;
  final IconData icon;

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(right: 8),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
        child: Row(
          children: [
            CircleAvatar(
              backgroundColor: color.withValues(alpha: 0.18),
              child: Icon(icon, color: color),
            ),
            const SizedBox(width: 10),
            Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('$value', style: Theme.of(context).textTheme.titleLarge),
                Text(label),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _GuidanceBanner extends StatelessWidget {
  const _GuidanceBanner({required this.state});

  final ParkingState state;

  @override
  Widget build(BuildContext context) {
    final isExit = state.guidanceType.toUpperCase() == 'EXIT';
    final target = state.routeTargetId.isNotEmpty
        ? state.routeTargetId
        : state.reservedSlotId;
    final color = isExit ? Colors.green : Colors.cyan;

    return Container(
      width: double.infinity,
      margin: const EdgeInsets.fromLTRB(12, 8, 12, 4),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.14),
        border: Border.all(color: color),
        borderRadius: BorderRadius.circular(14),
      ),
      child: Row(
        children: [
          Icon(isExit ? Icons.exit_to_app : Icons.route, color: color),
          const SizedBox(width: 10),
          Expanded(
            child: Text(
              isExit
                  ? 'Ruta Dijkstra activa hacia la salida. En OpenGL se '
                        'utiliza únicamente la rampa derecha de bajada.'
                  : 'Ruta Dijkstra activa hacia $target. Sigue las flechas '
                        'brillantes y el círculo de misión en OpenGL.',
              style: const TextStyle(fontWeight: FontWeight.w600),
            ),
          ),
        ],
      ),
    );
  }
}

class _FloorView extends StatelessWidget {
  const _FloorView({
    required this.state,
    required this.floorIndex,
    required this.onSlotTap,
  });

  final ParkingState state;
  final int floorIndex;
  final ValueChanged<ParkingSlot> onSlotTap;

  @override
  Widget build(BuildContext context) {
    final slots = state.slotsForFloor(floorIndex);
    final floor = state.floors
        .where((item) => item.level == floorIndex)
        .firstOrNull;

    if (slots.isEmpty) {
      return const Center(
        child: Text('Esperando los espacios reales enviados por OpenGL...'),
      );
    }

    return Column(
      children: [
        if (floor != null)
          Padding(
            padding: const EdgeInsets.fromLTRB(14, 8, 14, 2),
            child: Row(
              children: [
                Text(
                  floor.name,
                  style: Theme.of(context).textTheme.titleMedium,
                ),
                const Spacer(),
                Text('${floor.available} verdes de ${floor.total}'),
              ],
            ),
          ),
        const _Legend(),
        Expanded(
          child: LayoutBuilder(
            builder: (context, constraints) {
              final columns = constraints.maxWidth >= 800
                  ? 8
                  : constraints.maxWidth >= 520
                  ? 6
                  : 4;

              return GridView.builder(
                padding: const EdgeInsets.all(12),
                gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                  crossAxisCount: columns,
                  crossAxisSpacing: 8,
                  mainAxisSpacing: 8,
                  childAspectRatio: 0.7,
                ),
                itemCount: slots.length,
                itemBuilder: (context, index) {
                  final slot = slots[index];
                  return _SlotCard(slot: slot, onTap: () => onSlotTap(slot));
                },
              );
            },
          ),
        ),
      ],
    );
  }
}

class _Legend extends StatelessWidget {
  const _Legend();

  @override
  Widget build(BuildContext context) {
    return const Padding(
      padding: EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      child: Wrap(
        spacing: 12,
        runSpacing: 6,
        children: [
          _LegendItem(color: Colors.green, label: 'Disponible'),
          _LegendItem(color: Colors.red, label: 'Ocupado'),
          _LegendItem(color: Colors.amber, label: 'Reservado'),
          _LegendItem(color: Colors.blue, label: 'Accesible'),
        ],
      ),
    );
  }
}

class _LegendItem extends StatelessWidget {
  const _LegendItem({required this.color, required this.label});

  final Color color;
  final String label;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: 11,
          height: 11,
          decoration: BoxDecoration(color: color, shape: BoxShape.circle),
        ),
        const SizedBox(width: 5),
        Text(label, style: Theme.of(context).textTheme.bodySmall),
      ],
    );
  }
}

class _SlotCard extends StatelessWidget {
  const _SlotCard({required this.slot, required this.onTap});

  final ParkingSlot slot;
  final VoidCallback onTap;

  Color get _color {
    switch (slot.status) {
      case SlotStatus.available:
        return Colors.green;
      case SlotStatus.occupied:
        return Colors.red;
      case SlotStatus.reserved:
        return Colors.amber;
      case SlotStatus.disabled:
        return Colors.blue;
      case SlotStatus.unknown:
        return Colors.grey;
    }
  }

  IconData get _icon {
    switch (slot.status) {
      case SlotStatus.available:
        return Icons.local_parking;
      case SlotStatus.occupied:
        return Icons.directions_car;
      case SlotStatus.reserved:
        return Icons.bookmark;
      case SlotStatus.disabled:
        return Icons.accessible;
      case SlotStatus.unknown:
        return Icons.help_outline;
    }
  }

  @override
  Widget build(BuildContext context) {
    final enabled = slot.status == SlotStatus.available;

    return Material(
      color: _color.withValues(alpha: enabled ? 0.18 : 0.13),
      borderRadius: BorderRadius.circular(12),
      child: InkWell(
        onTap: enabled ? onTap : null,
        borderRadius: BorderRadius.circular(12),
        child: Container(
          decoration: BoxDecoration(
            border: Border.all(color: _color, width: 1.5),
            borderRadius: BorderRadius.circular(12),
          ),
          padding: const EdgeInsets.all(8),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(_icon, color: _color),
              const SizedBox(height: 4),
              FittedBox(
                child: Text(
                  slot.id,
                  style: const TextStyle(fontWeight: FontWeight.bold),
                ),
              ),
              if (enabled)
                const Text('Toca para reservar', textAlign: TextAlign.center),
            ],
          ),
        ),
      ),
    );
  }
}

class _BottomActions extends StatelessWidget {
  const _BottomActions({
    required this.busy,
    required this.hasActiveRoute,
    required this.onSimulateRandom,
    required this.onReserveBest,
    required this.onRouteExit,
    required this.onCancel,
  });

  final bool busy;
  final bool hasActiveRoute;
  final VoidCallback onSimulateRandom;
  final VoidCallback onReserveBest;
  final VoidCallback onRouteExit;
  final VoidCallback onCancel;

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      top: false,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 8, 12, 12),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            SizedBox(
              width: double.infinity,
              child: FilledButton.icon(
                onPressed: busy ? null : onSimulateRandom,
                icon: busy
                    ? const SizedBox.square(
                        dimension: 18,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.casino_outlined),
                label: const Text('Simular carros aleatoriamente'),
              ),
            ),
            const SizedBox(height: 8),
            Row(
              children: [
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: busy ? null : onReserveBest,
                    icon: const Icon(Icons.alt_route),
                    label: const Text('Espacio más cercano'),
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: OutlinedButton.icon(
                    onPressed: busy ? null : onRouteExit,
                    icon: const Icon(Icons.exit_to_app),
                    label: const Text('Ir a la salida'),
                  ),
                ),
              ],
            ),
            if (hasActiveRoute) ...[
              const SizedBox(height: 8),
              SizedBox(
                width: double.infinity,
                child: TextButton.icon(
                  onPressed: busy ? null : onCancel,
                  icon: const Icon(Icons.cancel_outlined),
                  label: const Text('Cancelar guía activa'),
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }
}

extension _FirstOrNullExtension<T> on Iterable<T> {
  T? get firstOrNull {
    final iterator = this.iterator;
    return iterator.moveNext() ? iterator.current : null;
  }
}
