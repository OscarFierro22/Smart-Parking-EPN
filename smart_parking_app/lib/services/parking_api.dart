import 'dart:async';
import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:web_socket_channel/web_socket_channel.dart';

import '../models/parking_state.dart';

class ParkingApiException implements Exception {
  const ParkingApiException(this.message);

  final String message;

  @override
  String toString() => message;
}

class ParkingApi {
  ParkingApi({required String serverAddress})
      : _serverAddress = _normalize(serverAddress);

  final http.Client _client = http.Client();
  final StreamController<ParkingState> _stateController =
      StreamController<ParkingState>.broadcast();
  final StreamController<bool> _connectionController =
      StreamController<bool>.broadcast();

  String _serverAddress;
  WebSocketChannel? _channel;
  StreamSubscription<dynamic>? _socketSubscription;
  Timer? _reconnectTimer;
  bool _disposed = false;
  bool _connected = false;

  Stream<ParkingState> get states => _stateController.stream;
  Stream<bool> get connections => _connectionController.stream;
  String get serverAddress => _serverAddress;

  static String _normalize(String value) {
    var normalized = value.trim();
    normalized = normalized.replaceFirst(RegExp(r'^https?://'), '');
    normalized = normalized.replaceFirst(RegExp(r'^wss?://'), '');
    while (normalized.endsWith('/')) {
      normalized = normalized.substring(0, normalized.length - 1);
    }
    return normalized;
  }

  Uri _httpUri(String path) => Uri.parse('http://$_serverAddress$path');
  Uri _wsUri() => Uri.parse('ws://$_serverAddress/ws');

  Future<void> changeServer(String serverAddress) async {
    _serverAddress = _normalize(serverAddress);
    await connect();
  }

  Future<void> connect() async {
    _reconnectTimer?.cancel();
    await _closeSocket();

    try {
      final initialState = await fetchState();
      if (!_stateController.isClosed) {
        _stateController.add(initialState);
      }

      final channel = WebSocketChannel.connect(_wsUri());
      await channel.ready.timeout(const Duration(seconds: 5));
      _channel = channel;
      _setConnected(true);

      _socketSubscription = channel.stream.listen(
        _handleSocketMessage,
        onError: (_) {
          _setConnected(false);
          _scheduleReconnect();
        },
        onDone: () {
          _setConnected(false);
          _scheduleReconnect();
        },
        cancelOnError: true,
      );
    } catch (_) {
      _setConnected(false);
      _scheduleReconnect();
      rethrow;
    }
  }

  void _handleSocketMessage(dynamic message) {
    if (message is! String) {
      return;
    }

    final decoded = jsonDecode(message);
    if (decoded is! Map<String, dynamic>) {
      return;
    }
    if (decoded['type'] == 'pong') {
      return;
    }
    if (decoded['type'] == 'backend_error') {
      _setConnected(false);
      return;
    }

    _stateController.add(ParkingState.fromJson(decoded));
  }

  void _scheduleReconnect() {
    if (_disposed || _reconnectTimer?.isActive == true) {
      return;
    }

    _reconnectTimer = Timer(const Duration(seconds: 3), () async {
      try {
        await connect();
      } catch (_) {
        _scheduleReconnect();
      }
    });
  }

  void _setConnected(bool value) {
    if (_connected == value) {
      return;
    }
    _connected = value;
    if (!_connectionController.isClosed) {
      _connectionController.add(value);
    }
  }

  Future<ParkingState> fetchState() async {
    final response = await _client
        .get(_httpUri('/api/state'))
        .timeout(const Duration(seconds: 5));
    _ensureSuccess(response);
    return ParkingState.fromJson(
      jsonDecode(utf8.decode(response.bodyBytes)) as Map<String, dynamic>,
    );
  }

  Future<void> reserve(String slotId) =>
      _post('/api/reserve/${Uri.encodeComponent(slotId)}');

  Future<void> reserveBest() => _post('/api/reserve-best');

  Future<void> cancelReservation() => _post('/api/cancel-reservation');

  Future<void> simulateRandom() => _post('/api/simulate-random');

  Future<void> routeToExit() => _post('/api/route-exit');

  Future<void> release(String slotId) =>
      _post('/api/release/${Uri.encodeComponent(slotId)}');

  Future<void> occupy(String slotId) =>
      _post('/api/occupy/${Uri.encodeComponent(slotId)}');

  Future<void> _post(String path) async {
    final response = await _client
        .post(_httpUri(path))
        .timeout(const Duration(seconds: 5));
    _ensureSuccess(response);
  }

  void _ensureSuccess(http.Response response) {
    if (response.statusCode >= 200 && response.statusCode < 300) {
      return;
    }

    var message = 'Error ${response.statusCode}';
    try {
      final body = jsonDecode(utf8.decode(response.bodyBytes));
      if (body is Map<String, dynamic> && body['detail'] != null) {
        message = body['detail'].toString();
      }
    } catch (_) {
      if (response.body.isNotEmpty) {
        message = response.body;
      }
    }
    throw ParkingApiException(message);
  }

  Future<void> _closeSocket() async {
    await _socketSubscription?.cancel();
    _socketSubscription = null;
    final channel = _channel;
    _channel = null;
    if (channel != null) {
      await channel.sink.close();
    }
  }

  Future<void> dispose() async {
    _disposed = true;
    _reconnectTimer?.cancel();
    await _closeSocket();
    _client.close();
    await _stateController.close();
    await _connectionController.close();
  }
}
