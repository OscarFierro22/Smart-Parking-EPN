import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';

class ServerDiscovery {
  ServerDiscovery._();

  static const int discoveryPort = 40404;
  static const String discoveryRequest = 'SMART_PARKING_DISCOVER_V1';
  static const String _savedServerKey = 'smart_parking_server_address';

  static final SharedPreferencesAsync _preferences = SharedPreferencesAsync();

  static Future<String?> loadSavedServer() {
    return _preferences.getString(_savedServerKey);
  }

  static Future<void> saveServer(String address) async {
    await _preferences.setString(_savedServerKey, address.trim());
  }

  static Future<bool> isReachable(
    String address, {
    Duration timeout = const Duration(seconds: 2),
  }) async {
    final normalized = _normalize(address);
    if (normalized.isEmpty) return false;

    final client = http.Client();
    try {
      final response = await client
          .get(Uri.parse('http://$normalized/api/health'))
          .timeout(timeout);
      if (response.statusCode != 200) return false;

      final decoded = jsonDecode(utf8.decode(response.bodyBytes));
      return decoded is Map<String, dynamic> && decoded['ok'] == true;
    } catch (_) {
      return false;
    } finally {
      client.close();
    }
  }

  static Future<String?> discover({
    Duration timeout = const Duration(seconds: 4),
  }) async {
    RawDatagramSocket? socket;
    StreamSubscription<RawSocketEvent>? subscription;
    Timer? repeatTimer;
    final completer = Completer<String?>();

    try {
      final boundSocket = await RawDatagramSocket.bind(
        InternetAddress.anyIPv4,
        0,
        reuseAddress: true,
      );
      socket = boundSocket;
      boundSocket.broadcastEnabled = true;

      subscription = boundSocket.listen(
        (event) {
          if (event != RawSocketEvent.read || completer.isCompleted) return;
          final datagram = boundSocket.receive();
          if (datagram == null) return;

          try {
            final decoded = jsonDecode(utf8.decode(datagram.data));
            if (decoded is! Map<String, dynamic>) return;
            if (decoded['service'] != 'smart-parking') return;

            final port = (decoded['port'] as num?)?.toInt() ?? 8000;
            final server = '${datagram.address.address}:$port';
            completer.complete(server);
          } catch (_) {
            // Ignora paquetes UDP que no pertenezcan a Smart Parking.
          }
        },
        onError: (_) {
          if (!completer.isCompleted) completer.complete(null);
        },
      );

      Future<void> sendDiscovery() async {
        final bytes = utf8.encode(discoveryRequest);
        final destinations = <String>{'255.255.255.255'};

        try {
          final interfaces = await NetworkInterface.list(
            type: InternetAddressType.IPv4,
            includeLoopback: false,
          );
          for (final interface in interfaces) {
            for (final address in interface.addresses) {
              final parts = address.address.split('.');
              if (parts.length == 4) {
                destinations.add('${parts[0]}.${parts[1]}.${parts[2]}.255');
              }
            }
          }
        } catch (_) {
          // El broadcast general sigue disponible.
        }

        for (final destination in destinations) {
          try {
            boundSocket.send(
              bytes,
              InternetAddress(destination),
              discoveryPort,
            );
          } catch (_) {
            // Algunas interfaces virtuales no aceptan broadcast.
          }
        }
      }

      await sendDiscovery();
      repeatTimer = Timer.periodic(const Duration(milliseconds: 900), (_) {
        unawaited(sendDiscovery());
      });

      return await completer.future.timeout(timeout, onTimeout: () => null);
    } catch (_) {
      return null;
    } finally {
      repeatTimer?.cancel();
      if (subscription != null) {
        await subscription.cancel();
      }
      socket?.close();
    }
  }

  static String _normalize(String value) {
    var normalized = value.trim();
    normalized = normalized.replaceFirst(RegExp(r'^https?://'), '');
    normalized = normalized.replaceFirst(RegExp(r'^wss?://'), '');
    while (normalized.endsWith('/')) {
      normalized = normalized.substring(0, normalized.length - 1);
    }
    return normalized;
  }
}
