import 'dart:math' as math;

import 'package:flutter/material.dart';

import '../models/parking_state.dart';

class ParkingMap extends StatelessWidget {
  const ParkingMap({
    super.key,
    required this.state,
    required this.floorIndex,
  });

  final ParkingState state;
  final int floorIndex;

  @override
  Widget build(BuildContext context) {
    return AspectRatio(
      aspectRatio: 1.55,
      child: ClipRRect(
        borderRadius: BorderRadius.circular(22),
        child: DecoratedBox(
          decoration: const BoxDecoration(color: Color(0xFF07101C)),
          child: CustomPaint(
            painter: ParkingMapPainter(
              state: state,
              floorIndex: floorIndex,
            ),
            child: const SizedBox.expand(),
          ),
        ),
      ),
    );
  }
}

class ParkingMapPainter extends CustomPainter {
  ParkingMapPainter({required this.state, required this.floorIndex});

  final ParkingState state;
  final int floorIndex;

  static const double minX = -49;
  static const double maxX = 49;
  static const double minZ = -38;
  static const double maxZ = 58;

  Offset _worldToScreen(double x, double z, Size size) {
    final dx = ((x - minX) / (maxX - minX)) * size.width;
    final dy = size.height - ((z - minZ) / (maxZ - minZ)) * size.height;
    return Offset(dx, dy);
  }

  Color _slotColor(SlotStatus status) {
    return switch (status) {
      SlotStatus.available => const Color(0xFF22E66B),
      SlotStatus.occupied => const Color(0xFFFF3B30),
      SlotStatus.reserved => const Color(0xFFFFC928),
      SlotStatus.disabled => const Color(0xFF2F8CFF),
      SlotStatus.unknown => Colors.grey,
    };
  }

  @override
  void paint(Canvas canvas, Size size) {
    final background = Paint()..color = const Color(0xFF07101C);
    canvas.drawRect(Offset.zero & size, background);

    final buildingTopLeft = _worldToScreen(minX + 3, 35, size);
    final buildingBottomRight = _worldToScreen(maxX - 3, -35, size);
    final buildingRect = Rect.fromPoints(buildingTopLeft, buildingBottomRight);
    canvas.drawRRect(
      RRect.fromRectAndRadius(buildingRect, const Radius.circular(12)),
      Paint()
        ..color = const Color(0xFF111F30)
        ..style = PaintingStyle.fill,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(buildingRect, const Radius.circular(12)),
      Paint()
        ..color = const Color(0xFF35506E)
        ..strokeWidth = 2
        ..style = PaintingStyle.stroke,
    );

    if (floorIndex == 0) {
      final entranceLeft = _worldToScreen(1.55, 35, size);
      final entranceRight = _worldToScreen(23.35, 35, size);
      final streetEnd = _worldToScreen(23.35, 57, size);
      canvas.drawRect(
        Rect.fromLTRB(
          entranceLeft.dx,
          streetEnd.dy,
          entranceRight.dx,
          entranceLeft.dy,
        ),
        Paint()..color = const Color(0xFF24364A),
      );
    }

    final aislePaint = Paint()
      ..color = const Color(0xFF29425D)
      ..strokeWidth = 12
      ..strokeCap = StrokeCap.round;
    canvas.drawLine(
      _worldToScreen(-42, 20, size),
      _worldToScreen(42, 20, size),
      aislePaint,
    );
    canvas.drawLine(
      _worldToScreen(-42, -20, size),
      _worldToScreen(42, -20, size),
      aislePaint,
    );
    canvas.drawLine(
      _worldToScreen(-10.68, -20, size),
      _worldToScreen(-10.68, 20, size),
      aislePaint,
    );
    canvas.drawLine(
      _worldToScreen(12.45, -20, size),
      _worldToScreen(12.45, 20, size),
      aislePaint,
    );

    final slots = state.slotsForFloor(floorIndex);
    for (final slot in slots) {
      final center = _worldToScreen(slot.x, slot.z, size);
      final color = _slotColor(slot.status);
      final rect = Rect.fromCenter(
        center: center,
        width: math.max(3.0, size.width * 0.009),
        height: math.max(7.0, size.height * 0.025),
      );
      canvas.drawRRect(
        RRect.fromRectAndRadius(rect, const Radius.circular(2)),
        Paint()..color = color.withValues(alpha: 0.92),
      );
      if (slot.id == state.reservedSlotId ||
          slot.id == state.vehicle.parkedSlotId) {
        canvas.drawCircle(
          center,
          math.max(7.0, size.width * 0.018),
          Paint()
            ..color = color
            ..strokeWidth = 2.5
            ..style = PaintingStyle.stroke,
        );
      }
    }

    final routePoints = state.navigation.route
        .where((point) => point.floor == floorIndex)
        .toList(growable: false);
    if (routePoints.length >= 2) {
      final routePath = Path();
      for (var i = 0; i < routePoints.length; i++) {
        final point = _worldToScreen(routePoints[i].x, routePoints[i].z, size);
        if (i == 0) {
          routePath.moveTo(point.dx, point.dy);
        } else {
          routePath.lineTo(point.dx, point.dy);
        }
      }
      canvas.drawPath(
        routePath,
        Paint()
          ..color = state.navigation.type == 'EXIT'
              ? const Color(0xFF2DF39B)
              : const Color(0xFF23C9FF)
          ..strokeWidth = 5
          ..strokeCap = StrokeCap.round
          ..strokeJoin = StrokeJoin.round
          ..style = PaintingStyle.stroke,
      );
    }

    final vehicleVisible = state.vehicle.floor == floorIndex ||
        (state.vehicle.floor == -1 && floorIndex == 0);
    if (vehicleVisible) {
      final vehiclePosition =
          _worldToScreen(state.vehicle.x, state.vehicle.z, size);
      canvas.save();
      canvas.translate(vehiclePosition.dx, vehiclePosition.dy);
      canvas.rotate(state.vehicle.headingDegrees * math.pi / 180);
      final carPath = Path()
        ..moveTo(0, -13)
        ..lineTo(8, 9)
        ..lineTo(0, 5)
        ..lineTo(-8, 9)
        ..close();
      canvas.drawShadow(carPath, Colors.black, 4, true);
      canvas.drawPath(carPath, Paint()..color = const Color(0xFF64E6FF));
      canvas.drawPath(
        carPath,
        Paint()
          ..color = Colors.white
          ..strokeWidth = 1.5
          ..style = PaintingStyle.stroke,
      );
      canvas.restore();
    }

    final textPainter = TextPainter(
      text: TextSpan(
        text: floorIndex == 0 ? 'PB + acceso desde la calle' : 'Piso $floorIndex',
        style: const TextStyle(
          color: Color(0xFFBFD8F2),
          fontSize: 12,
          fontWeight: FontWeight.w600,
        ),
      ),
      textDirection: TextDirection.ltr,
    )..layout();
    textPainter.paint(canvas, const Offset(12, 10));
  }

  @override
  bool shouldRepaint(covariant ParkingMapPainter oldDelegate) {
    return oldDelegate.state != state || oldDelegate.floorIndex != floorIndex;
  }
}
