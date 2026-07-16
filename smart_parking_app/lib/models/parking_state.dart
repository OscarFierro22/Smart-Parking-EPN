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
  });

  final String id;
  final String floor;
  final int floorIndex;
  final String row;
  final int number;
  final SlotStatus status;

  factory ParkingSlot.fromJson(Map<String, dynamic> json) {
    return ParkingSlot(
      id: json['id']?.toString() ?? '',
      floor: json['floor']?.toString() ?? '',
      floorIndex: (json['floor_index'] as num?)?.toInt() ?? 0,
      row: json['row']?.toString() ?? '',
      number: (json['number'] as num?)?.toInt() ?? 0,
      status: SlotStatus.fromApi(json['state']?.toString() ?? ''),
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
  final List<ParkingFloor> floors;
  final List<ParkingSlot> slots;
  final bool waitingForOpenGL;

  factory ParkingState.fromJson(Map<String, dynamic> json) {
    int readInt(String key) => (json[key] as num?)?.toInt() ?? 0;

    final floorJson = (json['floors'] as List<dynamic>? ?? const []);
    final slotJson = (json['slots'] as List<dynamic>? ?? const []);

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
