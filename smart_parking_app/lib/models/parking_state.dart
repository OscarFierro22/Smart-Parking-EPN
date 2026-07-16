enum SlotStatus {
  available,
  occupied,
  reserved,
  disabled,
  unknown;

  static SlotStatus fromApi(String value) {
    switch (value.toUpperCase()) {
      case 'AVAILABLE':
        return SlotStatus.available;
      case 'OCCUPIED':
        return SlotStatus.occupied;
      case 'RESERVED':
        return SlotStatus.reserved;
      case 'DISABLED':
        return SlotStatus.disabled;
      default:
        return SlotStatus.unknown;
    }
  }
}

class ParkingSlot {
  const ParkingSlot({
    required this.id,
    required this.floor,
    required this.floorIndex,
    required this.row,
    required this.number,
    required this.status,
    required this.x,
    required this.y,
    required this.z,
    required this.carType,
  });

  final String id;
  final String floor;
  final int floorIndex;
  final String row;
  final int number;
  final SlotStatus status;
  final double x;
  final double y;
  final double z;
  final int carType;

  factory ParkingSlot.fromJson(Map<String, dynamic> json) {
    return ParkingSlot(
      id: json['id']?.toString() ?? '',
      floor: json['floor']?.toString() ?? '',
      floorIndex: (json['floor_index'] as num?)?.toInt() ?? 0,
      row: json['row']?.toString() ?? '',
      number: (json['number'] as num?)?.toInt() ?? 0,
      status: SlotStatus.fromApi(json['state']?.toString() ?? ''),
      x: (json['x'] as num?)?.toDouble() ?? 0,
      y: (json['y'] as num?)?.toDouble() ?? 0,
      z: (json['z'] as num?)?.toDouble() ?? 0,
      carType: (json['car_type'] as num?)?.toInt() ?? 0,
    );
  }
}

class ParkingFloor {
  const ParkingFloor({
    required this.id,
    required this.name,
    required this.level,
    required this.total,
    required this.available,
    required this.occupied,
    required this.reserved,
    required this.disabled,
  });

  final String id;
  final String name;
  final int level;
  final int total;
  final int available;
  final int occupied;
  final int reserved;
  final int disabled;

  factory ParkingFloor.fromJson(Map<String, dynamic> json) {
    int readInt(String key) => (json[key] as num?)?.toInt() ?? 0;

    return ParkingFloor(
      id: json['id']?.toString() ?? '',
      name: json['name']?.toString() ?? '',
      level: readInt('level'),
      total: readInt('total'),
      available: readInt('available'),
      occupied: readInt('occupied'),
      reserved: readInt('reserved'),
      disabled: readInt('disabled'),
    );
  }
}

class VehicleState {
  const VehicleState({
    required this.x,
    required this.y,
    required this.z,
    required this.floor,
    required this.floorName,
    required this.headingDegrees,
    required this.speedKmh,
    required this.parkedSlotId,
  });

  final double x;
  final double y;
  final double z;
  final int floor;
  final String floorName;
  final double headingDegrees;
  final double speedKmh;
  final String parkedSlotId;

  factory VehicleState.fromJson(Map<String, dynamic> json) {
    return VehicleState(
      x: (json['x'] as num?)?.toDouble() ?? 0,
      y: (json['y'] as num?)?.toDouble() ?? 0,
      z: (json['z'] as num?)?.toDouble() ?? 0,
      floor: (json['floor'] as num?)?.toInt() ?? -1,
      floorName: json['floor_name']?.toString() ?? 'Calle',
      headingDegrees:
          (json['heading_degrees'] as num?)?.toDouble() ?? 0,
      speedKmh: (json['speed_kmh'] as num?)?.toDouble() ?? 0,
      parkedSlotId: json['parked_slot_id']?.toString() ?? '',
    );
  }
}

class RoutePoint {
  const RoutePoint({
    required this.x,
    required this.y,
    required this.z,
    required this.floor,
  });

  final double x;
  final double y;
  final double z;
  final int floor;

  factory RoutePoint.fromJson(Map<String, dynamic> json) {
    return RoutePoint(
      x: (json['x'] as num?)?.toDouble() ?? 0,
      y: (json['y'] as num?)?.toDouble() ?? 0,
      z: (json['z'] as num?)?.toDouble() ?? 0,
      floor: (json['floor'] as num?)?.toInt() ?? 0,
    );
  }
}

