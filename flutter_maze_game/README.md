# Flutter Maze Game

This directory contains a standalone Flutter app for a tilt-controlled maze game.

## Features

- Moves a simulated marble with the device accelerometer
- Uses simple collision physics against maze walls
- Detects when the marble reaches the goal
- Supports restarting the run after a win

## Run

1. Install Flutter on a machine with Android or iOS tooling.
2. From `/home/runner/work/surfacestreams/surfacestreams/flutter_maze_game`, run `flutter pub get`.
3. If you need native runner files, generate them with `flutter create --platforms=android,ios .`.
4. Launch on a physical device with `flutter run`.

## Notes

- The sandbox used for this task does not include the Flutter SDK, so native runner files could not be generated here.
- For iOS, add `NSMotionUsageDescription` to the generated `ios/Runner/Info.plist` before running on device.

