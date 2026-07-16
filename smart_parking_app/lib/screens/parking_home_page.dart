import 'dart:async';

import 'package:flutter/material.dart';

import '../models/parking_state.dart';
import '../services/parking_api.dart';
import '../services/server_discovery.dart';
import '../widgets/parking_map.dart';

class ParkingHomePage extends StatefulWidget {
  const ParkingHomePage({super.key});

  @override
  State<ParkingHomePage> createState() => _ParkingHomePageState();
}

class _ParkingHomePageState extends State<ParkingHomePage>
    with SingleTickerProviderStateMixin {
  static const String _fallbackHost = String.fromEnvironment(
    'SERVER_HOST',
    defaultValue: '127.0.0.1:8000',
  );

  late final TabController _tabController;
  late ParkingApi _api;
  StreamSubscription<ParkingState>? _stateSubscription;
  StreamSubscription<bool>? _connectionSubscription;

  ParkingState? _state;
  bool _connected = false;
  bool _busy = false;
  bool _discovering = true;
  String _serverAddress = _fallbackHost;
  String? _message;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 4, vsync: this);
    _api = ParkingApi(serverAddress: _fallbackHost);
    _listenToApi();
    unawaited(_initializeConnection());
  }

  void _listenToApi() {
    _stateSubscription = _api.states.listen((state) {
      if (!mounted) return;
      setState(() {
        _state = state;
        _message = null;
      });

      final floor = state.vehicle.floor;
      if (floor >= 0 && floor < 4 && _tabController.index != floor) {
        _tabController.animateTo(floor);
      }
    });

    _connectionSubscription = _api.connections.listen((connected) {
      if (!mounted) return;
      setState(() => _connected = connected);
    });
  }

  Future<void> _initializeConnection() async {
    setState(() => _discovering = true);
    String? address;

    final saved = await ServerDiscovery.loadSavedServer();
    if (saved != null && await ServerDiscovery.isReachable(saved)) {
      address = saved;
    }
    address ??= await ServerDiscovery.discover();

    if (!mounted) return;
    setState(() => _discovering = false);

    if (address == null) {
      setState(() {
        _message = 'No se encontró la computadora automáticamente. '
            'Comprueba que ambos equipos estén en la misma red Wi-Fi.';
      });
      return;
    }

    await _connectTo(address);
  }

  Future<void> _connectTo(String address) async {
    if (address.trim().isEmpty) return;
    setState(() {
      _serverAddress = address.trim();
      _message = null;
    });

    try {
      await _api.changeServer(address.trim());
      await ServerDiscovery.saveServer(address.trim());
    } catch (error) {
      if (!mounted) return;
      setState(() => _message = 'No se pudo conectar: $error');
    }
  }

  Future<void> _manualServerDialog() async {
    final controller = TextEditingController(text: _serverAddress);
    final result = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Conectar con la computadora'),
        content: TextField(
          controller: controller,
          autofocus: true,
          keyboardType: TextInputType.url,
          decoration: const InputDecoration(
            labelText: 'IPv4 y puerto',
            hintText: '192.168.1.25:8000',
            prefixIcon: Icon(Icons.wifi),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancelar'),
          ),
          FilledButton.icon(
            onPressed: () => Navigator.pop(context, controller.text.trim()),
            icon: const Icon(Icons.link),
            label: const Text('Conectar'),
          ),
        ],
      ),
    );
    controller.dispose();
    if (result != null && result.isNotEmpty) {
      await _connectTo(result);
    }
  }

  Future<void> _run(Future<void> Function() command) async {
    if (_busy || !_connected) return;
    setState(() => _busy = true);
    try {
      await command();
    } on ParkingApiException catch (error) {
      if (mounted) setState(() => _message = error.message);
    } catch (error) {
      if (mounted) setState(() => _message = error.toString());
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _reserve(ParkingSlot slot) async {
    if (slot.status != SlotStatus.available) {
      final label = switch (slot.status) {
        SlotStatus.occupied => 'ocupado',
        SlotStatus.reserved => 'reservado',
        SlotStatus.disabled => 'accesible',
        _ => 'no disponible',
      };
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('${slot.id} está $label.')),
      );
      return;
    }

    final accepted = await showModalBottomSheet<bool>(
      context: context,
      showDragHandle: true,
      builder: (context) => Padding(
        padding: const EdgeInsets.fromLTRB(24, 4, 24, 28),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(
              'Reservar ${slot.id}',
              style: Theme.of(context).textTheme.headlineSmall,
            ),
            const SizedBox(height: 8),
            Text('Piso ${slot.floor} · fila ${slot.row} · lugar ${slot.number}'),
            const SizedBox(height: 18),
            FilledButton.icon(
              onPressed: () => Navigator.pop(context, true),
              icon: const Icon(Icons.alt_route),
              label: const Text('Calcular ruta con Dijkstra'),
            ),
          ],
        ),
      ),
    );

    if (accepted == true) {
      await _run(() => _api.reserve(slot.id));
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = _state;
    return Scaffold(
      appBar: AppBar(
        title: const Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Smart Parking'),
            Text(
              'Navegación interna en tiempo real',
              style: TextStyle(fontSize: 11, fontWeight: FontWeight.w400),
            ),
          ],
        ),
        actions: [
          _ConnectionPill(
            connected: _connected,
            discovering: _discovering,
          ),
          IconButton(
            tooltip: 'Buscar computadora en Wi-Fi',
            onPressed: _discovering ? null : _initializeConnection,
            icon: const Icon(Icons.radar),
          ),
          IconButton(
            tooltip: 'Configurar dirección manual',
            onPressed: _manualServerDialog,
            icon: const Icon(Icons.settings_ethernet),
          ),
        ],
      ),
      body: Column(
        children: [
          if (_message != null)
            MaterialBanner(
              leading: const Icon(Icons.info_outline),
              content: Text(_message!),
              actions: [
                TextButton(
                  onPressed: _initializeConnection,
                  child: const Text('Buscar otra vez'),
                ),
                TextButton(
                  onPressed: _manualServerDialog,
                  child: const Text('Ingresar IP'),
                ),
              ],
            ),
          if (state?.waitingForOpenGL == true)
            const MaterialBanner(
              leading: Icon(Icons.desktop_windows_outlined),
              content: Text(
                'La API está encendida. Inicia OpenGL para recibir la posición '
                'del Mazda y el estado de las plazas.',
              ),
              actions: [SizedBox.shrink()],
            ),
          Expanded(
            child: state == null
                ? _LoadingPanel(
                    discovering: _discovering,
                    serverAddress: _serverAddress,
                  )
                : RefreshIndicator(
                    onRefresh: () async {
                      try {
                        final refreshed = await _api.fetchState();
                        if (mounted) setState(() => _state = refreshed);
                      } catch (_) {}
                    },
                    child: ListView(
                      padding: const EdgeInsets.fromLTRB(14, 12, 14, 120),
                      children: [
                        _VehicleNavigationCard(state: state),
                        const SizedBox(height: 12),
                        _SummaryRow(state: state),
                        const SizedBox(height: 14),
                        Text(
                          'Mapa en vivo',
                          style: Theme.of(context).textTheme.titleLarge,
                        ),
                        const SizedBox(height: 8),
                        AnimatedBuilder(
                          animation: _tabController,
                          builder: (context, _) => ParkingMap(
                            state: state,
                            floorIndex: _tabController.index,
                          ),
                        ),
                        const SizedBox(height: 14),
                        Card(
                          clipBehavior: Clip.antiAlias,
                          child: Column(
                            children: [
                              TabBar(
                                controller: _tabController,
                                onTap: (_) => setState(() {}),
                                tabs: const [
                                  Tab(text: 'PB'),
                                  Tab(text: 'P1'),
                                  Tab(text: 'P2'),
                                  Tab(text: 'P3'),
                                ],
                              ),
                              SizedBox(
                                height: 370,
                                child: TabBarView(
                                  controller: _tabController,
                                  children: List.generate(
                                    4,
                                    (floor) => _FloorGrid(
                                      slots: state.slotsForFloor(floor),
                                      onTap: _reserve,
                                      reservedSlotId: state.reservedSlotId,
                                      parkedSlotId: state.vehicle.parkedSlotId,
                                    ),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        ),
                        const SizedBox(height: 8),
                        const _Legend(),
                      ],
                    ),
                  ),
          ),
        ],
      ),
      bottomNavigationBar: SafeArea(
        minimum: const EdgeInsets.fromLTRB(12, 6, 12, 10),
        child: _ActionPanel(
          busy: _busy,
          enabled: _connected,
          hasRoute: state?.navigation.active == true,
          onReserveBest: () => _run(_api.reserveBest),
          onRouteExit: () => _run(_api.routeToExit),
          onRandom: () => _run(_api.simulateRandom),
          onCancel: () => _run(_api.cancelReservation),
        ),
      ),
    );
  }

  @override
  void dispose() {
    _stateSubscription?.cancel();
    _connectionSubscription?.cancel();
    unawaited(_api.dispose());
    _tabController.dispose();
    super.dispose();
  }
}

class _VehicleNavigationCard extends StatelessWidget {
  const _VehicleNavigationCard({required this.state});

  final ParkingState state;

  @override
  Widget build(BuildContext context) {
    final vehicle = state.vehicle;
    final navigation = state.navigation;
    final parked = vehicle.parkedSlotId.isNotEmpty;

    return Card(
      color: const Color(0xFF10233A),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    color: const Color(0xFF1D4062),
                    borderRadius: BorderRadius.circular(14),
                  ),
                  child: const Icon(Icons.directions_car_filled, size: 30),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        parked
                            ? 'Mazda estacionado en ${vehicle.parkedSlotId}'
                            : 'Mazda en ${vehicle.floorName}',
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                              fontWeight: FontWeight.w700,
                            ),
                      ),
                      Text(
                        'X ${vehicle.x.toStringAsFixed(1)} · '
                        'Z ${vehicle.z.toStringAsFixed(1)} · '
                        '${vehicle.speedKmh.toStringAsFixed(1)} km/h',
                      ),
                    ],
                  ),
                ),
                Icon(
                  navigation.active ? Icons.navigation : Icons.location_pin,
                  color: navigation.active
                      ? const Color(0xFF25D8FF)
                      : const Color(0xFF70E29A),
                ),
              ],
            ),
            const Divider(height: 26),
            Text(
              navigation.instruction,
              style: Theme.of(context).textTheme.titleLarge?.copyWith(
                    color: const Color(0xFF8EEBFF),
                    fontWeight: FontWeight.w700,
                  ),
            ),
            const SizedBox(height: 4),
            Text(
              navigation.active
                  ? 'Destino ${navigation.destination} · '
                      '${navigation.distanceRemaining.toStringAsFixed(0)} m restantes'
                  : parked
                      ? 'Pulsa “Buscar salida” para abandonar el parqueadero.'
                      : 'Selecciona una plaza verde o busca la más cercana.',
            ),
          ],
        ),
      ),
    );
  }
}

