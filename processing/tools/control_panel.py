#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
# Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

"""Build/test/run the Processing panel without modifying global preferences."""

import argparse
import hashlib
import os
import plistlib
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sketch = root / "canon_xl_control"
defaultProcessing = Path("/Applications/Processing.app/Contents/MacOS/Processing")
dependencyHash = "33e5087ab957bd75277ff54d027f305fcb6938ef05fe3ff4f1e871bfe6c6d4cd"
serialHash = "b5f7ec40d622864b92dc679159cdbaa3030d37884f0af2deef45b56c16bda7a6"


def processingPath() -> Path:
    """Resolve an explicit runtime override, then the installed macOS application."""
    path = Path(os.environ.get("processingPath", str(defaultProcessing)))
    if not path.is_file():
        raise RuntimeError("Set processingPath to the Processing 4 launcher")
    return path


def javaDirectory() -> Path:
    """Use Processing's bundled Java rather than macOS's unconfigured Java stub."""
    directory = processingPath().parents[1] / "app/resources/jdk/bin"
    if not (directory / "javac").is_file():
        raise RuntimeError("Set up the bundled Processing JDK before running tests")
    return directory


def prepareDependency() -> None:
    """Use the pinned, unmodified G4P jar; never install a global sketchbook library."""
    jar = sketch / "code/G4P.jar"
    if not jar.exists():
        archive = root / "vendor/G4P-4.3.11.zip"
        if not archive.exists():
            raise RuntimeError("Download G4P 4.3.11 as described in README.md")
        with zipfile.ZipFile(archive) as package:
            content = package.read("G4P/library/G4P.jar")
        if hashlib.sha256(content).hexdigest() != dependencyHash:
            raise RuntimeError("Unexpected G4P jar checksum")
        jar.parent.mkdir(parents=True, exist_ok=True)
        jar.write_bytes(content)
    if hashlib.sha256(jar.read_bytes()).hexdigest() != dependencyHash:
        raise RuntimeError("Unexpected G4P jar checksum")
    serialJar = sketch / "code/jSerialComm-2.11.4.jar"
    if not serialJar.exists() or hashlib.sha256(serialJar.read_bytes()).hexdigest() != serialHash:
        raise RuntimeError("Install the pinned jSerialComm jar described in README.md")


def test(capture: Path | None) -> None:
    """Compile only pure Java contracts; tests do not load G4P or serial drivers."""
    output = root / "build/tests"
    output.mkdir(parents=True, exist_ok=True)
    java = javaDirectory()
    sources = [
        sketch / "ControllerProtocol.java",
        sketch / "ControllerSession.java",
        root / "tests/ControllerTests.java",
        root / "tests/CaptureReplay.java",
    ]
    subprocess.run(
        [str(java / "javac"), "-Xlint:all", "-d", str(output), *map(str, sources)], check=True
    )
    subprocess.run([str(java / "java"), "-ea", "-cp", str(output), "ControllerTests"], check=True)
    if capture:
        subprocess.run(
            [
                str(java / "java"),
                "-ea",
                "-cp",
                str(output),
                "CaptureReplay",
                str(capture.resolve()),
            ],
            check=True,
        )


def packageMac(output: Path, arguments: list[str]) -> Path:
    """Wrap Processing's flat bundled JDK for the native exported app launcher."""
    application = output / "canon_xl_control.app"
    runtime = application / "Contents/PlugIns/runtime/Contents/Home"
    shutil.copytree(
        javaDirectory().parent,
        runtime,
        symlinks=True,
        ignore=shutil.ignore_patterns("jmods", "man", "include"),
    )
    infoPath = application / "Contents/Info.plist"
    with infoPath.open("rb") as stream:
        info = plistlib.load(stream)
    info["JVMRuntime"] = "runtime"
    info["JVMArguments"] = arguments
    info["CFBundleIdentifier"] = "com.noxsilicium.canonxl.control"
    info["CFBundleDisplayName"] = "Canon XL Control"
    info["NSHumanReadableCopyright"] = "Copyright (c) 2026 Sebastien Marchand"
    info.pop("NSCameraUsageDescription", None)
    info.pop("NSMicrophoneUsageDescription", None)
    with infoPath.open("wb") as stream:
        plistlib.dump(info, stream)
    return application


