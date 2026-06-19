import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:sensors_plus/sensors_plus.dart';

void main() {
  runApp(MazeApp(sensorEvents: accelerometerEvents));
}

class MazeApp extends StatelessWidget {
  const MazeApp({
    super.key,
    this.sensorEvents,
  });

  final Stream<AccelerometerEvent>? sensorEvents;

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF2563EB)),
        useMaterial3: true,
      ),
      home: MazeGamePage(sensorEvents: sensorEvents),
    );
  }
}

class MazeGamePage extends StatefulWidget {
  const MazeGamePage({
    super.key,
    required this.sensorEvents,
  });

  final Stream<AccelerometerEvent>? sensorEvents;

  @override
  State<MazeGamePage> createState() => _MazeGamePageState();
}

class _MazeGamePageState extends State<MazeGamePage>
    with SingleTickerProviderStateMixin {
  static const double _marbleRadius = 0.035;
  static const double _goalRadius = 0.05;
  static const double _maxTilt = 1.0;
  static const double _tiltStrength = 2.7;
  static const double _damping = 0.985;
  static const Offset _startPosition = Offset(0.10, 0.10);
  static const Offset _goalPosition = Offset(0.89, 0.89);

  final List<Rect> _walls = const <Rect>[
    Rect.fromLTWH(0.18, 0.00, 0.05, 0.70),
    Rect.fromLTWH(0.00, 0.20, 0.62, 0.05),
    Rect.fromLTWH(0.38, 0.20, 0.05, 0.58),
    Rect.fromLTWH(0.58, 0.00, 0.05, 0.58),
    Rect.fromLTWH(0.58, 0.58, 0.24, 0.05),
    Rect.fromLTWH(0.00, 0.78, 0.62, 0.05),
    Rect.fromLTWH(0.78, 0.20, 0.05, 0.60),
  ];

  late final Ticker _ticker;
  StreamSubscription<AccelerometerEvent>? _sensorSubscription;
  Offset _position = _startPosition;
  Offset _velocity = Offset.zero;
  Offset _tilt = Offset.zero;
  Duration _elapsed = Duration.zero;
  Duration? _lastTick;
  String? _sensorMessage;
  bool _won = false;

  @override
  void initState() {
    super.initState();
    _ticker = createTicker(_handleTick)..start();
    _subscribeToSensors();
  }

  void _subscribeToSensors() {
    final Stream<AccelerometerEvent>? sensorEvents = widget.sensorEvents;
    if (sensorEvents == null) {
      _sensorMessage = 'Accelerometer unavailable in this environment.';
      return;
    }

    _sensorSubscription = sensorEvents.listen(
      (AccelerometerEvent event) {
        final Offset nextTilt = Offset(
          (event.x / 7.0).clamp(-_maxTilt, _maxTilt).toDouble(),
          (event.y / 7.0).clamp(-_maxTilt, _maxTilt).toDouble(),
        );
        _tilt = Offset(
          (_tilt.dx * 0.85) + (nextTilt.dx * 0.15),
          (_tilt.dy * 0.85) + (nextTilt.dy * 0.15),
        );
        if (_sensorMessage != null && mounted) {
          setState(() {
            _sensorMessage = null;
          });
        }
      },
      onError: (Object error) {
        if (!mounted) {
          return;
        }
        setState(() {
          _sensorMessage = 'Sensor error: $error';
        });
      },
    );
  }

  void _handleTick(Duration elapsed) {
    if (_won) {
      return;
    }

    final Duration previousTick = _lastTick ?? elapsed;
    _lastTick = elapsed;
    double deltaTime =
        (elapsed - previousTick).inMicroseconds / Duration.microsecondsPerSecond;
    if (deltaTime <= 0) {
      deltaTime = 1 / 60;
    }
    deltaTime = deltaTime.clamp(0.0, 1 / 20).toDouble();

    final Offset acceleration = Offset(
      -_tilt.dx * _tiltStrength,
      _tilt.dy * _tiltStrength,
    );

    Offset nextVelocity = Offset(
      (_velocity.dx + (acceleration.dx * deltaTime)) *
          math.pow(_damping, deltaTime * 60).toDouble(),
      (_velocity.dy + (acceleration.dy * deltaTime)) *
          math.pow(_damping, deltaTime * 60).toDouble(),
    );

    Offset nextPosition = _position;
    nextPosition = _moveAlongAxis(
      nextPosition,
      Offset(nextVelocity.dx * deltaTime, 0),
      (double adjustedVelocity) {
        nextVelocity = Offset(adjustedVelocity, nextVelocity.dy);
      },
    );
    nextPosition = _moveAlongAxis(
      nextPosition,
      Offset(0, nextVelocity.dy * deltaTime),
      (double adjustedVelocity) {
        nextVelocity = Offset(nextVelocity.dx, adjustedVelocity);
      },
    );

    final Duration nextElapsed = _elapsed + Duration(
      microseconds: (deltaTime * Duration.microsecondsPerSecond).round(),
    );
    final bool reachedGoal =
        (nextPosition - _goalPosition).distance <= (_goalRadius - 0.01);

    if (!mounted) {
      return;
    }

    setState(() {
      _position = nextPosition;
      _velocity = nextVelocity;
      _elapsed = nextElapsed;
      _won = reachedGoal;
    });
  }

  Offset _moveAlongAxis(
    Offset position,
    Offset delta,
    ValueChanged<double> updateVelocity,
  ) {
    Offset nextPosition = position + delta;

    if (nextPosition.dx - _marbleRadius < 0) {
      nextPosition = Offset(_marbleRadius, nextPosition.dy);
      updateVelocity(0);
    } else if (nextPosition.dx + _marbleRadius > 1) {
      nextPosition = Offset(1 - _marbleRadius, nextPosition.dy);
      updateVelocity(0);
    }

    if (nextPosition.dy - _marbleRadius < 0) {
      nextPosition = Offset(nextPosition.dx, _marbleRadius);
      updateVelocity(0);
    } else if (nextPosition.dy + _marbleRadius > 1) {
      nextPosition = Offset(nextPosition.dx, 1 - _marbleRadius);
      updateVelocity(0);
    }

    for (final Rect wall in _walls) {
      if (!_intersectsWall(nextPosition, wall)) {
        continue;
      }

      if (delta.dx > 0) {
        nextPosition = Offset(wall.left - _marbleRadius, nextPosition.dy);
      } else if (delta.dx < 0) {
        nextPosition = Offset(wall.right + _marbleRadius, nextPosition.dy);
      }

      if (delta.dy > 0) {
        nextPosition = Offset(nextPosition.dx, wall.top - _marbleRadius);
      } else if (delta.dy < 0) {
        nextPosition = Offset(nextPosition.dx, wall.bottom + _marbleRadius);
      }

      updateVelocity(0);
    }

    return nextPosition;
  }

  bool _intersectsWall(Offset center, Rect wall) {
    final double closestX = center.dx.clamp(wall.left, wall.right).toDouble();
    final double closestY = center.dy.clamp(wall.top, wall.bottom).toDouble();
    final double dx = center.dx - closestX;
    final double dy = center.dy - closestY;
    return (dx * dx) + (dy * dy) < (_marbleRadius * _marbleRadius);
  }

  void _resetGame() {
    setState(() {
      _position = _startPosition;
      _velocity = Offset.zero;
      _tilt = Offset.zero;
      _elapsed = Duration.zero;
      _lastTick = null;
      _won = false;
    });
  }

  @override
  void dispose() {
    _ticker.dispose();
    _sensorSubscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final String elapsedLabel = _formatDuration(_elapsed);
    final ColorScheme colors = Theme.of(context).colorScheme;

    return Scaffold(
      backgroundColor: colors.surface,
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: <Widget>[
              Text(
                'Tilt Maze',
                style: Theme.of(context).textTheme.headlineMedium,
              ),
              const SizedBox(height: 8),
              Text(
                _won
                    ? 'Goal reached in $elapsedLabel.'
                    : 'Tilt your phone to roll the marble into the green goal.',
                style: Theme.of(context).textTheme.bodyLarge,
              ),
              if (_sensorMessage != null) ...<Widget>[
                const SizedBox(height: 8),
                Text(
                  _sensorMessage!,
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                        color: colors.error,
                      ),
                ),
              ],
              const SizedBox(height: 20),
              Expanded(
                child: Center(
                  child: AspectRatio(
                    aspectRatio: 1,
                    child: DecoratedBox(
                      decoration: BoxDecoration(
                        color: colors.surfaceContainerLowest,
                        borderRadius: BorderRadius.circular(28),
                        boxShadow: const <BoxShadow>[
                          BoxShadow(
                            blurRadius: 24,
                            offset: Offset(0, 12),
                            color: Color(0x22000000),
                          ),
                        ],
                      ),
                      child: Padding(
                        padding: const EdgeInsets.all(12),
                        child: Stack(
                          fit: StackFit.expand,
                          children: <Widget>[
                            CustomPaint(
                              painter: MazePainter(
                                walls: _walls,
                                marblePosition: _position,
                                marbleRadius: _marbleRadius,
                                goalPosition: _goalPosition,
                                goalRadius: _goalRadius,
                              ),
                            ),
                            if (_won)
                              Center(
                                child: Card(
                                  child: Padding(
                                    padding: const EdgeInsets.all(20),
                                    child: Column(
                                      mainAxisSize: MainAxisSize.min,
                                      children: <Widget>[
                                        const Text('You win!'),
                                        const SizedBox(height: 12),
                                        FilledButton(
                                          onPressed: _resetGame,
                                          child: const Text('Play again'),
                                        ),
                                      ],
                                    ),
                                  ),
                                ),
                              ),
                          ],
                        ),
                      ),
                    ),
                  ),
                ),
              ),
              const SizedBox(height: 16),
              Row(
                children: <Widget>[
                  Expanded(
                    child: Text(
                      'Time: $elapsedLabel',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                  ),
                  OutlinedButton(
                    onPressed: _resetGame,
                    child: const Text('Restart'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  String _formatDuration(Duration duration) {
    final int minutes = duration.inMinutes;
    final int seconds = duration.inSeconds % 60;
    final int hundredths = (duration.inMilliseconds % 1000) ~/ 10;
    return '$minutes:${seconds.toString().padLeft(2, '0')}.${hundredths.toString().padLeft(2, '0')}';
  }
}

class MazePainter extends CustomPainter {
  const MazePainter({
    required this.walls,
    required this.marblePosition,
    required this.marbleRadius,
    required this.goalPosition,
    required this.goalRadius,
  });

  final List<Rect> walls;
  final Offset marblePosition;
  final double marbleRadius;
  final Offset goalPosition;
  final double goalRadius;

  @override
  void paint(Canvas canvas, Size size) {
    final Paint boardPaint = Paint()..color = const Color(0xFFF8FAFC);
    final Paint wallPaint = Paint()..color = const Color(0xFF0F172A);
    final Paint startPaint = Paint()..color = const Color(0xFF94A3B8);
    final Paint goalPaint = Paint()..color = const Color(0xFF22C55E);
    final Paint marblePaint = Paint()
      ..shader = const LinearGradient(
        colors: <Color>[Color(0xFF60A5FA), Color(0xFF1D4ED8)],
      ).createShader(Offset.zero & size);

    canvas.drawRRect(
      RRect.fromRectAndRadius(Offset.zero & size, const Radius.circular(20)),
      boardPaint,
    );

    for (final Rect wall in walls) {
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromLTWH(
            wall.left * size.width,
            wall.top * size.height,
            wall.width * size.width,
            wall.height * size.height,
          ),
          const Radius.circular(12),
        ),
        wallPaint,
      );
    }

    canvas.drawCircle(
      Offset(size.width * 0.10, size.height * 0.10),
      size.width * 0.03,
      startPaint,
    );

    canvas.drawCircle(
      Offset(goalPosition.dx * size.width, goalPosition.dy * size.height),
      goalRadius * size.width,
      goalPaint,
    );

    canvas.drawCircle(
      Offset(marblePosition.dx * size.width, marblePosition.dy * size.height),
      marbleRadius * size.width,
      marblePaint,
    );
  }

  @override
  bool shouldRepaint(covariant MazePainter oldDelegate) {
    return oldDelegate.marblePosition != marblePosition;
  }
}