class NavigationState {
  const NavigationState({
    required this.active,
    required this.type,
    required this.destination,
    required this.destinationFloor,
    required this.distanceRemaining,
    required this.instruction,
    required this.route,
  });

  final bool active;
  final String type;
  final String destination;
  final int destinationFloor;
  final double distanceRemaining;
  final String instruction;
  final List<RoutePoint> route;

  factory NavigationState.fromJson(Map<String, dynamic> json) {
    final rawRoute = json['route'] as List<dynamic>? ?? const [];
    return NavigationState(
      active: json['active'] == true,
      type: json['type']?.toString() ?? 'NONE',
      destination: json['destination']?.toString() ?? '',
      destinationFloor:
          (json['destination_floor'] as num?)?.toInt() ?? 0,
      distanceRemaining:
          (json['distance_remaining'] as num?)?.toDouble() ?? 0,
      instruction: json['instruction']?.toString() ?? 'Sin ruta activa',
      route: rawRoute
          .whereType<Map<String, dynamic>>()
          .map(RoutePoint.fromJson)
          .toList(growable: false),
    );
  }
}

class ParkingState {
  const ParkingState({
    required this.version,
    required this.totalSlots,
    required this.available,
    required this.occupied,
    required this.reserved,
    required this.disabled,
    required this.guidanceActive,
    required this.guidanceType,
    required this.routeTargetId,
    required this.reservedSlotId,
    required this.selectedSlotId,
    required this.lastCommandId,
    required this.vehicle,
    required this.navigation,
    required this.floors,
    required this.slots,
    required this.waitingForOpenGL,
  });

  final int version;
  final int totalSlots;
  final int available;
  final int occupied;
  final int reserved;
  final int disabled;
  final bool guidanceActive;
  final String guidanceType;
  final String routeTargetId;
  final String reservedSlotId;
  final String selectedSlotId;
  final String lastCommandId;
  final VehicleState vehicle;
  final NavigationState navigation;
  final List<ParkingFloor> floors;
  final List<ParkingSlot> slots;
  final bool waitingForOpenGL;

  factory ParkingState.fromJson(Map<String, dynamic> json) {
    int readInt(String key) => (json[key] as num?)?.toInt() ?? 0;
    final floorJson = json['floors'] as List<dynamic>? ?? const [];
    final slotJson = json['slots'] as List<dynamic>? ?? const [];
    final vehicleJson = json['vehicle'] as Map<String, dynamic>? ?? const {};
    final navigationJson =
        json['navigation'] as Map<String, dynamic>? ?? const {};

    return ParkingState(
      version: readInt('version'),
      totalSlots: readInt('total_slots'),
      available: readInt('available'),
      occupied: readInt('occupied'),
      reserved: readInt('reserved'),
      disabled: readInt('disabled'),
      guidanceActive: json['guidance_active'] == true,
      guidanceType: json['guidance_type']?.toString() ?? 'NONE',
      routeTargetId: json['route_target_id']?.toString() ?? '',
      reservedSlotId: json['reserved_slot_id']?.toString() ?? '',
      selectedSlotId: json['selected_slot_id']?.toString() ?? '',
      lastCommandId: json['last_command_id']?.toString() ?? '',
      vehicle: VehicleState.fromJson(vehicleJson),
      navigation: NavigationState.fromJson(navigationJson),
      floors: floorJson
          .whereType<Map<String, dynamic>>()
          .map(ParkingFloor.fromJson)
          .toList(growable: false),
      slots: slotJson
          .whereType<Map<String, dynamic>>()
          .map(ParkingSlot.fromJson)
          .toList(growable: false),
      waitingForOpenGL: json['backend_waiting_for_opengl'] == true,
    );
  }

  List<ParkingSlot> slotsForFloor(int floorIndex) {
    final result = slots
        .where((slot) => slot.floorIndex == floorIndex)
        .toList(growable: false);
    result.sort((a, b) {
      final rowComparison = a.row.compareTo(b.row);
      return rowComparison != 0
          ? rowComparison
          : a.number.compareTo(b.number);
    });
    return result;
  }
}
