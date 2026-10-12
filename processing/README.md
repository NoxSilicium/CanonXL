# Canon XL Control Panel

The [Processing](https://processing.org/) application controls the standalone
Canon XL lens through the Arduino USB serial protocol. It retains the original
G4P focus, zoom, iris and X/Y layout, with telemetry display and recording.\
![Control panel connected to the Nano and XL 20x](../doc/images/canon-xl-processing-control-panel.png)\
The pictured panel shows live telemetry in read-only mode. Electronic remote
focus is not implemented; its original control remains disabled.

## Setup

Install [Processing 4](https://processing.org/download). The launcher requires
Python 3.10 or newer and uses Processing's bundled Java on macOS. Application
export is validated on Apple Silicon; other platforms are not yet validated.

From the repository root, download the pinned libraries:

```sh
mkdir -p processing/vendor processing/canon_xl_control/code
curl -fL 'https://downloads.sourceforge.net/project/g4p/G4P%20V4.3.11.zip' -o processing/vendor/G4P-4.3.11.zip
curl -fL 'https://repo.maven.apache.org/maven2/com/fazecast/jSerialComm/2.11.4/jSerialComm-2.11.4.jar' -o processing/canon_xl_control/code/jSerialComm-2.11.4.jar
```

Build the sketch:

```sh
python3 processing/tools/control_panel.py build
```

The launcher extracts `G4P.jar` into the sketch's `code/` folder and verifies
both jar hashes before building. Libraries remain local, outside Git. No global
sketchbook library or Processing preference is changed. Set `processingPath` to
the Processing launcher if the macOS application is installed elsewhere.

You can then open [canon_xl_control.pde](canon_xl_control/canon_xl_control.pde)
directly in Processing. For manual IDE setup, extract `G4P/library/G4P.jar` from
the downloaded archive into `canon_xl_control/code/`, alongside the jSerialComm
jar. Keep the matching sketch folder and entry filename.

## Run

```sh
python3 processing/tools/control_panel.py run
```

Select the controller's serial port explicitly, then connect. Opening a port
can reset the Nano and close the iris. The panel waits for startup to settle
and fresh telemetry to arrive before allowing control. A normal launch starts
disconnected; connection alone does not send actuator commands.

Activate commands explicitly. Iris editing stages a raw integer request; Apply
sends it. Zoom runs while a direction button is held. X/Y has its own enable
checkbox; the center button zeros both offsets without resetting the Nano.
Stop disables commands and zeros motion while retaining the current iris request.

Record saves received lines, transmitted commands and host monotonic timestamps
in a new file under `build/logs/`. Existing captures are never overwritten.
The language dropdown changes labels without reconnecting.

```sh
python3 processing/tools/control_panel.py demo
python3 processing/tools/control_panel.py run --language en
python3 processing/tools/control_panel.py app
```

Demo uses synthetic telemetry and never opens a serial port. The `app` command
prints the path to an Apple Silicon `.app` with Java included, generated under
`build/`. It starts disconnected and does not install anything globally.

## Controls and Feedback

| Area | Behavior |
| --- | --- |
| Iris | Raw request 0..255; FF closes. Request and MISO2 feedback are separate. |
| Zoom | Signed amplitude 1..15; hold/release with periodic command renewal. |
| X/Y | Raw offsets -5000..5000; original inverted screen-Y orientation. |
| Focus | Original slider retained, disabled: no electronic focus command implemented. |
| MISO 40 | Latest sampled response, checksum checked locally. |
| Connection | Explicit port selection, stale-state detection and disconnect cleanup. |

No F-number, focal length, focus distance, angular X/Y scale or absolute optical
position is inferred. MISO4 is displayed as a raw byte, not an exclusive AF/M
flag. ACK means controller acceptance, not physical lens movement. XLRAW sampling
is not an exhaustive recording of every SPI exchange. See the
[serial protocol](../doc/serial-protocol.md) for firmware commands and fields.

Stale telemetry, loss of window focus, a controller restart, a command error or
an excessive host scheduling gap disables control and stops owned motion. Motion
does not resume automatically after recovery. Closing a read-only session sends
no STOP; closing an owned session stops its motion. A broken physical link can
prevent delivery, so the firmware's motion lease remains the final fallback.
STOP does not close the iris.

## Verify

```sh
python3 processing/tools/control_panel.py test
python3 processing/tools/control_panel.py build
python3 processing/tools/control_panel.py demo --smoke 4 --screenshot /tmp/canon-xl-demo.png
```

Pure Java tests do not open serial hardware. They cover framing, response
validation, command bounds, explicit control ownership and motion interruption.
The finite demo saves the fully rendered final frame, including G4P widgets.

An optional recorded bench session can be checked with `test --capture PATH`.
The replay expects the complete iris/zoom/X/Y sequence and matching XL/XLRAW
samples; raw bench captures are not part of the repository.

**The following command actuates the lens.** Replace `SERIAL_PORT` with the
identified controller port and choose a new recording path:

```sh
python3 processing/tools/control_panel.py run --port SERIAL_PORT --raw --bench-test --smoke 15 --record /tmp/canon-xl-new-bench.txt
```

This opt-in sequence requests iris `0x70` (112), short zoom +2/-2 pulses,
X+1000/Y+1000, then iris `0xFF` (255) with zero motion. It does not require window
focus, but fresh-state and host-lease guards remain active. Do not select an
arbitrary attached Arduino or the camcorder observation setup. The launcher never uploads firmware.

## Sources

- [canon_xl_control.pde](canon_xl_control/canon_xl_control.pde): transport, polling and animation loop.
- [Gui.pde](canon_xl_control/Gui.pde): G4P widgets, localization and user actions.
- [ControllerProtocol.java](canon_xl_control/ControllerProtocol.java): framing, response validation and command ranges.
- [ControllerSession.java](canon_xl_control/ControllerSession.java): host lease, fresh state and control ownership.
- [Bench.pde](canon_xl_control/Bench.pde): opt-in finite bench sequence.
- [tests/](tests/): deterministic contract tests and optional capture replay.
- [control_panel.py](tools/control_panel.py): build, test, launch and macOS application export.

Java/PDE follow the shared English, four-space, Allman conventions. Classes use
PascalCase; methods, fields and constants use camelCase. Processing callback
signatures and its matching sketch folder/entry name are external contracts.
The source follows the [project coding convention](../doc/coding-style.md).

## Dependencies and License

Pinned libraries and their upstream notices are listed in
[THIRD-PARTY.md](THIRD-PARTY.md). The launcher verifies these SHA-256 values:

| File | SHA-256 |
| --- | --- |
| `G4P.jar` | `33e5087ab957bd75277ff54d027f305fcb6938ef05fe3ff4f1e871bfe6c6d4cd` |
| `jSerialComm-2.11.4.jar` | `b5f7ec40d622864b92dc679159cdbaa3030d37884f0af2deef45b56c16bda7a6` |

The G4P source archive's SHA-256 is
`30a92d03fc93f7ada3e4c8105a380dd82f46390f4523116d6d7a8552be7bac2a`.

Owned sources are covered by the [CanonXL Noncommercial License, version 1.0](../LICENSE).
Third-party libraries retain their own licenses; the project license does not
replace them. Generated binaries and downloaded dependencies are not tracked.
