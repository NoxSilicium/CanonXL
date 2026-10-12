# Controller Development

Run commands from the repository root. Nothing here opens a serial port or
uploads firmware. The controller and its tests do not depend on the web analyser,
PHP, Node.js, historical captures or any external service.

## Host Tests

You need CMake 3.16+ and a C++11 compiler. GCC and Clang support the optional
AddressSanitizer/UndefinedBehaviorSanitizer configuration:

```sh
cmake -S . -B build/host -DXL_SANITIZERS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Four suites cover protocol/state transitions, bounded serial parsing/output,
RP2040 transport, sketch scheduling and the AVR transport. Models exercise
software behavior; they do not measure electrical timing or optical performance.

## Formatting and Documentation

Use Python 3.13 and the pinned development tools:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r requirements-style.txt
python tools/style.py check
python tools/style.py test
```

`format` applies formatting; `analyse` checks naming and lint. C++ uses
clang-format 18.1.8 followed by a syntax-tree whitespace pass that puts aggregate
braces on separate lines. Four dedicated tests cover literals, nesting and
idempotence. Python uses Black 25.1.0 and Ruff 0.11.13.

With Doxygen installed, `python tools/style.py docs` generates ignored
`doc/generated/`. Undocumented APIs and documentation warnings fail generation.
See [coding-style.md](coding-style.md) for all conventions.

## Arduino Builds

Validated toolchain: Arduino CLI 1.5.1, Arduino AVR Boards 1.8.8 and Arduino Mbed
OS Nano Boards 4.6.0. Install only the core required by your board:

```sh
arduino-cli core install arduino:avr@1.8.8
arduino-cli core install arduino:mbed_nano@4.6.0
```

Compile without uploading:

```sh
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old --build-path build/nano canon_xl_controller
arduino-cli compile --fqbn arduino:mbed_nano:nanorp2040connect --build-path build/rp2040 canon_xl_controller
```

In Arduino IDE, open `canon_xl_controller/canon_xl_controller.ino`. Select Nano
and ATmega328P (Old Bootloader) for the classic bench board, or Nano RP2040
Connect for the second target. No external Arduino library is required.

The pinned AVR core emits unused-parameter warnings in third-party `new.cpp`
with `--warnings all`; maintained host code uses warnings as errors.
Build success does not qualify a board/lens combination. Identify the board,
check electrical levels and preserve a verified firmware backup before uploading.

## Processing Panel

The [panel guide](../processing/README.md) covers Processing installation, pinned
G4P/jSerialComm dependencies and GUI launch. The panel is independent of the
controller's CMake build. Its macOS launcher uses Processing's bundled Java.

These checks do not open a physical serial port or upload firmware:

```sh
python3 processing/tools/control_panel.py test
python3 processing/tools/control_panel.py build
python3 processing/tools/control_panel.py demo --smoke 4 --screenshot /tmp/canon-xl-demo.png
```

The first command compiles and runs deterministic Java contract tests. The second
builds the complete sketch; the third checks rendering with synthetic telemetry.
Downloaded libraries and generated outputs are excluded from Git. Bench captures
remain separate and can be replayed explicitly with `test --capture PATH`.
