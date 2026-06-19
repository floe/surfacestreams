import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:sensors_plus/sensors_plus.dart';

import 'package:flutter_maze_game/main.dart';

void main() {
  testWidgets('renders maze game shell', (WidgetTester tester) async {
    await tester.pumpWidget(
      const MazeApp(sensorEvents: Stream<AccelerometerEvent>.empty()),
    );

    expect(find.text('Tilt Maze'), findsOneWidget);
    expect(find.textContaining('Tilt your phone'), findsOneWidget);
    expect(find.byType(CustomPaint), findsOneWidget);
    expect(find.widgetWithText(OutlinedButton, 'Restart'), findsOneWidget);
  });
}