class _SummaryRow extends StatelessWidget {
  const _SummaryRow({required this.state});

  final ParkingState state;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        children: [
          _Counter('Disponibles', state.available, const Color(0xFF22E66B)),
          _Counter('Ocupados', state.occupied, const Color(0xFFFF3B30)),
          _Counter('Reservados', state.reserved, const Color(0xFFFFC928)),
          _Counter('Accesibles', state.disabled, const Color(0xFF2F8CFF)),
        ],
      ),
    );
  }
}

class _Counter extends StatelessWidget {
  const _Counter(this.label, this.value, this.color);

  final String label;
  final int value;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.only(right: 8),
      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.13),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: color.withValues(alpha: 0.45)),
      ),
      child: Row(
        children: [
          Container(
            width: 10,
            height: 10,
            decoration: BoxDecoration(color: color, shape: BoxShape.circle),
          ),
          const SizedBox(width: 8),
          Text('$label  ', style: const TextStyle(fontSize: 12)),
          Text(
            '$value',
            style: TextStyle(color: color, fontWeight: FontWeight.w800),
          ),
        ],
      ),
    );
  }
}

class _FloorGrid extends StatelessWidget {
  const _FloorGrid({
    required this.slots,
    required this.onTap,
    required this.reservedSlotId,
    required this.parkedSlotId,
  });