def sketchOptions(options: argparse.Namespace) -> list[str]:
    """Keep CLI and exported application options identical."""
    arguments = []
    if options.mode == "demo" or options.simulate:
        arguments.append("--demo")
    if options.port:
        arguments.append(f"--port={options.port}")
    if options.language:
        arguments.append(f"--language={options.language}")
    if options.smoke:
        arguments.append(f"--smoke={options.smoke}")
    if options.raw:
        arguments.append("--raw")
    if options.benchTest:
        arguments.append("--bench-test")
    if options.record:
        arguments.append(f"--record={options.record.resolve()}")
    if options.screenshot:
        options.screenshot.resolve().parent.mkdir(parents=True, exist_ok=True)
        arguments.append(f"--screenshot={options.screenshot.resolve()}")
    return arguments


def main() -> None:
    """Finite runs return an exit status; detached GUI runs log to a generated directory."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=["test", "build", "run", "demo", "app"])
    parser.add_argument("--capture", type=Path, help="Replay a recorded bench session with test")
    parser.add_argument("--port", help="Explicit port; opening can reset the Nano")
    parser.add_argument("--language", choices=["en", "fr"], help="Initial UI language; default fr")
    parser.add_argument("--smoke", type=int, help="Exit after N seconds, without arming")
    parser.add_argument("--raw", action="store_true", help="Enable observational MISO sampling")
    parser.add_argument("--record", type=Path, help="New capture path, never overwrite")
    parser.add_argument("--screenshot", type=Path, help="Save the final frame of a smoke run")
    parser.add_argument("--detach", action="store_true", help="Leave the GUI running")
    parser.add_argument("--simulate", action="store_true", help="Simulation for an exported app")
    parser.add_argument(
        "--bench-test",
        dest="benchTest",
        action="store_true",
        help="Explicit iris/zoom/XY sequence; requires --smoke >= 15",
    )
    options = parser.parse_args()
    if options.mode == "test":
        test(options.capture)
        return
    if (options.mode == "demo" or options.simulate) and options.port:
        parser.error("Demo cannot open a physical port")
    if options.smoke is not None and options.smoke < 1:
        parser.error("Smoke duration must be positive")
    if options.detach and options.smoke:
        parser.error("Finite smoke tests cannot be detached")
    if options.screenshot and not options.smoke:
        parser.error("Screenshot requires a finite smoke duration")
    if options.record and options.record.exists():
        parser.error("Capture already exists; select a new path")
    if options.benchTest and (not options.smoke or options.smoke < 15):
        parser.error("Bench test requires --smoke >= 15")
    prepareDependency()
    buildRoot = root / "build"
    buildRoot.mkdir(parents=True, exist_ok=True)
    runDirectory = Path(tempfile.mkdtemp(prefix="panel-", dir=buildRoot))
    output = runDirectory / "compiled"
    if options.mode == "app":
        subprocess.run(
            [
                str(processingPath()),
                "cli",
                f"--sketch={sketch}",
                f"--output={output}",
                "--variant=macos-aarch64",
                "--no-java",
                "--export",
            ],
            check=True,
        )
        print(f"Local application: {packageMac(output, sketchOptions(options))}")
        return
    command = [
        str(processingPath()),
        "cli",
        f"--sketch={sketch}",
        f"--output={output}",
        "--build" if options.mode == "build" else "--run",
    ]
    command.extend(sketchOptions(options))
    if options.detach:
        log = runDirectory / "runtime.log"
        with log.open("w") as stream:
            process = subprocess.Popen(
                command,
                stdout=stream,
                stderr=subprocess.STDOUT,
                stdin=subprocess.DEVNULL,
                start_new_session=True,
            )
        print(f"Panel launcher PID: {process.pid}\nRuntime log: {log}")
    else:
        subprocess.run(command, check=True)


if __name__ == "__main__":
    main()