  final List<ParkingSlot> slots;
  final ValueChanged<ParkingSlot> onTap;
  final String reservedSlotId;
  final String parkedSlotId;

  Color _color(SlotStatus status) => switch (status) {
        SlotStatus.available => const Color(0xFF22E66B),
        SlotStatus.occupied => const Color(0xFFFF3B30),
        SlotStatus.reserved => const Color(0xFFFFC928),
        SlotStatus.disabled => const Color(0xFF2F8CFF),
        SlotStatus.unknown => Colors.grey,
      };

  @override
  Widget build(BuildContext context) {
    return GridView.builder(
      padding: const EdgeInsets.all(12),
      gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
        crossAxisCount: 5,
        childAspectRatio: 1.25,
        crossAxisSpacing: 7,
        mainAxisSpacing: 7,
      ),
      itemCount: slots.length,
      itemBuilder: (context, index) {
        final slot = slots[index];
        final color = _color(slot.status);
        final isMazda = slot.id == parkedSlotId;
        final isTarget = slot.id == reservedSlotId;
        return InkWell(
          borderRadius: BorderRadius.circular(12),
          onTap: () => onTap(slot),
          child: Ink(
            decoration: BoxDecoration(
              color: color.withValues(alpha: 0.14),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: color,
                width: isMazda || isTarget ? 2.7 : 1,
              ),
            ),
            child: Stack(
              alignment: Alignment.center,
              children: [
                Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text(
                      '${slot.row}${slot.number.toString().padLeft(2, '0')}',
                      style: TextStyle(
                        color: color,
                        fontWeight: FontWeight.w800,
                      ),
                    ),
                    if (isMazda)
                      const Icon(Icons.directions_car, size: 17)
                    else if (isTarget)
                      const Icon(Icons.flag, size: 17),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}

class _ActionPanel extends StatelessWidget {
  const _ActionPanel({
    required this.busy,
    required this.enabled,
    required this.hasRoute,
    required this.onReserveBest,
    required this.onRouteExit,
    required this.onRandom,
    required this.onCancel,
  });

  final bool busy;
  final bool enabled;
  final bool hasRoute;
  final VoidCallback onReserveBest;
  final VoidCallback onRouteExit;
  final VoidCallback onRandom;
  final VoidCallback onCancel;

  @override
  Widget build(BuildContext context) {
    final active = enabled && !busy;
    return Material(
      elevation: 10,
      borderRadius: BorderRadius.circular(22),
      color: const Color(0xFF122239),
      child: Padding(
        padding: const EdgeInsets.all(9),
        child: Row(
          children: [
            Expanded(
              child: FilledButton.icon(
                onPressed: active ? onReserveBest : null,
                icon: const Icon(Icons.local_parking),
                label: const Text('Buscar lugar'),
              ),
            ),
            const SizedBox(width: 7),
            Expanded(
              child: FilledButton.tonalIcon(
                onPressed: active ? onRouteExit : null,
                icon: const Icon(Icons.exit_to_app),
                label: const Text('Buscar salida'),
              ),
            ),
            PopupMenuButton<String>(
              enabled: active,
              tooltip: 'Más controles',
              onSelected: (value) {
                if (value == 'random') onRandom();
                if (value == 'cancel') onCancel();
              },
              itemBuilder: (context) => [
                const PopupMenuItem(
                  value: 'random',
                  child: ListTile(
                    leading: Icon(Icons.casino),
                    title: Text('Nueva simulación (G)'),
                    subtitle: Text('Genera los 5 tipos de autos'),
                  ),
                ),
                if (hasRoute)
                  const PopupMenuItem(
                    value: 'cancel',
                    child: ListTile(
                      leading: Icon(Icons.route_outlined),
                      title: Text('Cancelar ruta'),
                    ),
                  ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _ConnectionPill extends StatelessWidget {
  const _ConnectionPill({
    required this.connected,
    required this.discovering,
  });

  final bool connected;
  final bool discovering;

  @override
  Widget build(BuildContext context) {
    final color = connected
        ? const Color(0xFF22E66B)
        : discovering
            ? const Color(0xFFFFC928)
            : const Color(0xFFFF5A52);
    final text = connected
        ? 'Wi-Fi'
        : discovering
            ? 'Buscando'
            : 'Sin conexión';
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 5),
      padding: const EdgeInsets.symmetric(horizontal: 9, vertical: 5),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.13),
        borderRadius: BorderRadius.circular(30),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          if (discovering)
            SizedBox(
              width: 11,
              height: 11,
              child: CircularProgressIndicator(strokeWidth: 2, color: color),
            )
          else
            Icon(Icons.circle, size: 10, color: color),
          const SizedBox(width: 5),
          Text(text, style: TextStyle(color: color, fontSize: 11)),
        ],
      ),
    );
  }
}

class _LoadingPanel extends StatelessWidget {
  const _LoadingPanel({
    required this.discovering,
    required this.serverAddress,
  });

  final bool discovering;
  final String serverAddress;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.local_parking, size: 74),
            const SizedBox(height: 18),
            if (discovering) const CircularProgressIndicator(),
            const SizedBox(height: 16),
            Text(
              discovering
                  ? 'Buscando el servidor Smart Parking en tu red Wi-Fi…'
                  : 'Esperando datos de $serverAddress',
              textAlign: TextAlign.center,
            ),
          ],
        ),
      ),
    );
  }
}

class _Legend extends StatelessWidget {
  const _Legend();

  @override
  Widget build(BuildContext context) {
    return Wrap(
      spacing: 12,
      runSpacing: 6,
      children: const [
        _LegendItem('Disponible', Color(0xFF22E66B)),
        _LegendItem('Ocupado', Color(0xFFFF3B30)),
        _LegendItem('Reservado', Color(0xFFFFC928)),
        _LegendItem('Accesible', Color(0xFF2F8CFF)),
      ],
    );
  }
}

class _LegendItem extends StatelessWidget {
  const _LegendItem(this.text, this.color);

  final String text;
  final Color color;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: 10,
          height: 10,
          decoration: BoxDecoration(color: color, shape: BoxShape.circle),
        ),
        const SizedBox(width: 5),
        Text(text, style: const TextStyle(fontSize: 12)),
      ],
    );
  }
}
